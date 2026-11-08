#include "gw_world_simulation.h"
#include <iostream>
#include <limits>
#include <source_location>
int main()try{
 using namespace fw::gw;auto check=[](bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("health sacrifice regression at line "+std::to_string(at.line()));};
 auto loss=skills::sacrifice::resolve(118,480,480);check(loss&&loss->lost==48&&loss->remaining==432&&!loss->killed);
 loss=skills::sacrifice::resolve(118,10,480);check(loss&&loss->requested==48&&loss->lost==11&&loss->remaining==0&&loss->killed);
 loss=skills::sacrifice::resolve(118,48,480);check(loss&&loss->lost==48&&loss->killed);
 check(!skills::sacrifice::resolve(117,480,480));check(!skills::sacrifice::resolve(118,std::numeric_limits<double>::quiet_NaN(),480));
 world::Instance instance;auto owner=std::make_shared<world::Outbox>(),peer=std::make_shared<world::Outbox>();
 instance.join(1,"Caster",0,0,owner,0,{}, {},100,1);instance.join(2,"Peer",10,0,peer,0,{}, {},100,1);
 instance.set_vitals(1,{480,480,20,20},100);const auto scope=*instance.effect_scope(1);owner->drain();peer->drain();
 check(!instance.health_sacrifice({scope.instance,scope.actor,scope.generation+1},118,100));
 check(!instance.health_sacrifice(scope,118,99));check(instance.resources(1)->health==480);
 loss=instance.health_sacrifice(scope,118,100);check(loss&&instance.resources(1)->health==432&&!owner->drain().empty()&&!peer->drain().empty());
 instance.set_vitals(1,{10,480,20,20},100);loss=instance.health_sacrifice(scope,118,100);
 check(loss&&loss->killed&&instance.resources(1)->health==0);check(!instance.health_sacrifice(scope,118,100));
 instance.leave(1);instance.join(1,"Reused",0,0,owner,0,{}, {},101,1);instance.set_vitals(1,{480,480,20,20},101);
 check(!instance.health_sacrifice(scope,118,101));check(instance.resources(1)->health==480);
 instance.set_vitals(1,{1,1,20,20},100);
 loss=instance.health_sacrifice(*instance.effect_scope(1),119,101);check(loss&&loss->lost==0&&!loss->killed&&instance.resources(1)->health==1);
 std::cout<<"PASS sacrifice maximum health, unavoidable scoped loss, lethal cleanup, stale clocks/lifetimes and publication\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
