#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
void check(bool v,std::source_location at=std::source_location::current()){
    if(!v)throw std::runtime_error("Channeling debt regression line "+std::to_string(at.line()));
}
int main()try{
    using namespace fw::gw;
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
    w.join(2,"Foe A",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    w.join(3,"Foe B",250,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    w.set_vitals(1,{1000,1000,3,100},100);
    w.set_vitals(2,{1000,1000,40,100},100);w.set_vitals(3,{1000,1000,40,100},100);
    const auto scope=*w.effect_scope(1);
    check(w.energy_capacity(1,5,0,100,{80,-2}));
    check(w.status_mutation(scope,38,10,12,100,46100,true));
    // Dropping80 capacity with only3 Energy creates77 debt. A two-foe
    // refund must repay debt, never create positive UI Energy alongside it.
    check(w.energy_capacity(1,5,0,200,{0,-2}));
    check(w.resources(1)->max_energy==20&&w.resources(1)->energy==0);
    check(w.prepare_channeling(scope,281,1,300)&&w.complete_channeling(scope,1,true,300));
    check(w.resources(1)->energy==0);
    // Restoring capacity exposes the remaining signed balance:3+2=5.
    check(w.energy_capacity(1,5,0,400,{80,-2}));
    check(w.resources(1)->max_energy==100&&w.resources(1)->energy==5);
    check(w.prepare_channeling(scope,281,2,500)&&w.complete_channeling(scope,2,true,500));
    check(w.resources(1)->energy==7);
    check(w.prepare_channeling(scope,281,3,600));
    w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},700,1);
    w.set_vitals(1,{1000,1000,40,100},100);const auto replacement=*w.effect_scope(1);
    check(replacement!=scope&&!w.complete_channeling(scope,3,true,700));
    check(w.resources(1)->energy==40);
    check(w.prepare_channeling(replacement,281,1,700)&&w.complete_channeling(replacement,1,true,700));
    check(w.resources(1)->energy==40);
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
