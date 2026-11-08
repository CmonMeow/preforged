#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Water Trident World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source;
 Fixture(unsigned unknown=0,bool crowded=false,unsigned dying_npc=0){
  std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?650:800;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(id==dying_npc?1:1000),1000};d.damageable=true;d.conditions_allowed=true;
   if(id!=unknown){d.cold_armor=60;d.cold_armor_evidence="test";}n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id:{1u,2u,3u,4u,5u}){
   const float x=id==1?0:id==2?600:id==3?766:id==4?767:610;
   w.join(id,"Player",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
   w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),{},id==unknown?std::nullopt:std::optional<int>{60}));
  }
  if(crowded){w.join(6,"Additional foe",620,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(6,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(6),{},60));}
  source=*w.effect_scope(1);box->drain();
 }
 double hp(unsigned id){return id>=100?w.npc_combat_snapshot(*w.effect_scope(id))->health->current:double(w.resources(id)->health);}
 bool cast(unsigned rank=15,std::uint64_t at=1000,std::vector<world::Packet>* out=nullptr){return w.water_projectile(source,*w.effect_scope(2),*skills::effects::water_projectile(237,rank),20,at,out);}
};
int main()try{
 for(unsigned rank:{0u,12u,15u,20u}){
  Fixture f;std::vector<world::Packet> out;check(f.cast(rank,1000,&out)&&out.size()==3);
  check(f.hp(2)==1000&&f.hp(3)==1000&&f.hp(100)==1000);f.w.tick(1400);
  const auto damage=skills::effects::damage_scale(10,90,rank);
  for(unsigned id:{2u,3u,100u})check(std::abs(f.hp(id)-(1000-damage))<(id<100?1:1e-7));
  for(unsigned id:{1u,4u,5u,101u})check(f.hp(id)==1000);
  for(unsigned id:{2u,3u})check(!f.w.knocked_down(*f.w.effect_scope(id),1400));
 }
 for(unsigned unknown:{3u,100u}){
  Fixture f(unknown);std::vector<world::Packet> out{{1,2}};check(!f.cast(15,1000,&out)&&out.empty());f.w.tick(2000);
  for(unsigned id:{2u,3u,100u})check(f.hp(id)==1000); // Batch rejects before registering any flight.
 }
 for(unsigned dying:{3u,100u}){
  Fixture f(0,false,dying==100?100:0);
  if(dying==3)f.w.set_vitals(3,{1,1000,100,100},100);
  check(f.w.degeneration_hex(f.source,*f.w.effect_scope(dying),*skills::effects::degeneration_hex(31,15),200));
  std::vector<world::Packet> out;check(f.cast(15,1000,&out)&&out.size()==2);
  check(f.hp(dying)==0);f.w.tick(1400);check(f.hp(2)==910&&f.hp(dying==3?100:3)==910);
 }
 {
  Fixture f(0,true);std::vector<world::Packet> out;check(f.cast(15,1000,&out)&&out.size()==3);f.w.tick(1400);
  for(unsigned id:{2u,6u,100u})check(f.hp(id)==910);
  check(f.hp(3)==1000); // Cap is three including selected; explicit nearest-first tie policy.
 }
 {
  Fixture f(4);check(f.cast());f.w.leave(2);f.w.tick(1400);
  check(f.hp(3)==910&&f.hp(100)==910); // Selected departure does not cancel secondary flights.
 }
 {
  Fixture f;check(f.cast());check(f.w.kill(f.source,1100));f.w.tick(1400);
  for(unsigned id:{2u,3u,100u})check(f.hp(id)==910);
 }
 for(bool stop:{false,true}){
  Fixture f;f.w.move(2,900,0,900);std::vector<world::Packet> out;check(f.cast(15,1000,&out));
  auto number=[&](unsigned offset){unsigned bits=0;for(unsigned i=0;i<4;++i)bits|=unsigned(out[0][offset+i])<<(8*i);return std::bit_cast<float>(bits);};
  const auto at=1002+static_cast<std::uint64_t>(std::llround(number(16)*1000));
  if(stop)f.w.move(2,number(6),number(10),1000); // Stop on the fixed aim instead of dodging it.
  f.w.tick(at);check(f.hp(2)==910);
  check(f.w.knocked_down(*f.w.effect_scope(2),at)==!stop);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  Fixture f;skills::casting::Hooks h;unsigned payments=0,commits=0;bool stale=false;
  const auto selected=npc?100u:2u;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){check(cost==5);++payments;return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){return stale&&id==selected?std::optional<skills::effects::Scope>{}:f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==selected&&range==1248;};
  if(mode!=4)h.water_projectile=[&](auto target,const auto& plan,auto at)->skills::casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{{163,0}}};std::vector<world::Packet> out;
   const bool ok=f.w.water_projectile(f.source,target,plan,20,at,&out);return {ok,std::move(out)};
  };
  skills::Slots bar{};bar[0]={237,0};skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[237]=1;
  attributes::Ranks ranks{};ranks[11]=15;skills::casting::Runtime cast(1);
  check(cast.start(1000,{237,0,selected},bar,policy,ranks,true,h)==(mode==4?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
  check(payments==unsigned(mode!=4));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(commits==unsigned(mode==0||mode==5)&&payments==1);
  const auto packets=f.box->drain();auto launch=std::find_if(packets.begin(),packets.end(),[](const auto& p){return !p.empty()&&p[0]==163;});
  check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return !p.empty()&&p[0]==163;})==(mode==0?3:0));
  const auto body=*skills::casting::success_body_effect(1,237);check(std::count(packets.begin(),packets.end(),body)==unsigned(mode==0));
  if(mode==0)check(std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1))<std::find(packets.begin(),packets.end(),body)&&std::find(packets.begin(),packets.end(),body)<launch);
  f.w.tick(2500);check(f.hp(selected)==(mode==0?910:1000));
 }
 std::cout<<"PASS Water Trident independent flights,mixed targets,adjacent boundary,atomic rejection,moving-only knockdown and casting\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
