#include "gw_world_simulation.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Soul Leech World line "+std::to_string(at.line()));
}
int main()try{
 for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);auto selected=target;auto caster=source;auto plan=*effects::soul_leech(12);
  if(mode==1)++selected.generation;if(mode==2)++caster.generation;if(mode==3)++plan.amount;
  if(mode==4)selected=source;
  std::vector<world::Packet> visuals;box->drain();
  check(w.stage_soul_leech(caster,selected,plan,1000,&visuals)==(mode==0));
  check(w.resources(1)->health==1000&&w.resources(2)->health==1000&&w.npc_combat_snapshot(*w.effect_scope(100))->health->current==1000);
  if(mode){check(visuals.empty());continue;}
  check(visuals.size()==1&&visuals[0]==*particles::targeted_body_2022(target.actor,1,128));
  const auto sent=box->drain();check(std::count(sent.begin(),sent.end(),visuals[0])==0);
  if(npc)check(w.npc_combat_snapshot(target)->effects&0x800);
  else {const auto statuses=w.status_snapshot(target,1000);check(statuses&&statuses->size()==1&&statuses->front().skill==128);}
  check(w.stage_soul_leech(source,target,*effects::soul_leech(0),2000));
  if(!npc){const auto statuses=w.status_snapshot(target,2000);check(statuses&&statuses->size()==1&&statuses->front().rank==0);}
  w.tick(12000);
  if(npc)check(!(w.npc_combat_snapshot(target)->effects&0x800));
  else check(w.status_snapshot(target,12000)->empty());
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{mode==1?10.:1000.,1000};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?(mode==2?1000u:500u):(mode==1?10u:1000u),1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  check(w.stage_soul_leech(source,target,*effects::soul_leech(12),1000));
  if(mode==4)check(w.backfire(source,target,*effects::backfire(28,0),20,1000));
  if(mode==5){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1500,1);w.set_vitals(1,{500,1000,100,100},100);check(*w.effect_scope(1)!=source);}
  const auto at=mode==3?11000u:2000u;const std::uint64_t event=1;box->drain();
  check(w.before_spell_effect(target,281,event,at)==(mode!=1));
  const auto hp=[&]{return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;};
  check(hp()==(mode==1?0:mode==3||mode==5?1000:mode==4?898:933));
  check(w.resources(1)->health==(mode==1?511:mode==2?1000:mode==3||mode==5?500:567));
  box->drain();check(!w.before_spell_effect(target,281,event,at)&&box->drain().empty());
  if(mode==1){if(npc){check(w.take_npc_deaths(source).size()==1);check(w.take_npc_deaths(source).empty());}}
  else {check(w.before_spell_effect(target,281,event+1,at));check(hp()==(mode==3||mode==5?1000:mode==4?796:866));}
 }
 std::cout<<"PASS trusted Soul Leech World scoped application and reserved spell transfers,duplicate suppression,expiry,full-source cap and lethal credit;Runtime/rendering/retail combined ordering pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
