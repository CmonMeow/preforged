#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Tainted Flesh World line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;
  for(unsigned id=1;id<=3;++id){w.join(id,"Player",id==2?(mode==3?1300.f:60.f):0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto caster=*w.effect_scope(1),foe=*w.effect_scope(3);auto target=*w.effect_scope(mode==0?1:2);if(mode==4)++target.generation;
  if(mode==2)target=foe;
  auto plan=*tainted_flesh(0);if(mode==5)++plan.duration;
  const bool accepted=mode<2;std::vector<world::Packet> visuals;
  if(accepted)check(w.rotting_flesh(foe,target,*rotting_flesh(0),100));
  check(w.tainted_flesh(caster,target,plan,200,&visuals)==accepted);
  if(!accepted){check(visuals.empty());continue;}
  check(!w.has_condition(target,Kind::disease,200));
  check(w.apply_condition(foe,target,Kind::disease,250,10000)&&!w.has_condition(target,Kind::disease,250));
  const auto donor=*w.effect_scope(mode==0?2:1);
  check(w.apply_condition(foe,donor,Kind::disease,260,10000));
  check(w.condition_spell(foe,donor,*condition_spell(78,0),270));
  check(!w.has_condition(target,Kind::disease,270)&&w.has_condition(donor,Kind::disease,270));
  check(!w.status_mutation(target,113,999,0,200,20200,true));
  if(target==caster)check(visuals==std::vector<world::Packet>{combat::detail::integer(21,target.actor,214)});
  else check(visuals==std::vector<world::Packet>{*skills::particles::targeted_body_2022(target.actor,caster.actor,113)});
  check(w.rotting_flesh(foe,target,*rotting_flesh(0),300)&&!w.has_condition(target,Kind::disease,300));
  auto entries=w.status_snapshot(target,300);check(entries&&entries->size()==1&&entries->front().skill==113);const auto effect=entries->front().instance;
  // Generic effects cannot overwrite an owned Tainted Flesh ID and orphan its timer.
  check(!w.status_mutation(target,299,effect,0,300,8300,true));
  if(mode==0){check(w.tainted_flesh(caster,target,*tainted_flesh(12),1000));entries=w.status_snapshot(target,1000);check(entries&&entries->size()==1&&entries->front().instance!=effect);
   check(w.rotting_flesh(foe,target,*rotting_flesh(0),20200)&&!w.has_condition(target,Kind::disease,20200));
   check(w.rotting_flesh(foe,target,*rotting_flesh(0),40000)&&w.has_condition(target,Kind::disease,40000));
  }else{check(w.status_mutation(target,113,effect,0,400,std::nullopt,false));
   check(w.rotting_flesh(foe,target,*rotting_flesh(0),400)&&w.has_condition(target,Kind::disease,400));}
 }
 {
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;d.relation=combat::npcs::Relation::allied_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;d.disease_immune=false;d.disease_immunity_evidence="fixture";
  const combat::npcs::Lifetime life{1,1,7,146,100};combat::npcs::LiveActor live(d,life);const Scope caster{1,1,2};
  check(live.apply_condition(life,Kind::disease,2,100,10000));check(live.record_disease_credit(life,Scope{1,2,3},100));
  check(live.apply_tainted_flesh(life,caster,*tainted_flesh(0),1,200).has_value());
  check(!live.has_condition(life,Kind::disease,200)&&!live.disease_credit(life,200)&&live.disease_immune(life,200)==true);
  const auto renewed=live.apply_tainted_flesh(life,caster,*tainted_flesh(12),2,1000);
  check(renewed&&renewed->removed.size()==1&&!live.hex_effect(life,1));
  check(live.disease_immune(life,20200)==true&&live.disease_immune(life,40000)==false);
  check(live.remove_reviewed_enchantment(life,2,20200)&&live.disease_immune(life,20200)==false);
 }
 for(unsigned mode=0;mode<3;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=mode==1?combat::npcs::Relation::neutral:mode==2?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto target=*w.effect_scope(100);check(w.tainted_flesh(*w.effect_scope(1),target,*tainted_flesh(12),100)==(mode==0));
 }
 {
  world::Instance w;
  for(unsigned id=1;id<=2;++id){w.join(id,"Player",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto ally=*w.effect_scope(1),foe=*w.effect_scope(2);
  check(w.rotting_flesh(foe,ally,*rotting_flesh(0),100));w.set_vitals(1,{1,1000,100,100},100);
  check(w.degeneration_hex(foe,ally,*degeneration_hex(19,12),100));
  check(w.tainted_flesh(ally,ally,*tainted_flesh(0),150));
  check(w.resources(1)->health==0&&!w.has_condition(ally,Kind::disease,150));
  const auto entries=w.status_snapshot(ally,150);check(entries&&entries->empty());
 }
 std::cout<<"PASS Tainted Flesh owned player/NPC application,Disease cleanse,immunity expiry/removal/renewal,Fragility removal death,self214 wire and invalid targets;retaliation/Runtime tested separately\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
