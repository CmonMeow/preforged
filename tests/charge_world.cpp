#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Charge World line "+std::to_string(at.line()));}
world::Instance::Npc creature(unsigned id,combat::npcs::Relation relation,float x=100){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=relation;d.spawn_health=combat::Health{500,500};d.damageable=d.conditions_allowed=relation!=combat::npcs::Relation::protected_ally;n.combat_definition=d;return n;
}
bool charged(world::Instance& w,Scope scope,std::uint64_t now){const auto entries=w.status_snapshot(scope,now);return entries&&std::any_of(entries->begin(),entries->end(),[](auto e){return e.skill==364;});}
int main()try{
 for(unsigned rank=0;rank<=20;++rank)for(unsigned mode=0;mode<4;++mode){
  world::Instance w;w.configure_npcs({creature(100,combat::npcs::Relation::allied_to_players),creature(101,combat::npcs::Relation::protected_ally),creature(102,combat::npcs::Relation::neutral),creature(103,combat::npcs::Relation::hostile_to_players),creature(104,combat::npcs::Relation::allied_to_players,1013)});
  auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=7;++id){const float x=id==3?1012:id==4?1013:id==1?0:100;const unsigned team=id==5?2:id==6?0:1;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,team);w.set_vitals(id,{id==7?0u:500u,1000,100,100},100);}
  const auto source=*w.effect_scope(1),ally=*w.effect_scope(2);check(w.apply_condition(source,ally,combat::conditions::Kind::crippled,100,10000));check(w.apply_condition(source,ally,combat::conditions::Kind::blind,100,10000));
  if(mode==3)check(w.consume_energy(2,1,ally,2000));
  box->drain();auto plan=*charge(rank);auto actor=source;if(mode==1)++plan.energy;if(mode==2)++actor.generation;
  std::function<void()> publication;check(w.stage_charge_allies(actor,plan,1000,&publication)==(mode==0));
  if(mode){check(!publication&&!charged(w,source,1000)&&!charged(w,ally,1000)&&w.has_condition(ally,combat::conditions::Kind::crippled,1000));continue;}
  // Independent parties with the same team are still allies of this shout.
  for(unsigned id:{1u,2u,3u,100u,101u})check(charged(w,*w.effect_scope(id),1000));
  for(unsigned id:{4u,5u,6u,7u,102u,103u,104u})check(!charged(w,*w.effect_scope(id),1000));
  check(!w.has_condition(ally,combat::conditions::Kind::crippled,1000)&&w.has_condition(ally,combat::conditions::Kind::blind,1000));check(bool(publication));
  const auto premature=box->drain();check(std::none_of(premature.begin(),premature.end(),[](const auto& p){return p.size()==20&&p[0]==65&&p[6]==108&&p[7]==1;}));
  publication();const auto packets=box->drain();check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&p[0]==65&&p[6]==108&&p[7]==1;})==5);
  check(w.apply_condition(source,ally,combat::conditions::Kind::crippled,1100,10000));check(w.has_condition(ally,combat::conditions::Kind::crippled,1100));
  w.tick(1000+plan.duration);for(unsigned id:{1u,2u,3u,100u,101u})check(!charged(w,*w.effect_scope(id),1000+plan.duration));
 }
 for(bool initially_crippled:{false,true}){
  world::Instance w;w.join(1,"Runner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto source=*w.effect_scope(1);
  if(initially_crippled)check(w.apply_condition(source,source,combat::conditions::Kind::crippled,100,10000));
  w.steer(1,1,0,1000);w.tick(2000);const auto before=w.position(1,2000).first;check(std::abs(before-(initially_crippled?144.:288.))<.1);
  check(w.stage_charge_allies(source,*charge(0),2000));w.tick(3000);const auto boosted=w.position(1,3000).first;check(std::abs(boosted-before-288.*1.33)<.1);
  check(w.apply_condition(source,source,combat::conditions::Kind::crippled,3000,10000));w.tick(4000);const auto slowed=w.position(1,4000).first;check(std::abs(slowed-boosted-288.*1.33*.5)<.1);
  check(w.stage_charge_allies(source,*charge(0),4000));w.tick(5000);check(std::abs(w.position(1,5000).first-slowed-288.*1.33)<.1);
 }
 // A cleanse must retain the normal Fragility reaction and cannot buff a corpse.
 {world::Instance w;w.join(1,"Warrior",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{30,1000,100,100},100);
  w.join(3,"Mesmer",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{500,1000,100,100},100);
  const auto source=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);check(w.degeneration_hex(foe,ally,*degeneration_hex(19,12),100));check(w.apply_condition(foe,ally,combat::conditions::Kind::crippled,200,10000));
  check(w.resources(2)->health==13&&w.stage_charge_allies(source,*charge(12),1000));check(w.resources(2)->health==0&&!charged(w,ally,1000)&&charged(w,source,1000));}
 // Exact NPC recipient primitive:condition cleanse is not a healing event.
 for(unsigned rank=0;rank<=20;++rank){auto n=creature(100,combat::npcs::Relation::allied_to_players);combat::npcs::Lifetime life{1,1,1,146,100};combat::npcs::LiveActor live(*n.combat_definition,life);const Scope caster{1,1,1};
  check(live.apply_condition(life,combat::conditions::Kind::crippled,1,100,10000));check(live.apply_condition(life,combat::conditions::Kind::blind,1,100,10000));
  check(live.remove_condition(life,combat::conditions::Kind::crippled,1000));const auto change=live.apply_charge(life,caster,*charge(rank),10,1000);check(change&&change->accepted&&live.movement_multiplier(life,1000)==1.33f);
  check(live.has_condition(life,combat::conditions::Kind::blind,1000)&&!live.has_condition(life,combat::conditions::Kind::crippled,1000));
  const auto again=live.apply_charge(life,caster,*charge(rank),11,1100);check(again&&again->removed.size()==1&&again->removed[0].instance==10&&live.movement_multiplier(life,1100)==1.33f);
  check(live.apply_condition(life,combat::conditions::Kind::crippled,1,1200,10000));check(std::abs(live.movement_multiplier(life,1200)-.665f)<.0001);
 }
 std::cout<<"PASS108 Charge World rank/all-ally versus party/range/allegiance,lifetime/future preflight,deferred publication,expiry,new Crippled,Fragility death and NPC motion/renewal;normal casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
