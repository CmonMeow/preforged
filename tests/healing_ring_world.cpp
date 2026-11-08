#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Healing Ring World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank:{0u,12u,20u})for(unsigned divine:{0u,12u}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
  w.join(2,"Ally",166,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(3,"Foe",-100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.join(4,"Far",167,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u,3u,4u})w.set_vitals(id,{100,500,30,30},100);
  const auto source=*w.effect_scope(1);const auto plan=*skills::effects::healing_ring(rank,divine);
  box->drain();check(w.healing_ring(source,plan,100));check(w.resources(1)->health==100+plan.caster_healing);
  const auto feedback=box->drain();
  for(unsigned id:{2u,3u})check(std::count(feedback.begin(),feedback.end(),skills::particles::health_change_2022(id,1,plan.healing,500))==1);
  if(divine)check(std::count(feedback.begin(),feedback.end(),skills::particles::health_change_2022(1,1,plan.caster_healing,500))==1);
  check(w.resources(2)->health==100+plan.healing&&w.resources(3)->health==100+plan.healing&&w.resources(4)->health==100);
  auto stale=source;++stale.generation;check(!w.healing_ring(stale,plan,100));
  auto forged=plan;++forged.healing;check(!w.healing_ring(source,forged,100));
 }
 const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 80 0 0 - -\n");
 auto population=world::load_npc_catalog(appearance);std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/healing_ring_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 100 - 1 0 test test - - - - - - -\n");
 world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
 for(unsigned mode=0;mode<5;++mode){auto defs=population.for_map(146);
  if(mode<2||mode==3){defs.front().combat_definition->healing_allowed=mode!=1;defs.front().combat_definition->healing_evidence="test";}
  if(mode==4)defs.front().x=200;
  world::Instance w;w.configure_npcs(defs);auto box=std::make_shared<world::Outbox>();w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{100,500,30,30},100);w.set_vitals(2,{100,500,30,30},100);const auto owner=*w.effect_scope(1),npc=*w.effect_scope(100);
  if(mode==3)check(w.damage_npc(owner,npc,{combat::Kind::armor_ignoring,1000,20,133},1248,100).killed);
  box->drain();check(w.healing_ring(owner,*skills::effects::healing_ring(12,0),100)==(mode!=2));
  const auto packets=box->drain();
  check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(100,1,150,500))==(mode==0?1:0));
  check(w.npc_combat_snapshot(npc)->health->current==(mode==0?250:mode==3?0:100));check(w.resources(2)->health==(mode!=2?250u:100u));
 }
 {world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Target",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{500,500,30,30},100);w.set_vitals(2,{499,500,30,30},100);const auto owner=*w.effect_scope(1);const auto plan=*skills::effects::healing_ring(12,0);
  box->drain();check(w.healing_ring(owner,plan,100));const auto first=box->drain();
  check(std::count(first.begin(),first.end(),skills::particles::health_change_2022(2,1,1,500))==1);
  check(w.healing_ring(owner,plan,100));const auto full=box->drain();
  check(std::none_of(full.begin(),full.end(),[](const auto& p){return p.size()==18&&p[0]==162&&p[2]==55;}));
 }
 // Resolve natural death before deciding whether a recipient can be healed.
 for(bool dense:{false,true})for(bool caster_dies:{false,true})for(auto at:{266u,267u}){
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:80,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==(caster_dies?1u:2u)?1u:500u,500,30,30},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2),dying=caster_dies?owner:target;
  check(w.apply_condition(owner,dying,combat::conditions::Kind::bleeding,100,10000));
  if(dense)w.tick(200);
  check(w.healing_ring(owner,*skills::effects::healing_ring(12,0),at)==(!caster_dies||at==266));
  if(at==267){check(w.resources(dying.actor)->health==0);check(w.control_event_time(dying,0)==267);}
  else if(!caster_dies)check(w.resources(2)->health>=150&&w.resources(2)->health<=151);
 }
 for(bool dense:{false,true})for(auto at:{266u,267u}){
  auto defs=population.for_map(146);auto& definition=*defs.front().combat_definition;
  definition.spawn_health=combat::Health{1,500};definition.conditions_allowed=true;definition.healing_allowed=true;definition.healing_evidence="test";
  definition.fleshy=true;definition.fleshy_evidence="test authored bleeding recipient";
  world::Instance w;w.configure_npcs(defs);w.join(1,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,30,30},100);
  const auto owner=*w.effect_scope(1),npc=*w.effect_scope(100);
  check(w.condition_spell(owner,npc,*skills::effects::condition_spell(131,0),100));
  if(dense)w.tick(200);
  check(w.healing_ring(owner,*skills::effects::healing_ring(12,0),at));
  const auto health=w.npc_combat_snapshot(npc)->health->current;
  check(at==267?health==0:health>150&&health<151);
 }
 for(bool dense:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Target",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{100,500,30,30},100);const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.apply_parasitic_bond(owner,target,0,100));
  if(dense)for(unsigned at=200;at<20100;at+=100)w.tick(at);
  box->drain();check(w.healing_ring(owner,*skills::effects::healing_ring(12,0),20100));
  check(w.resources(1)->health==130&&w.resources(2)->health>=210&&w.resources(2)->health<=211);
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(2,1,150,500))==1);
  const auto before=*w.resources(1);w.tick(21100);check(w.resources(1)->health==before.health); // End return consumed exactly once.
 }
 // Verata-style following: range must use the NPC position at cast time,
 // even if no ordinary World tick was sampled while he approached.
 for(bool dense:{true,false})for(bool enters:{false,true})for(bool retired:{false,true}){
  auto defs=population.for_map(146);defs.front().x=0;auto& definition=*defs.front().combat_definition;
  definition.relation=combat::npcs::Relation::neutral;definition.healing_allowed=true;definition.healing_evidence="test";
  world::Instance w;w.configure_npcs(defs);w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,30,30},100);
  w.join(2,"Stationary Healer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,500,30,30},100);
  const auto owner=*w.effect_scope(1),guide=*w.effect_scope(100),healer=*w.effect_scope(2);
  auto enemies=population.for_map(146);enemies.front().x=1000;enemies.front().combat_definition->healing_allowed=false;enemies.front().combat_definition->healing_evidence="test";
  check(w.spawn_corpse_encounter(owner,guide,1,enemies,[]{return true;})&&w.arm_corpse_follow(owner,1,guide,100));
  w.move(1,500,0,101);w.tick(2501);check(w.follow_corpse_trainer(owner,1,220,2502));
  if(retired){w.leave(1);w.join(1,"Replacement",500,0,std::make_shared<world::Outbox>(),0,{}, {},2503,1);w.set_vitals(1,{500,500,30,30},100);}
  if(dense)w.tick(4502);
  check(w.healing_ring(!retired&&enters?owner:healer,*skills::effects::healing_ring(12,0),4502));
  check(w.npc_combat_snapshot(guide)->health->current==(retired||enters?250:100));
 }
 std::cout<<"PASS Healing Ring World mixed players/NPCs, immune/unknown policy, radius, self Divine Favor and stale/forged rollback and recipient healing feedback\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
