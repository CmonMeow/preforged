#include "gw_stone_daggers.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Stone Daggers line "+std::to_string(at.line()));}
combat::Actor foe(double health=1000){combat::Actor a;a.id=2;a.generation=1;a.health=combat::Health{health,1000};a.earth_armor=60;a.damageable=true;return a;}
int main()try{
 const auto p=*stone_daggers(12);check(p.projectiles==2&&p.damage==28&&p.bleeding_ms==4000&&p.model==306);
 check(stone_daggers(0)->damage==8&&stone_daggers(15)->damage==33&&stone_daggers(20)->bleeding_ms==6000&&!stone_daggers(21));
 auto a=foe();a.incoming_reduction=10;
 const auto first=stone_dagger_hit(p,a,{},20,1,false,{},1000);check(first.accepted&&first.actor.health->current==982&&!first.bleeding_applied);
 const auto second=stone_dagger_hit(p,first.actor,first.conditions,20,1,true,true,1500);
 check(second.accepted&&second.actor.health->current==964&&second.bleeding_applied&&second.conditions.lookup(combat::conditions::Kind::bleeding)->expires==5500);
 check(a.health->current==1000); // Proposals never mutate either live input.
 auto unknown=foe();unknown.earth_armor.reset();check(!stone_dagger_hit(p,unknown,{},20,1,false,{},1000).accepted);
 check(!stone_dagger_hit(p,foe(),{},20,1,{},true,1000).accepted);
 check(!stone_dagger_hit(p,foe(),{},20,1,true,{},1000).accepted);
 check(stone_dagger_hit(p,foe(),{},20,1,true,false,1000).accepted&&!stone_dagger_hit(p,foe(),{},20,1,true,false,1000).bleeding_applied);
 const auto lethal=stone_dagger_hit(p,foe(10),{},20,1,true,{},1000);check(lethal.accepted&&lethal.hit.killed&&!lethal.bleeding_applied);
 check(!stone_dagger_hit(p,lethal.actor,lethal.conditions,20,1,true,true,1500).accepted);
 auto forged=p;forged.projectiles=1;check(!stone_dagger_hit(forged,foe(),{},20,1,false,true,1000).accepted);
 check(!stone_dagger_hit(p,foe(),{},20,2,false,true,1000).accepted);
 combat::conditions::State covered;check(covered.apply(combat::conditions::Kind::bleeding,3,100,10000));
 const auto refreshed=stone_dagger_hit(p,foe(),covered,20,1,true,true,1000);check(refreshed.accepted&&refreshed.conditions.lookup(combat::conditions::Kind::bleeding)->expires==10100);
 check(!stone_dagger_hit(p,foe(),{},20,1,true,true,UINT64_MAX-100).accepted);
 std::cout<<"PASS two independent earth impacts,per-hit protection,Overcast bleeding,unknown-state refusal,death,canonical plan and condition refresh;World/Runtime unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
