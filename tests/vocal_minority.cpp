#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Vocal Minority line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source;
 Fixture(unsigned unknown=0,unsigned primary_health=1000){
  std::vector<world::Instance::Npc> population;
  for(unsigned id=100;id<=106;++id){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==101?352:id==102?353:100;n.plane=id==104?1:0;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.relation=id==103?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(id==106?10:id==100?primary_health:1000),1000};d.damageable=id!=103;d.conditions_allowed=id!=103&&id!=105;
   if(id!=unknown&&id!=102){d.lightning_armor=80;d.lightning_armor_evidence="test";}n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id=1;id<=5;++id){
   const float x=id==1?0:id==2?100:id==3?352:id==4?353:110;
   w.join(id,"Player",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:id==3?3:2);
   w.set_vitals(id,{1000,1000,100,100},100);
   check(w.lightning_armor(*w.effect_scope(id),id==unknown?std::nullopt:std::optional<int>{80}));
  }
  source=*w.effect_scope(1);box->drain();
 }
 skills::effects::Scope scope(unsigned id){return *w.effect_scope(id);}
 double health(unsigned id){return id>=100?w.npc_combat_snapshot(scope(id))->health->current:double(w.resources(id)->health);}
};
int main()try{
 using namespace skills;using namespace effects;
 unsigned checks=0;
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u}){
  Fixture f;auto plan=*degeneration_hex(883,rank);std::vector<world::Packet> visuals;
  std::function<void()> publication;
  check(f.w.degeneration_hex(f.source,f.scope(npc?100:2),plan,1000,20,&visuals,&publication));
  check(bool(publication));
  for(unsigned id:{2u,3u,100u,101u,105u,106u}){
   const bool hex=id>=100?bool(f.w.npc_combat_snapshot(f.scope(id))->effects&0x800):!f.w.status_snapshot(f.scope(id),1000)->empty();
   check(hex);check(f.health(id)==(id==106?10:1000));++checks;
  }
  for(unsigned id:{1u,4u,5u,102u,103u,104u}){
   const bool hex=id>=100?bool(f.w.npc_combat_snapshot(f.scope(id))->effects&0x800):!f.w.status_snapshot(f.scope(id),1000)->empty();
   check(!hex);++checks;
  }
  // Publication is deferred and repeat calls cannot duplicate queued packets.
  check(f.box->drain().empty());publication();check(!f.box->drain().empty());publication();check(f.box->drain().empty());
  const auto expiry=1000+plan.duration;
  check(!f.w.status_snapshot(f.scope(2),expiry-1)->empty());
  f.w.tick(expiry);check(f.w.status_snapshot(f.scope(2),expiry)->empty());
  check(!(f.w.npc_combat_snapshot(f.scope(100))->effects&0x800));
 }
 {
  Fixture f;check(f.w.hex_breaker(f.scope(2),*hex_breaker(12),20,500));
  check(f.w.degeneration_hex(f.source,f.scope(2),*degeneration_hex(883,12),1000,20));
  check(f.w.status_snapshot(f.scope(2),1000)->empty());
  check(!f.w.status_snapshot(f.scope(3),1000)->empty());
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<8;++mode){
  Fixture f;const auto selected=f.scope(npc?100:2);casting::Hooks h;unsigned commits=0;bool stale=false;
  h.resources=[&](unsigned id){return f.w.resources(id);};
  h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==selected.actor)return std::optional<Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==selected.actor&&range==1248;};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return mode!=6&&f.w.before_spell_effect(f.source,skill,event,at);};
  h.degeneration_hex_completed=[&](auto target,const auto& plan,auto event,auto at)->casting::EffectCompletion{
   ++commits;std::vector<world::Packet> out;std::function<void()> publication;
   const bool ok=f.w.degeneration_hex(f.source,target,plan,at,20,&out,&publication,event);
   return {ok,std::move(out),std::nullopt,std::move(publication)};
  };
  if(mode==4)h.degeneration_hex_completed={};
  Slots slots{};slots[0]={883,0};Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[883]=1;
  attributes::Ranks ranks{};ranks[7]=12;casting::Runtime runtime(1);
  check(runtime.start(1000,{883,0,selected.actor},slots,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==(mode==4?100u:90u));if(mode==4)continue;
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1100,h);if(mode==3)stale=true;
  if(mode==5)h.degeneration_hex_completed={};if(mode==7)h.before_spell_effect={};
  runtime.update(1999,h);check(!commits);runtime.update(2000,h);if(commits!=unsigned(mode==0))std::cerr<<"npc="<<npc<<" mode="<<mode<<" commits="<<commits<<"\n";check(commits==unsigned(mode==0));
  const auto packets=f.box->drain();const auto body=casting::skill_success_body_effect(1,883);check(body.has_value());
  check(std::count(packets.begin(),packets.end(),*body)==unsigned(mode==0));
  if(mode==0){
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   const auto visual=std::find(packets.begin(),packets.end(),*body);
   check(finish!=packets.end()&&visual!=packets.end()&&finish<visual&&visual+1!=packets.end());
   runtime.update(2750,h);check(runtime.start(2750,{883,0,selected.actor},slots,policy,ranks,true,h)==casting::Reason::recharge);
  }
 }
 for(unsigned version:{181u,2860u}){
  Fixture f;f.w.public_professions(1,6,4);
  const auto renewal=*ether_renewal(version,15);
  check(f.w.status_mutation(f.source,version,80,15,100,100+renewal.duration,true));
  f.w.set_vitals(1,{500,1000,50,100},100);
  const auto event=900u+version;
  check(f.w.before_spell_effect(f.source,883,event,1000));
  std::function<void()> publication;std::vector<world::Packet> visuals;
  check(f.w.degeneration_hex(f.source,f.scope(2),*degeneration_hex(883,12),1000,20,&visuals,&publication,event));
  check(f.w.resources(1)->energy==50+renewal.energy_per_enchantment);
  check(f.w.resources(1)->health==500+renewal.healing_per_enchantment);
  check(!f.w.degeneration_hex(f.source,f.scope(2),*degeneration_hex(883,12),1000,20,nullptr,nullptr,event));
  check(f.w.resources(1)->energy==50+renewal.energy_per_enchantment);
 }
 for(unsigned skill:{2216u,1553u,132u,281u})for(bool expired:{false,true}){
  using namespace combat;using namespace combat::npcs;
  Definition d;d.binding.actor=d.binding.definition=100;d.binding.map=146;
  d.relation=Relation::hostile_to_players;d.spawn_health=Health{1000,1000};d.damageable=true;
  const Lifetime life{7,2,3,146,100};LiveActor actor(d,life);
  check(bool(actor.apply_single_degeneration_hex(life,*degeneration_hex(883,0),1,77,100)));
  const auto epoch=actor.control_epoch(life);const auto at=expired?5100u:100u;
  const auto token=actor.begin_action(life,control::Action::spell,at,at+100,Slot{skill,0});
  const bool allowed=expired||(skill!=2216&&skill!=1553);check(bool(token)==allowed);
  if(!allowed)check(actor.control_epoch(life)==epoch);
  if(token)check(actor.complete_action(life,*token,at+100));
  auto stale=life;++stale.actor_generation;
  check(!actor.begin_action(stale,control::Action::spell,at,at+100,Slot{281,0}));
 }
 std::cout<<"Vocal mixed-recipient assertions "<<checks<<"\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
