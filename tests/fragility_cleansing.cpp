#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>
int main()try{
    using namespace fw::gw;
    for(unsigned initial:{30u,60u}){
    world::Instance instance;
    instance.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    instance.join(2,"Monk",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    instance.set_vitals(1,{1000,1000,100,100},100);
    instance.set_vitals(2,{initial,1000,100,100},100);
    const auto caster=*instance.effect_scope(1),target=*instance.effect_scope(2);
    if(!instance.degeneration_hex(caster,target,*skills::effects::degeneration_hex(19,12),100))
        throw std::runtime_error("Fragility application failed");
    if(!instance.apply_condition(caster,target,combat::conditions::Kind::blind,200,1000)||instance.resources(2)->health!=initial-17)
        throw std::runtime_error("initial Fragility trigger incorrect");
    const auto before_epoch=*instance.control_epoch(target);
    const auto plan=*skills::effects::condition_removal(1401,12,0);
    if(!instance.remove_conditions(target,2,plan,300))
        throw std::runtime_error("Mending Touch failed before removal");
    const unsigned expected=initial==30?0:initial-34+plan.per_condition_heal;
    if(instance.resources(2)->health!=expected)
        throw std::runtime_error("condition-removal healing result incorrect: HP="+std::to_string(instance.resources(2)->health));
    const auto deaths=*instance.control_epoch(target)-before_epoch;
    if(deaths!=(initial==30?1u:0u))throw std::runtime_error("condition-removal death cleanup count="+std::to_string(deaths));
    instance.tick(2000);
    if(instance.resources(2)->health!=expected)throw std::runtime_error("post-cleansing health changed unexpectedly");
    }
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
