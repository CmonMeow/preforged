#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ice Prison line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 Fixture(bool npc=false){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=true;n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc){w.join(2,"Runner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
};
int main()try{
 using namespace skills;using namespace effects;
 for(unsigned rank=0;rank<=20;++rank){const auto p=movement_hex(210,rank);check(p&&p->duration==1000ull*interpolate(8,20,rank)&&p->multiplier==.34f&&p->pips==0);}
 check(!movement_hex(210,21));
 {
  MovementEffects m;m.apply({1,1,210,0,Category::hex,100,8100});
  check(std::abs(m.at(100)-.34f)<1e-6&&std::abs(m.integral(100,1100)-340)<1e-3);
  m.apply({2,1,76,0,Category::hex,100,9100});check(std::abs(m.at(200)-.34f)<1e-6);
  m.remove(1,1100);check(m.at(1100)==.5f&&std::abs(m.integral(100,2100)-840)<1e-3);
  m.remove(2,2100);check(m.at(2100)==1);
  MovementEffects haste;haste.apply({1,1,210,0,Category::hex,100,8100});haste.apply({2,1,1043,0,Category::stance,100,3100});
  check(std::abs(haste.at(200)-.51f)<1e-6&&std::abs(haste.at(3100)-.34f)<1e-6&&haste.at(8100)==1);
 }
 {
  Fixture f;check(!f.w.movement_multiplier(2,.34f,50));
  check(f.w.movement_hex(f.source,f.target,*movement_hex(210,0),100));check(f.w.movement_multiplier(2,.34f,100));
  f.w.move(2,5000,0,100);f.w.tick(600);check(std::abs(f.w.position(2,600).first-148.96f)<.2f);
  check(f.w.remove_effects(f.target,2,*removal(301,0,0),600));f.w.tick(1600);
  check(std::abs(f.w.position(2,1600).first-436.96f)<.3f);
 }
 {
  Fixture f;check(f.w.movement_hex(f.source,f.target,*movement_hex(210,0),100));
  f.w.move(2,5000,0,100);f.w.tick(9100);check(std::abs(f.w.position(2,9100).first-(100+288*.34*8+288))<.5f);
 }
 {
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;
  const combat::npcs::Lifetime life{1,1,1,146,100};combat::npcs::LiveActor actor(d,life);
  check(actor.apply_movement_hex(life,*movement_hex(210,0),9,1,100));check(std::abs(actor.movement_multiplier(life,100)-.34f)<1e-6);
  check(actor.remove_hex_effect(life,1,1100)&&actor.movement_multiplier(life,1100)==1);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  Fixture f(npc);casting::Hooks h;unsigned commits=0,payments=0;bool stale=false;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){++payments;return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==f.target.actor)return std::optional<Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==f.target.actor&&range==1248;};
  if(mode!=4)h.movement_hex=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{*particles::targeted_body_2022(f.target.actor,1,210)}};
   std::vector<world::Packet> visuals;const bool ok=f.w.movement_hex(f.source,target,plan,at,&visuals);return {ok,std::move(visuals)};
  };
  Slots bar{};bar[0]={210,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[210]=1;attributes::Ranks ranks{};
  casting::Runtime cast(1);check(cast.start(1000,{210,0,f.target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(payments==unsigned(mode!=4)&&f.w.resources(1)->energy==(mode==4?100:90));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(2999,h);check(!commits);cast.update(3000,h);check(commits==unsigned(mode==0||mode==5)&&payments==1);
  const auto packets=f.box->drain();const auto particle=*particles::targeted_body_2022(f.target.actor,1,210);
  check(std::count(packets.begin(),packets.end(),particle)==unsigned(mode==0));
  if(mode==0)check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),particle));
 }
 std::cout<<"PASS Ice Prison rank policy,strong-snare cap,stacking,player route distance,removal/expiry,NPC curve and actual casts\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
