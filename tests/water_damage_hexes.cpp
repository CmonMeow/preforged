#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Water hex batch line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source;
 Fixture(unsigned skill,unsigned unknown=0,unsigned primary_health=1000){
  const auto plan=*skills::effects::movement_hex(skill,12);const float center=plan.caster_center?0.f:100.f,boundary=center+plan.radius;
  std::vector<world::Instance::Npc> population;
  for(unsigned id=100;id<=106;++id){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==101?boundary:id==102?boundary+1:100;n.plane=id==104?1:0;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.relation=id==103?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(id==106?10:id==100?primary_health:1000),1000};d.damageable=id!=103;d.conditions_allowed=id!=103&&id!=105;d.healing_allowed=true;d.healing_evidence="authored Reversal admission fixture";
   if(id!=unknown&&id!=102){d.cold_armor=60;d.cold_armor_evidence="test";}n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id=1;id<=5;++id){
   const float x=id==1?0:id==2?100:id==3?boundary:id==4?boundary+1:110;
   w.join(id,"Player",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:id==3?3:2);
   w.set_vitals(id,{1000,1000,100,100},100);
   check(w.elemental_armor(*w.effect_scope(id),{},id==unknown?std::nullopt:std::optional<int>{60}));
  }
  source=*w.effect_scope(1);box->drain();
 }
 skills::effects::Scope scope(unsigned id){return *w.effect_scope(id);}
 double health(unsigned id){return id>=100?w.npc_combat_snapshot(scope(id))->health->current:double(w.resources(id)->health);}
};
int main()try{
 using namespace skills;using namespace effects;
 // Reaction/status publication stays behind the completion prefix and is
 // idempotent even when the completion owner retries its callback.
 {Fixture f(211);std::vector<world::Packet> out;std::function<void()> publish;
  const auto plan=*movement_hex(211,12);
  check(f.w.water_damage_hex(f.source,f.scope(2),plan,20,1000,&out,&publish));
  check(publish&&f.box->drain().empty()&&out.empty());
  publish();check(!f.box->drain().empty());publish();check(f.box->drain().empty());
 }
 for(bool reversal:{false,true}){Fixture f(211);const auto target=f.scope(2);
  check(f.w.status_mutation(target,1399,80,12,100,6100,true));
  if(reversal){f.w.set_vitals(2,{1,1000,100,100},100);check(f.w.status_mutation(target,307,81,12,100,8100,true));}
  const auto plan=*movement_hex(211,12);check(f.w.water_damage_hex(f.source,target,plan,20,1000));
  check(f.health(2)>0);const auto entries=f.w.status_snapshot(target,1000);
  check(entries&&std::any_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==211&&e.instance!=0;}));
  const auto before=f.health(2);check(f.w.water_damage_hex(f.source,target,plan,20,1001));
  check(f.health(2)==before-(plan.damage-10));
 }
 {Fixture f(211,0,1);const auto target=f.scope(100);
  check(f.w.stage_npc_shield_absorption(target,12,100));check(f.w.stage_npc_reversal(target,12,100));
  const auto plan=*movement_hex(211,12);check(f.w.water_damage_hex(f.source,target,plan,20,1000));
  check(f.health(100)>0&&(f.w.npc_combat_snapshot(target)->effects&0x800));
 }
 // Legacy cosmetic-vector callers cannot defer authoritative damage behind
 // immediately published reactions. Every returned packet is a body visual.
 {Fixture f(211);std::vector<world::Packet> cosmetics;
  check(f.w.water_damage_hex(f.source,f.scope(2),*movement_hex(211,12),20,1000,&cosmetics));
  check(!cosmetics.empty()&&std::all_of(cosmetics.begin(),cosmetics.end(),[](const auto& p){return p.size()>1&&p[0]==159&&p[1]==0;}));
  const auto feedback=f.box->drain();check(std::any_of(feedback.begin(),feedback.end(),[](const auto& p){return p.size()>1&&p[0]==162&&p[1]==0;}));
 }
 // Literal pinned2022 normalization of the frozen targeted receipts.
 check(particles::water_hex_target_2022(100,1,211)==world::Packet{159,0,20,0,0,0,100,0,0,0,1,0,0,0,119,1,0,0});
 check(particles::water_hex_target_2022(100,1,212)==world::Packet{159,0,20,0,0,0,100,0,0,0,1,0,0,0,121,1,0,0});
 check(particles::water_hex_target_2022(100,1,234)==world::Packet{159,0,20,0,0,0,100,0,0,0,1,0,0,0,158,1,0,0});
 check(*casting::success_body_effect(1,212)==world::Packet{158,0,21,0,0,0,1,0,0,0,120,1,0,0});
 check(*casting::success_body_effect(1,234)==world::Packet{158,0,21,0,0,0,1,0,0,0,157,1,0,0});
 auto hex=[](Fixture& f,unsigned id,std::uint64_t at){return id>=100?bool(f.w.npc_combat_snapshot(f.scope(id))->effects&0x800):!f.w.status_snapshot(f.scope(id),at)->empty();};
 for(unsigned skill:{211u,212u,234u})for(unsigned rank=0;rank<=20;++rank){
  const auto p=movement_hex(skill,rank);check(p&&p->multiplier==.34f&&p->damage==damage_scale(skill==211?20:10,skill==211?80:85,rank));
  check(p->duration==1000ull*interpolate(skill==211?2:skill==212?3:10,skill==211?6:skill==212?8:10,rank));
  check(p->radius==(skill==211?166:skill==212?252:322)&&p->caster_center==(skill==212));
 }
 for(unsigned skill:{211u,212u,234u})for(unsigned rank:{0u,12u,20u})for(bool npc:{false,true}){
  Fixture f(skill);const auto plan=*movement_hex(skill,rank);const auto selected=plan.caster_center?f.source:f.scope(npc?100:2);std::vector<world::Packet> out;
  check(f.w.water_damage_hex(f.source,selected,plan,20,1000,&out));
  for(unsigned id:{2u,3u,100u,101u,105u,106u}){
   check(std::abs(f.health(id)-(id==106?0:1000-plan.damage))<1e-7);
   check(hex(f,id,1000)==(id!=106));
   const auto particle=particles::water_hex_target_2022(id,1,skill);check(std::count(out.begin(),out.end(),particle)==1);
  }
  for(unsigned id:{1u,4u,5u,102u,103u,104u})check(f.health(id)==1000&&!hex(f,id,1000));
  f.w.tick(1000+plan.duration);check(!hex(f,2,1000+plan.duration)&&!hex(f,100,1000+plan.duration));
 }
 for(unsigned skill:{211u,212u,234u}){
  Fixture f(skill,101);const auto plan=*movement_hex(skill,12);std::vector<world::Packet> out{{1,2}};
  check(!f.w.water_damage_hex(f.source,plan.caster_center?f.source:f.scope(100),plan,20,1000,&out)&&out.empty());
  check(f.health(2)==1000&&f.health(100)==1000&&!hex(f,2,1000));
  auto forged=plan;forged.radius=5000;check(!f.w.water_damage_hex(f.source,f.scope(100),forged,20,1000));
 }
 for(unsigned skill:{211u,212u,234u}){
  Fixture f(skill);f.w.set_vitals(2,{10,1000,100,100},100);f.w.set_vitals(3,{900,1000,100,100},100);
  check(f.w.degeneration_hex(f.scope(3),f.scope(2),*degeneration_hex(99,12),500));
  const auto plan=*movement_hex(skill,12);check(f.w.water_damage_hex(f.source,plan.caster_center?f.source:f.scope(2),plan,20,1000));
  check(f.health(2)==0&&f.health(3)==1000-plan.damage);
 }
 for(unsigned skill:{211u,234u}){
  Fixture f(skill);check(f.w.hex_breaker(f.scope(2),*hex_breaker(12),20,500));
  const auto plan=*movement_hex(skill,12);check(f.w.water_damage_hex(f.source,f.scope(2),plan,20,1000));
  check(f.health(2)==1000-plan.damage&&!hex(f,2,1000)&&hex(f,3,1000));
 }
 for(unsigned skill:{211u,234u}){
  Fixture f(skill);f.w.set_vitals(1,{10,1000,100,100},100);check(f.w.hex_breaker(f.scope(2),*hex_breaker(12),20,500));
  const auto plan=*movement_hex(skill,12);check(f.w.water_damage_hex(f.source,f.scope(2),plan,20,1000));
  check(f.health(1)==0&&f.health(2)==1000-plan.damage&&!hex(f,2,1000));
  check(f.health(3)==1000-plan.damage&&f.health(100)==1000-plan.damage&&hex(f,100,1000));
 }
 for(unsigned skill:{211u,212u,234u})for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  Fixture f(skill,mode==6?101:0);const auto plan=*movement_hex(skill,12);const auto selected=plan.caster_center?f.source:f.scope(npc?100:2);
  casting::Hooks h;unsigned commits=0,payments=0;bool stale=false;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){++payments;return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==selected.actor)return std::optional<Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==selected.actor&&range==1248;};
  h.target_valid=[&](unsigned id,float,bool self){return self&&id==1;};
  if(mode!=4)h.movement_hex=[&](auto target,const auto& value,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{particles::water_hex_target_2022(100,1,skill)}};
   std::vector<world::Packet> out;std::function<void()> publication;
   const bool ok=f.w.water_damage_hex(f.source,target,value,20,at,&out,&publication);
   return {ok,std::move(out),{},std::move(publication),true};
  };
  Slots bar{};bar[0]={skill,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[skill]=1;attributes::Ranks ranks{};ranks[11]=12;
  casting::Runtime cast(1);check(cast.start(1000,{skill,0,npc?100u:2u},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(payments==unsigned(mode!=4)&&f.w.resources(1)->energy==100-(mode==4?0:find_skill(skill)->energy_cost()));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  const auto complete=1000+std::uint64_t(find_skill(skill)->activation*1000);
  cast.update(complete-1,h);check(!commits);cast.update(complete,h);check(commits==unsigned(mode==0||mode>=5)&&payments==1);
  check(f.health(100)==(mode==0?1000-plan.damage:1000));
  const auto packets=f.box->drain();const auto particle=particles::water_hex_target_2022(100,1,skill);
  check(std::count(packets.begin(),packets.end(),particle)==unsigned(mode==0));
  if(mode==0){const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));const auto effect=std::find(packets.begin(),packets.end(),particle);
   world::Packet modifier{162,0};combat::detail::put(modifier,16);combat::detail::put(modifier,100);combat::detail::put(modifier,1);combat::detail::put(modifier,std::bit_cast<std::uint32_t>(float(-plan.damage/1000)));
   const auto hit=std::find(packets.begin(),packets.end(),modifier);check(finish<effect&&hit!=packets.end());check(skill==211?effect<hit:hit<effect);
   const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>5&&p[0]==65&&p[1]==0&&p[2]==2&&p[3]==0;});
   const auto player_effect=std::find(packets.begin(),packets.end(),particles::water_hex_target_2022(2,1,skill));
   check(status!=packets.end()&&finish<status&&player_effect<status);
   if(skill!=211){const auto body=*casting::success_body_effect(1,skill);check(std::count(packets.begin(),packets.end(),body)==1&&finish<std::find(packets.begin(),packets.end(),body));}
  }
 }
 std::cout<<"PASS three Water hexes mixed area damage/movement,expiry,atomic armor checks,selected Hex Breaker and42 real cast paths\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
