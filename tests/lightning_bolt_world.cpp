#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Lightning Bolt World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;world::Instance::Npc npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 skills::effects::Scope source,target;
 Fixture(bool is_npc=true,bool armor=true,unsigned health=1000,bool guardian=false,float target_x=200){
  npc.id=npc.definition.id=100;npc.x=target_x;npc.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{double(health),1000};d.damageable=true;d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="test";
  if(armor){d.lightning_armor=80;d.lightning_armor_evidence="test";}npc.combat_definition=d;
  auto placed=npc;if(guardian){placed.id=placed.definition.id=101;placed.x=400;placed.combat_definition->binding.actor=placed.combat_definition->binding.definition=101;placed.combat_definition->relation=combat::npcs::Relation::neutral;}
  w.configure_npcs({placed});w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!is_npc){w.join(2,"Foe",target_x,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{health,1000,100,100},100);check(w.lightning_armor(*w.effect_scope(2),armor?std::optional<int>{80}:std::nullopt));}
  source=*w.effect_scope(1);target=*w.effect_scope(guardian?101:is_npc?100:2);check(w.configure_projectiles({100,32}));box->drain();
 }
 double health(){return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(target.actor)->health);}
};
int main()try{
 using namespace skills;
 for(bool pvp:{false,true})for(bool npc:{false,true})for(unsigned mode=0;mode<9;++mode){
  if(npc&&(mode==6||mode==7))continue;
  Fixture f(npc,true,mode==8?20:1000,false,!npc&&mode==5?600.f:200.f);casting::Hooks h;unsigned releases=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& batch){f.w.skill_event(1,batch);};
  h.consume_energy_at=[&](auto cost,auto at){check(cost==5);return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float){return id==f.target.actor;};
  if(mode!=3)h.lightning=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   std::vector<world::Packet> visuals;const bool ok=f.w.lightning(f.source,target,plan,20,at,&visuals);if(ok)++releases;return {ok,std::move(visuals)};};
  Slots bar{};bar[0]={1369,0};Policy policy;policy.primary=6;policy.pvp_match=pvp;policy.campaigns=255;policy.learned[1369]=1;attributes::Ranks ranks{};ranks[8]=12;
  casting::Runtime cast(1);check(cast.start(1000,{1369,0,f.target.actor},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==3){check(f.w.resources(1)->energy==100);continue;}
  check(f.w.resources(1)->energy==95);
  if(mode==1)cast.cancel(h,1500);if(mode==2)cast.interrupt(1500,h);
  cast.update(2000,h);const bool released=mode!=1&&mode!=2;check(releases==unsigned(released));
  check(f.health()==(mode==8?20:1000));
  auto packets=f.box->drain();const auto impact=particles::lightning_bolt_impact_2022(f.target.actor,1);
  check(std::find(packets.begin(),packets.end(),impact)==packets.end());
  if(released){
   const auto flight=particles::lightning_orb_launch_2022(1,!npc&&mode==5?600.f:200.f,0,0,!npc&&mode==5?.25f:.084f);

   check(std::count(packets.begin(),packets.end(),flight)==1);
   check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),flight));
   check(std::count(packets.begin(),packets.end(),*casting::skill_success_body_effect(1,1369))==1);
  }
  if(mode==4){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},100,1);f.w.set_vitals(1,{1000,1000,100,100},100);}
  if(!npc&&mode==5)f.w.move(2,600,500,2001); // At impact it has left the hit bubble.
  if(!npc&&mode==6)f.w.move(2,200,500,2070); // Still inside fixed aim, but moving at impact.
  if(!npc&&mode==7)f.w.move(2,200,1,2001); // Movement ended before impact.
  f.w.tick(!npc&&mode==5?2300:2100);const bool hit=released&&mode!=4&&(npc||mode!=5);
  check(f.health()==(mode==8?0:hit?(mode==6?918:959):1000));
  packets=f.box->drain();check(std::count(packets.begin(),packets.end(),impact)==unsigned(hit));
  if(hit)check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,1369))==unsigned(mode==6?2:1));
  f.w.tick(!npc&&mode==5?2400:2200);packets=f.box->drain();check(std::find(packets.begin(),packets.end(),impact)==packets.end());
  if(mode==0){
   check(cast.start(2200,{1369,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(2750,{1369,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);
   check(cast.start(9999,{1369,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);
   check(cast.start(10000,{1369,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  }
 }
 for(bool stopped:{false,true}){
  Fixture f(true,true,1000,true);const auto guard=*f.w.effect_scope(101);
  auto enemy=f.npc;enemy.combat_definition->knockdown_allowed=true;enemy.combat_definition->knockdown_evidence="fixture";
  check(f.w.begin_wave_objective(f.source,guard,73,1,3,1)&&f.w.spawn_quest_wave(f.source,73,1,{enemy},[]{return true;}));
  const auto victim=*f.w.effect_scope(60000);
  check(f.w.lightning(f.source,victim,*effects::lightning(1369,12),20,2000));
  check(f.w.chase_quest_guardian(victim,100,2070));
  if(stopped)check(f.w.knockdown_npc(victim,2000,2080)==combat::npcs::control::Knockdown::accepted);
  f.box->drain();f.w.tick(2100);
  check(f.w.npc_combat_snapshot(victim)->health->current==(stopped?959:918));
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,1369))==(stopped?1:2));
 }
 for(bool stopped:{false,true})for(unsigned hp:{10u,1000u})for(unsigned rr:{0u,12u,20u}){
  Fixture f(true,true,1000,true);const auto guard=*f.w.effect_scope(101);auto enemy=f.npc;
  enemy.combat_definition->spawn_health={double(hp),1000};enemy.combat_definition->knockdown_allowed=true;enemy.combat_definition->knockdown_evidence="test";
  check(f.w.begin_wave_objective(f.source,guard,73,1,3,1)&&f.w.spawn_quest_wave(f.source,73,1,{enemy},[]{return true;}));
  const auto victim=*f.w.effect_scope(60000);check(f.w.stage_npc_reversal(victim,rr,2000));
  check(f.w.lightning(f.source,victim,*effects::lightning(1369,12),20,2000)&&f.w.chase_quest_guardian(victim,100,2070));
  if(stopped)check(f.w.knockdown_npc(victim,2000,2080)==combat::npcs::control::Knockdown::accepted);
  f.box->drain();f.w.tick(2100);const double converted=std::min(41.,double(effects::reversal_fortune(rr)->maximum_healing));
  double expected=std::max(0.,std::min(1000.,hp+converted)-(41-converted));if(!stopped&&expected>0)expected=std::max(0.,expected-41);
  check(std::abs(f.w.npc_combat_snapshot(victim)->health->current-expected)<1e-7);
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),particles::lightning_bolt_impact_2022(victim.actor,1))==1);
  check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67;})==1);
 }
 for(bool npc:{false,true}){Fixture f(npc,false);check(!f.w.lightning(f.source,f.target,*effects::lightning(1369,12),20,1000));check(f.health()==1000);}
 std::cout<<"PASS Lightning Bolt Runtime/World player and NPC flight,late movement bonus,stopped motion,miss,lethal,cancel,interrupt,missing hook,reused caster and unknown armor\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
