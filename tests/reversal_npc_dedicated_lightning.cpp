#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC dedicated lightning Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;skills::effects::Scope source,target;
 Fixture(unsigned hp,unsigned own=80,std::optional<bool> control=true){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={double(hp),1000};
  d.damageable=true;d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;d.lightning_armor=80;
  d.spawn_energy=combat::npcs::Energy{20,100};d.energy_pips=0;d.healing_allowed=true;d.healing_evidence="synthetic";
  d.knockdown_allowed=control;if(control.has_value())d.knockdown_evidence="synthetic";
  n.combat_definition=d;w.configure_npcs({n});w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(1,{1000,1000,own,100},100);check(w.energy_capacity(1,6,0,100,{80,-2}));source=*w.effect_scope(1);target=*w.effect_scope(100);
 }
 double health(){return w.npc_combat_snapshot(target)->health->current;}
 bool cast(unsigned skill){return w.lightning(source,target,*skills::effects::lightning(skill,12),20,1000);}
};
int main()try{
 unsigned cases=0;
 for(unsigned skill:{205u,222u,226u,2804u})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})
 for(unsigned mode=0;mode<(skill==205?1u:2u);++mode){
  ++cases;Fixture f(hp,(skill==226||skill==2804)&&!mode?20:80);
  if(skill==222&&mode)check(f.w.activate_overcast_energy(f.source,219,0,1,100));
  check(f.w.stage_npc_reversal(f.target,rr,100));check(f.cast(skill));
  const auto plan=*skills::effects::lightning(skill,12);const double raw=plan.damage;
  if(skill==205){check(f.health()==hp);f.w.tick(4000);}
  const double converted=std::min(raw,double(skills::effects::reversal_fortune(rr)->maximum_healing));
  double expected=std::max(0.,std::min(1000.,hp+converted)-(raw-converted));
  if((skill==226||skill==2804)&&mode&&expected>0)expected=std::max(0.,expected-raw);
  check(std::abs(f.health()-expected)<1e-7);
  const bool knock=expected>0&&(skill==205||(skill==226||skill==2804)&&mode);
  check(f.w.knocked_down(f.target,skill==205?4000:1000)==knock);
  if(skill==205)check(f.w.has_condition(f.target,combat::conditions::Kind::cracked_armor,4000)==(expected>0));
  if(skill==222)check(bool(f.w.npc_combat_snapshot(f.target)->effects&0x800)==(mode&&expected>0));
  check(f.w.take_npc_deaths(f.source).size()==unsigned(expected<=0));
 }
 check(cases==63);
 for(unsigned rr:{0u,12u}){Fixture f(1000);check(f.w.activate_overcast_energy(f.source,219,0,1,100)&&f.cast(222));
  check(f.health()==959&&f.w.stage_npc_reversal(f.target,rr,2000));f.w.tick(4000);check(f.health()==(rr?1000:948));}
 for(unsigned skill:{226u,2804u}){Fixture f(10,80,std::nullopt);check(f.w.stage_npc_reversal(f.target,12,100));
  check(!f.cast(skill)&&f.health()==10);check(f.w.npc_reversal_damage(f.source,f.target,{combat::Kind::armor_ignoring,1,20,42},1000));}
 std::cout<<"PASS63 dedicated NPC lightning combinations,2 late Strike conversions,2 unknown-control rejections;paid307 remains unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
