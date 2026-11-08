#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Tainted Flesh retaliation line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<14;++mode){
  world::Instance w;w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return .1;});
  if(mode==1||mode==9){
   world::Instance::Npc n;n.id=n.definition.id=100;n.x=220;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1,1000};d.damageable=d.conditions_allowed=true;
   d.fleshy=true;d.fleshy_evidence="fixture";d.creature_type="human";d.creature_type_evidence="fixture";
   d.disease_immune=false;d.disease_immunity_evidence="fixture";n.combat_definition=d;w.configure_npcs({n});
  }
  for(unsigned id=1;id<=3;++id){w.join(id,"Actor",id==3?60.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{1000,1000,100,100},100);check(w.physical_armor(*w.effect_scope(id),60));}
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2),attacker=*w.effect_scope(3);
  check(w.tainted_flesh(caster,target,*tainted_flesh(0),100));
  const auto entry=w.status_snapshot(target,100)->front();
  if(mode==2)check(w.apply_condition(caster,attacker,Kind::blind,200,5000));
  if(mode==3)check(w.apply_health_regeneration(target,2,*skills::regeneration::reviewed(258,12),200,6200,1248,true,target));
  if(mode==6)check(w.status_mutation(target,113,entry.instance,0,200,std::nullopt,false));
  if(mode==7)check(w.tainted_flesh(caster,target,*tainted_flesh(12),200));
  if(mode==8)w.set_vitals(2,{1,1000,100,100},100);
  if(mode==9){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{1000,1000,100,100},100);check(*w.effect_scope(1)!=caster);}
  if(mode==10)check(w.kill(caster,200));
  if(mode==11)check(w.tainted_flesh(attacker,attacker,*tainted_flesh(0),200));
  if(mode==12){w.set_vitals(3,{1,1000,100,100},100);check(w.degeneration_hex(caster,attacker,*degeneration_hex(19,12),200));}
  if(mode==13)check(w.physical_absorption(target,3));
  combat::weapon::Owned weapon{1,1,{mode==13?1u:20u,mode==13?1u:20u,2},12,false,false,mode==4?5u:27u};
  check(w.ordinary_weapon(attacker,weapon,20));const auto at=mode==5?20200u:600u;
  if(mode==0||mode==13){check(w.ordinary_attack(attacker,2,100));w.tick(600);w.cancel_ordinary_attack(attacker);}
  else if(mode==4){check(w.melee_attack(attacker,target,weapon,*combat::weapon::attack_skill(394,12),at));w.tick(at+100);}
  else check(w.melee_attack(attacker,target,weapon,*combat::weapon::attack_skill(322,0),at));
  const bool infected=mode!=2&&mode!=3&&mode!=4&&mode!=5&&mode!=6&&mode!=11&&mode!=12;
  if(w.has_condition(attacker,Kind::disease,at)!=infected)std::cerr<<"mode="<<mode<<'\n';
  check(w.has_condition(attacker,Kind::disease,at)==infected);
  if(infected){const auto expires=at+(mode==7?13000:3000);check(w.has_condition(attacker,Kind::disease,expires-1)&&!w.has_condition(attacker,Kind::disease,expires));}
  if(mode==8)check(w.resources(2)->health==0);
  if(mode==12)check(w.resources(3)->health==0&&w.resources(2)->health<1000);
  if(mode==13)check(w.resources(2)->health==1000);
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==1);
  w.tick(at+101);check(w.take_attack_outcomes(attacker).empty());
  if(mode==1||mode==9){
   const auto npc=*w.effect_scope(100);check(w.has_condition(npc,Kind::disease,at+101));w.tick(1000);
   check(w.npc_combat_snapshot(npc)->health->current==0);
   check(w.take_npc_deaths(caster).size()==unsigned(mode==1)&&w.take_npc_deaths(attacker).empty());
   if(mode==9)check(w.take_npc_deaths(*w.effect_scope(1)).empty());
  }
 }
 std::cout<<"PASS Tainted Flesh player ordinary/skill melee,miss/block/ranged exclusions,zero-damage hit,renewal/expiry/removal,killing blow,old caster lifetime and Fragility death;NPC delivery/Runtime tested separately\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
