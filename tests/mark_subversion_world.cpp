#include "gw_world_simulation.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mark Subversion World line "+std::to_string(at.line()));
}
int main()try{
 for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);auto selected=target;auto caster=source;auto plan=*effects::mark_subversion(12);
  if(mode==1)++selected.generation;if(mode==2)++caster.generation;if(mode==3)++plan.amount;
  if(mode==4)selected=source;
  std::vector<world::Packet> visuals;box->drain();
  check(w.stage_mark_subversion(caster,selected,plan,1000,&visuals)==(mode==0));
  check(w.resources(1)->health==1000&&w.resources(2)->health==1000&&w.npc_combat_snapshot(*w.effect_scope(100))->health->current==1000);
  if(mode){check(visuals.empty());continue;}
  check(visuals.size()==1&&visuals[0]==*particles::targeted_body_2022(target.actor,1,127));
  const auto sent=box->drain();check(std::count(sent.begin(),sent.end(),visuals[0])==0);
  if(npc)check(w.npc_combat_snapshot(target)->effects&0x800);
  else {const auto statuses=w.status_snapshot(target,1000);check(statuses&&statuses->size()==1&&statuses->front().skill==127);}
  check(w.stage_mark_subversion(source,target,*effects::mark_subversion(0),2000));
  if(!npc){const auto statuses=w.status_snapshot(target,2000);check(statuses&&statuses->size()==1&&statuses->front().rank==0);}
  w.tick(8000);
  if(npc)check(!(w.npc_combat_snapshot(target)->effects&0x800));
  else check(w.status_snapshot(target,8000)->empty());
 }
 {
  world::Instance w;
  std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u,102u,103u,104u}){world::Instance::Npc n;n.id=n.definition.id=id;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;if(id!=103)d.packed_allegiance=id==102?8:id==104?0x616e696d:7;n.combat_definition=d;npcs.push_back(n);}
  w.configure_npcs(npcs);
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==2?1:id==3?2:0);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto scope=[&](unsigned id){return *w.effect_scope(id);};
  using R=effects::SpellTargetRelation;
  check(w.subversion_spell_target(scope(1),scope(2))->relation==R::ally);
  check(w.subversion_spell_target(scope(1),scope(3))->relation==R::foe);
  check(w.subversion_spell_target(scope(1),scope(4))->relation==R::unknown);
  check(w.subversion_spell_target(scope(100),scope(100))->relation==R::ally);
  check(w.subversion_spell_target(scope(100),scope(101))->relation==R::ally);
  check(w.subversion_spell_target(scope(100),scope(102))->relation==R::foe);
  check(w.subversion_spell_target(scope(100),scope(103))->relation==R::unknown);
  check(w.subversion_spell_target(scope(100),scope(104))->relation==R::unknown);
  check(w.subversion_spell_target(scope(1),scope(100))->relation==R::foe);
  check(w.subversion_spell_target(scope(100),scope(1))->relation==R::foe);
  auto stale=scope(101);++stale.generation;check(!w.subversion_spell_target(scope(100),stale));
  effects::Registry registry(scope(100));check(registry.apply(scope(100),{77,1,127,12,effects::Category::hex,100,6100},effects::Replace::same_skill).accepted);
  const auto selected=*w.subversion_spell_target(scope(100),scope(101));
  check(effects::prepare_mark_subversion_failure(registry,{scope(1),scope(100),77,100},scope(1),scope(100),scope(101),281,selected,1,1000,{50,1000},{1000,1000}).has_value());
  const auto foe=*w.subversion_spell_target(scope(100),scope(102));
  check(!effects::prepare_mark_subversion_failure(registry,{scope(1),scope(100),77,100},scope(1),scope(100),scope(102),281,foe,1,1000,{50,1000},{1000,1000}));
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{mode==2?10.:1000.,1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?500u:mode==2?10u:1000u,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);check(w.stage_mark_subversion(source,target,*effects::mark_subversion(12),1000));
  const unsigned at=mode==3?7000:2000;const std::uint64_t event=1;
  if(mode==5)w.tick(at+1); // World may advance beyond the Runtime timestamp.
  if(mode!=4)check(w.before_spell_effect(target,281,event,at));
  using R=effects::TargetedSpellFailure;const auto selected=mode==1?source:target;
  if(mode==5)check(w.mark_subversion_spell_failure(target,281,selected,event,at+2)==R::rejected);
  if(mode==5)check(w.mark_subversion_spell_failure(target,959,selected,event,at)==R::rejected);
  const auto result=w.mark_subversion_spell_failure(target,281,selected,event,at);
  check(result==(mode==4?R::rejected:mode==1||mode==3?R::none:R::failed));
  const auto hp=[&]{return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;};
  check(hp()==(mode==2?0:(mode==0||mode==5)?924:1000));check(w.resources(1)->health==(mode==2?511:(mode==0||mode==5)?576:500));
  box->drain();check(w.mark_subversion_spell_failure(target,281,selected,event,at)==R::rejected&&box->drain().empty());
  if(mode==2&&npc){check(w.take_npc_deaths(source).size()==1&&w.take_npc_deaths(source).empty());}
  if(mode==0){check(w.before_spell_effect(target,281,2,at));check(w.mark_subversion_spell_failure(target,281,target,2,at)==R::none&&hp()==924);}
 }
 std::cout<<"PASS Mark World application, replacement, expiry, scope guards, deferred235 and reserved spell failure clocks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}




