#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("IW armor line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned penetration:{0u,25u,100u}){
  combat::Actor ordinary{2,1,combat::Health{1000,1000},60,0,true};auto protected_actor=ordinary;
  protected_actor.post_penetration_armor=20;
  const combat::Damage hit{combat::Kind::earth,100,20,174,penetration};
  const auto baseline=combat::apply(ordinary,hit,true),protected_hit=combat::apply(protected_actor,hit,true);
  check(baseline.reason==combat::Reason::accepted&&protected_hit.reason==combat::Reason::accepted);
  check(std::abs(protected_hit.damage/baseline.damage-std::exp2(-.5))<1e-10);
 }
 {combat::Actor target{2,1,combat::Health{1000,1000},60,0,true};target.post_penetration_armor={};
  check(combat::apply(target,{combat::Kind::earth,100,20,174},true).reason!=combat::Reason::accepted);
  check(combat::apply(target,{combat::Kind::armor_ignoring,100,20,133},true).damage==100);
 }
 for(bool pvp:{false,true})for(bool changed:{false,true})for(unsigned kind=0;kind<4;++kind){
  world::Instance w;w.join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Mesmer",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{10000,10000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.physical_armor(target,60)&&w.combat_armor(target,60)&&w.elemental_armor(target,60,60)&&w.lightning_armor(target,60));
  skills::Slots bar{};bar[0]={33,0};bar[1]={11,0};check(w.configure_adrenaline(target,bar,100));
  check(w.status_mutation(target,pvp?3181:33,123,12,100,30100,true));
  if(changed){bar[2]={43,0};check(w.configure_adrenaline(target,bar,150));}
  const auto factor=pvp?1.:std::exp2(-double(changed?15:10)/40.);double damage=0;
  if(kind==0){const auto plan=*skills::effects::lightning(224,0);check(w.lightning(source,target,plan,20,200));damage=25*std::exp2(15./40.)*factor;}
  if(kind==1){const auto plan=*skills::effects::elemental(191,0);check(w.elemental(source,target,plan,20,200));damage=20*factor;}
  if(kind==2){combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(source,sword,20));check(w.ordinary_attack(source,2,200));w.tick(700);w.cancel_ordinary_attack(source);damage=20*factor;}
  if(kind==3){check(w.shadow_damage(source,target,*skills::effects::shadow_damage(133,0),20,200));damage=10;}
  check(w.resources(2)->health==unsigned(std::ceil(10000.-damage)));
 }
 std::cout<<"PASS IW final armor ignores penetration,uses owned current bar,PvP zero bonus,physical/elemental paths and armor-ignoring exclusion\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
