#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Ward177 history line "+std::to_string(at.line()));
}
int main()try{
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    auto foe_box=std::make_shared<world::Outbox>();
    w.join(1,"Caster",0,0,box,0,{}, {},100,1);
    w.join(2,"Foe",60,0,foe_box,0,{}, {},100,2);
    w.set_vitals(1,{1000,1000,100,100},100);
    w.set_vitals(2,{1000,1000,100,100},100);
    const auto source=*w.effect_scope(1),foe=*w.effect_scope(2);
    const auto plan=*ward_foes_plan(0);
    std::vector<world::Packet> visuals;
    check(w.apply_ward_foes(source,plan,100,&visuals));
    const auto initial=w.ward_foes_history(foe,100,8100);
    check(initial&&initial->size()==1);
    const auto field=initial->front();
    check(w.ward_foes_history(source,100,8100)->empty());
    check(!w.ward_foes_history(foe,8100,100));
    box->drain();foe_box->drain();
    w.tick(9000);
    const auto removed=box->drain();
    world::Packet removal;world::u16(removal,67);world::u32(removal,1);world::u32(removal,field.effect);
    check(std::count(removed.begin(),removed.end(),removal)==1);
    check(!w.ward_foes_multiplier(foe,8100));
    // A consumer starting before expiry must still receive the complete field
    // after an unrelated timed drain has passed expiry.
    const auto retained=w.ward_foes_history(foe,100,9000);
    check(retained&&retained->size()==1&&retained->front().expires==8100);
    check(w.ward_foes_history(foe,8100,9000)->empty());
    w.tick(10000);
    const auto repeated=box->drain();
    check(std::count(repeated.begin(),repeated.end(),removal)==0);
    check(w.ward_foes_history(foe,100,10000)->size()==1);
    std::cout<<"Ward177 expired history retained across sparse ticks; removal published once\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

