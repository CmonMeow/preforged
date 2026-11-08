#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Lightning Orb line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;world::Instance::Npc npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 skills::effects::Scope source,target;
 Fixture(bool is_npc=true,bool armor=true,unsigned health=1000){
  npc.id=npc.definition.id=100;npc.x=200;npc.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{double(health),1000};d.damageable=true;d.conditions_allowed=true;
  if(armor){d.lightning_armor=80;d.lightning_armor_evidence="test";}npc.combat_definition=d;
  w.configure_npcs({npc});w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!is_npc){w.join(2,"Foe",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{health,1000,100,100},100);check(w.lightning_armor(*w.effect_scope(2),armor?std::optional<int>{80}:std::nullopt));}
  source=*w.effect_scope(1);target=*w.effect_scope(is_npc?100:2);check(w.configure_projectiles({100,32}));box->drain();
 }
 double health(){return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(target.actor)->health);}
};
int main()try{
 using namespace skills;
 const world::Packet literal{159,0,20,0,0,0,100,0,0,0,1,0,0,0,148,1,0,0};
 check(particles::lightning_orb_impact_2022(100,1)==literal);
 const world::Packet captured_launch{163,128,25,0,0,0,0,64,180,197,0,64,247,68,0,0,92,147,38,61,147,1,0,0,1,0,0,0,0};
 check(particles::lightning_orb_launch_2022(25,-5768,1978,0,std::bit_cast<float>(0x3d26935cu))==captured_launch);
 for(bool npc:{false,true})for(unsigned mode=0;mode<9;++mode){
  if(npc&&mode==8)continue; // Player dodge exercises shared fixed-aim flight;NPC movement has separate authored ownership.
  Fixture f(npc,true,mode==5||mode==7?10:1000);casting::Hooks h;unsigned commits=0;bool stale=false;
  h.resources=[&](unsigned actor){return f.w.resources(actor);};h.events=[&](const auto& values){f.w.skill_event(1,values);};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned actor){auto scope=f.w.effect_scope(actor);if(stale&&actor==f.target.actor)return std::optional<effects::Scope>{};return scope;};
  h.projectile_target_valid=[&](unsigned actor,float range){return actor==f.target.actor&&range==1248;};
  if(mode!=4)h.lightning=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;std::vector<world::Packet> visuals;const bool accepted=f.w.lightning(f.source,target,plan,20,at,&visuals);
   return {accepted,std::move(visuals)};
  };
  Slots bar{};bar[0]={229,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[229]=1;attributes::Ranks ranks{};ranks[8]=12;
  casting::Runtime cast(1);check(cast.start(1000,{229,0,f.target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==4){check(f.w.resources(1)->energy==100);continue;}
  check(f.w.resources(1)->energy==85);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(2999,h);check(!commits);cast.update(3000,h);check(commits==unsigned(mode==0||mode>=5));
  const auto before=f.health();check(before==(mode==5||mode==7?10:1000));
  const auto launch=f.box->drain();const auto impact=particles::lightning_orb_impact_2022(f.target.actor,1);
  check(std::find(launch.begin(),launch.end(),impact)==launch.end());
  const auto flight=particles::lightning_orb_launch_2022(1,200,0,0,2.f);
  const bool released=mode==0||mode>=5;
  check(std::count(launch.begin(),launch.end(),flight)==unsigned(released));
  if(released)check(std::find(launch.begin(),launch.end(),casting::terminal(58,1))<std::find(launch.begin(),launch.end(),flight));
  if(mode==6){f.w.leave(1);f.w.join(1,"Reused caster",0,0,f.box,0,{}, {},100,1);f.w.set_vitals(1,{1000,1000,100,100},100);check(*f.w.effect_scope(1)!=f.source);}
  if(mode==7)check(f.w.shadow_damage(f.source,f.target,*effects::shadow_damage(133,20),20,3500));
  if(mode==8)f.w.move(2,200,500,3200);
  f.w.tick(4900);check(f.health()==(mode==7?0:before));f.w.tick(5100);
  const bool hit=mode==0||mode==5;check(f.health()==(mode==5||mode==7?0:hit?918:1000));
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),impact)==unsigned(hit));
  if(hit){const auto effect=std::find(packets.begin(),packets.end(),impact);
   const auto damage=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,229));check(damage!=packets.end()&&effect<damage);
  }
  f.w.tick(6100);const auto again=f.box->drain();check(std::find(again.begin(),again.end(),impact)==again.end());
 }
 for(bool npc:{false,true}){
  Fixture f(npc,false);check(!f.w.lightning(f.source,f.target,*effects::lightning(229,12),20,1000));check(f.health()==1000);
 }
 {
  Fixture f;check(f.w.spawn_kill_encounter(f.source,85,1,{f.npc},[]{return true;}));const auto target=*f.w.effect_scope(60000);
  check(f.w.lightning(f.source,target,*effects::lightning(229,12),20,1000));check(f.w.clear_kill_objective(f.source,85,1,true));
  check(!f.w.effect_scope(target.actor));f.box->drain();f.w.tick(3100);
  const auto packets=f.box->drain();const auto impact=particles::lightning_orb_impact_2022(target.actor,1);check(std::find(packets.begin(),packets.end(),impact)==packets.end());
 }
 {
  Fixture f;auto enemy=f.npc;enemy.combat_definition->spawn_health=combat::Health{10,1000};
  check(f.w.spawn_kill_encounter(f.source,85,1,{enemy},[]{return true;}));const auto target=*f.w.effect_scope(60000);
  check(f.w.lightning(f.source,target,*effects::lightning(229,12),20,1000));f.w.tick(3100);
  check(f.w.npc_combat_snapshot(target)->health->current==0);
  check(f.w.pending_quest_completions(f.source)==std::vector<world::Instance::QuestCompletion>{{85,1}});
  check(f.w.take_npc_deaths(f.source).size()==1);f.w.tick(4100);check(f.w.take_npc_deaths(f.source).empty());
 }
 std::cout<<"PASS Lightning Orb delayed player/NPC damage,impact wire/order,cancel,interrupt,stale/reused caster,dead/retired targets,dodge and unknown armor\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
