#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){
 if(!x)throw std::runtime_error("Breath Fire line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;skills::effects::Scope source,npc,player;
 Fixture(){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.damageable=true;d.spawn_health=combat::Health{1000,1000};d.fire_armor=60;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{1000,1000,50,50},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  source=*w.effect_scope(1);npc=*w.effect_scope(100);player=*w.effect_scope(2);
 }
};
int main()try{using namespace skills;
 for(unsigned id:{197u,1094u})for(unsigned rank=0;rank<=20;++rank){
  const auto p=*effects::fire_storm(rank,id);check(p.skill==id&&p.pulses==(id==197?10u:5u));
  auto field=*effects::FireStormField::create({1,1,1},20,10,20,0,p,1000);
  check(field.advance(1999).empty());const auto pulses=field.advance(20000);check(pulses.size()==p.pulses&&field.finished());
  for(unsigned i=0;i<p.pulses;++i)check(pulses[i].at==2000+i*1000&&pulses[i].x==10&&pulses[i].y==20);
  auto forged=p;++forged.pulses;check(!effects::FireStormField::create({1,1,1},20,0,0,0,forged,1000));
 }
 for(bool npc_target:{false,true}){
  Fixture f;const auto target=npc_target?f.npc:f.player;
  check(f.w.fire_storm(f.source,target,*effects::fire_storm(12,1094),20,1000));
  f.w.tick(1900);f.w.tick(1999);check(f.w.resources(2)->health==1000);
  f.w.tick(2000);if(f.w.resources(2)->health!=966||f.w.npc_combat_snapshot(f.npc)->health->current!=966)
   std::cerr<<"targetNpc="<<npc_target<<" player="<<f.w.resources(2)->health<<" npc="<<f.w.npc_combat_snapshot(f.npc)->health->current<<'\n';
  check(f.w.resources(2)->health==966&&f.w.npc_combat_snapshot(f.npc)->health->current==966);
  f.w.move(2,500,0,2100);f.w.tick(6000);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==830); // Five34-damage pulses at original center.
  check(f.w.resources(2)->health>=966&&f.w.resources(3)->health==1000);
  f.w.tick(6100);check(f.w.npc_combat_snapshot(f.npc)->health->current==830);
 }
 // Field pulses must use the same player damage modifiers as direct spells.
 for(unsigned mode=0;mode<4;++mode){
  Fixture f;
  if(mode&1)check(f.w.status_mutation(f.player,346,10,0,200,5000,true));
  if(mode&2)check(f.w.protective_enchantment(f.player,f.player,*regeneration::reviewed(299,12),0,200));
  check(f.w.fire_storm(f.source,f.player,*effects::fire_storm(12,1094),20,1000));
  f.w.tick(2000);
  const auto expected=34*((mode&1)?2:1)-((mode&2)?15:0);
  check(f.w.resources(2)->health==1000-expected);
 }
 // Sparse ticking must not skip lethal pulses or damage an actor twice after death.
 {
  Fixture f;f.w.set_vitals(2,{30,1000,50,50},100);check(f.w.resources(2)->health==30);
  check(f.w.fire_storm(f.source,f.player,*effects::fire_storm(12,1094),20,1000));
  f.w.tick(6000);check(f.w.resources(2)->health==0);
  check(f.w.resources(3)->health==1000);
 }
 // Independent native serializer fixture: captured world effect has actor0.
 {
  const combat::Packet expected{0xa0,0x80,0,0x40,0xb4,0xc5,0,0x40,0xf7,0x44,0,0,0,0,0,0,0xa7,4,0,0};
  check(particles::breath_fire_ground_2022(std::bit_cast<float>(0xc5b44000u),std::bit_cast<float>(0x44f74000u),0)==expected);
  auto storm=expected;storm[16]=0x5e;storm[17]=1;check(particles::fire_storm_ground_2022(std::bit_cast<float>(0xc5b44000u),std::bit_cast<float>(0x44f74000u),0)==storm);
  Fixture f;std::vector<combat::Packet> visuals{{1,2,3}};auto stale=f.player;++stale.generation;
  check(!f.w.fire_storm(f.source,stale,*effects::fire_storm(12,1094),20,1000,&visuals)&&visuals.empty());
  check(f.w.fire_storm(f.source,f.player,*effects::fire_storm(12,1094),20,1000,&visuals));
  check(visuals==std::vector<combat::Packet>{particles::breath_fire_ground_2022(60,0,0)});
  f.w.move(2,500,0,1100);check(visuals==std::vector<combat::Packet>{particles::breath_fire_ground_2022(60,0,0)});
 }
 for(unsigned id:{197u,1094u})for(bool npc_target:{false,true})for(unsigned mode=0;mode<5;++mode){
  Fixture f;const auto target=npc_target?f.npc:f.player;unsigned commits=0;std::vector<combat::Packet> emitted;
  Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[id]=1;attributes::Ranks ranks{};ranks[10]=12;
  casting::Hooks h;h.resources=[&](unsigned actor){return f.w.resources(actor);};h.events=[&](const auto& packets){emitted.insert(emitted.end(),packets.begin(),packets.end());};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.consume_overcast_energy=[&](unsigned skill,unsigned cost,auto event,auto at){return f.w.activate_overcast_energy(f.source,skill,cost,event,at);};
  h.target_scope=[&](unsigned actor){return f.w.effect_scope(actor);};h.projectile_target_valid=[&](unsigned actor,float){return actor==target.actor;};
  if(mode!=4){
   h.complete_fire_storm=[&](auto recipient,const auto& plan,auto at)->std::optional<std::vector<combat::Packet>>{
    ++commits;if(mode==3)return {};std::vector<combat::Packet> visuals;
    if(!f.w.fire_storm(f.source,recipient,plan,20,at,&visuals))return {};return visuals;
   };
  }
  casting::Runtime cast(1);check(cast.start(1000,{static_cast<std::uint16_t>(id),0,target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1500,h);cast.update(2999,h);check(!commits);cast.update(3000,h);check(commits==unsigned(mode==0||mode==3));
  f.w.tick(4000);check(f.w.npc_combat_snapshot(f.npc)->health->current==1000-(mode==0?id==197?29:34:0));
  {
   if(id==1094)check(bool(f.w.overcast_energy_snapshot(f.source))==(mode!=4));
   std::vector<combat::Packet> completed;
   for(const auto& packet:emitted)if((packet.size()==20&&packet[0]==160&&packet[1]==0x80)||(packet.size()>=14&&packet[0]==158&&(packet[2]==58||packet[2]==21)))completed.push_back(packet);
   if(mode==0)check(completed==std::vector<combat::Packet>{casting::generic(58,1,0),id==1094?particles::breath_fire_ground_2022(60,0,0):particles::fire_storm_ground_2022(60,0,0)});
   else check(completed.empty());
  }
 }
 std::cout<<"PASS Breath Fire five pulses,fixed mixed-population center,ally exclusion and Fire Storm/Breath player/NPC cast cancellation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
