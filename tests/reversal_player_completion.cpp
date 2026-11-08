#include "gw_world_simulation.h"
#include <iostream>
#include <fstream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Reversal player completion line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 effects::Scope source,target,attacker;
 Fixture(unsigned hp,bool self=false,bool npc=false){
  if(npc){
   world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,0};d.relation=combat::npcs::Relation::allied_to_players;
   d.spawn_health={500,1000};d.healing_allowed=true;d.healing_evidence="synthetic healing ally";n.combat_definition=d;w.configure_npcs({n});
  }
  for(unsigned id:{1u,2u,3u}){
   const bool selected=id==(self?1:2);
   w.join(id,"Actor",id==3?0:100,0,id==3?box:std::make_shared<world::Outbox>(),0,
       {world::Packet{165,0,static_cast<std::uint8_t>(id),0,0,0,3,0}}, {},100,id==3?2:1);
   w.set_vitals(id,{selected?hp:1000,1000,selected?20u:100u,100},100);
   check(w.combat_armor(*w.effect_scope(id),60)&&w.elemental_armor(*w.effect_scope(id),{},60));
  }
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  source=*w.effect_scope(1);target=*w.effect_scope(self?1:2);attacker=*w.effect_scope(3);
  check(w.configure_projectiles({1000,32}));box->drain();
 }
 effects::EnchantmentDuration duration(unsigned percent,std::uint64_t at=1000,unsigned skill=307){
  inventory::Profile p;p.bags.emplace(1,inventory::Bag{1,1,0,20});
  if(percent){inventory::Item item;item.definition.id=100;item.definition.type=26;item.definition.quantity=1;
   item.definition.modifiers={0x22b80000u|percent};item.bag=1;p.items.emplace(100,item);p.weapons.sets[0].weapon=100;}
  const auto equipment=items::weapon_passives::equipped_enchanting(p,1);check(equipment.has_value());
  const auto active=w.status_snapshot(source,at);check(active.has_value());
  const auto quote=effects::enchanting_duration(skill,8000,source,*equipment,*active,at);check(quote.has_value());return *quote;
 }
};
unsigned u32(const world::Packet& p,unsigned at){unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(p.at(at+i))<<(i*8);return n;}
int main(int argc,char** argv)try{
 std::ofstream trace;if(argc==2){trace.open(argv[1]);check(bool(trace));}else check(argc==1);
 unsigned cases=0;
 for(bool self:{false,true})for(unsigned state=0;state<(self?1u:4u);++state)
 for(unsigned rank:{0u,12u,20u})for(unsigned hp:{1u,500u,1000u})for(unsigned route=0;route<4;++route)
 for(unsigned divine:{0u,12u})for(unsigned percent:{0u,20u}){
  Fixture f(hp,self);const auto duration=f.duration(percent);const auto plan=*effects::reversal_fortune(rank);
  check(f.w.complete_reversal_fortune(f.source,f.target,plan,divine,1000,&duration));
  const auto application_packets=f.box->drain();
  const auto status=*f.w.status_snapshot(f.target,1000);check(status.size()==1&&status[0].cause==f.source.actor&&status[0].expires==1000+(percent?10000:8000));
  const double before=std::min(1000u,hp+(divine?38u:0u));check(f.w.resources(f.target.actor)->health==before);
  if(state==1)check(f.w.kill(f.source,1050));
  if(state>=2)f.w.leave(1);
  if(state==3){f.w.join(1,"Reused",100,0,std::make_shared<world::Outbox>(),0,{}, {},1050,1);f.w.set_vitals(1,{1000,1000,100,100},100);check(*f.w.effect_scope(1)!=f.source);
   check(f.w.combat_armor(*f.w.effect_scope(1),60));}
  f.box->drain();double amount=72;
  if(route==0)check(f.w.energy_drain(f.attacker,f.target,*effects::energy_drain(42,12),20,1100));
  if(route==1)check(f.w.player_reversal_life_steal_recipient(f.attacker,f.target,amount,1100));
  if(route==2){const auto aoe=*effects::caster_earth_aoe(174,12);check(aoe.count==1);amount=aoe.strikes[0].earth_damage;
   const bool ok=f.w.shockwave(f.attacker,aoe,20,1100);if(!ok)std::cerr<<"AoE refused self="<<self<<" state="<<state<<" rank="<<rank<<" hp="<<hp<<" divine="<<divine<<" percent="<<percent<<'\n';check(ok);}
  if(route==3){const auto dagger=*effects::disrupting_dagger(12);amount=dagger.earth_damage;check(f.w.launch_disrupting_dagger(f.attacker,f.target,dagger,20,1100));f.w.tick(1200);}
  const double converted=std::min(amount,double(plan.maximum_healing));
  const double healed=std::min(1000.,before+converted),expected=std::max(0.,healed-(amount-converted));
  if(f.w.resources(f.target.actor)->health!=unsigned(std::ceil(expected)))std::cerr<<"self="<<self<<" state="<<state<<" rank="<<rank<<" hp="<<hp<<" route="<<route<<" divine="<<divine<<" actual="<<f.w.resources(f.target.actor)->health<<" expected="<<expected<<'\n';
  check(f.w.resources(f.target.actor)->health==unsigned(std::ceil(expected)));
  const auto after=*f.w.status_snapshot(f.target,1200);check(std::none_of(after.begin(),after.end(),[](const auto& e){return e.skill==307;}));
  const auto packets=f.box->drain();unsigned heals=0;
  for(const auto& p:packets)if(p.size()==18&&p[0]==162&&u32(p,2)==55&&u32(p,6)==f.target.actor&&std::bit_cast<float>(u32(p,14))>0){check(u32(p,10)==f.source.actor);++heals;}
  check(heals==unsigned(healed>before&&state<2));
  if(trace&&!self&&rank==12&&hp==500&&!divine&&!percent){
   trace<<"{\"state\":"<<state<<",\"route\":"<<route<<",\"target\":"<<f.target.actor<<",\"healer\":"<<f.source.actor<<",\"gained\":"<<healed-before<<",\"final_health\":"<<expected<<",\"packets\":[";
   bool first=true;for(const auto& p:packets){if(!first)trace<<',';first=false;trace<<'\"';constexpr char hex[]="0123456789abcdef";for(auto b:p)trace<<hex[b>>4]<<hex[b&15];trace<<'\"';}
   trace<<"],\"application_packets\":[";first=true;
   for(const auto& p:application_packets){if(!first)trace<<',';first=false;trace<<'\"';constexpr char hex[]="0123456789abcdef";for(auto b:p)trace<<hex[b>>4]<<hex[b&15];trace<<'\"';}trace<<"]}\n";
  }
  ++cases;
 }
 // Current scope and duration receipts are mandatory at completion. No cast
 // cost or production Runtime support is inferred from this World API.
 for(unsigned mode=0;mode<7;++mode){
  Fixture f(500);auto plan=*effects::reversal_fortune(12);auto target=f.target;auto source=f.source;
  auto duration=f.duration(20,mode==4?999:1000,mode==5?245:307);
  if(mode==0)++plan.maximum_healing;if(mode==1)++target.generation;if(mode==2)++source.generation;
  if(mode==3)target=f.attacker;if(mode==6)check(f.w.kill(target,1000));
  check(!f.w.complete_reversal_fortune(source,target,plan,0,1000,&duration));++cases;
 }
 for(unsigned percent:{0u,20u}){
  Fixture f(500);auto duration=f.duration(percent);check(f.w.complete_reversal_fortune(f.source,f.target,*effects::reversal_fortune(12),0,1000,&duration));
  const auto expires=1000+(percent?10000:8000);f.w.tick(expires);
  const auto status=*f.w.status_snapshot(f.target,expires);check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;}));++cases;
 }
 for(unsigned rank:{0u,12u,20u})for(unsigned divine:{0u,12u})for(unsigned percent:{0u,20u}){
  Fixture f(500,false,true);
  const auto target=*f.w.effect_scope(100);const auto duration=f.duration(percent);
  check(f.w.complete_reversal_fortune(f.source,target,*effects::reversal_fortune(rank),divine,1000,&duration));
  const auto entries=*f.w.status_snapshot(target,1000);check(entries.size()==1&&entries[0].cause==1&&entries[0].expires==1000+(percent?10000:8000));
  check(f.w.npc_combat_snapshot(target)->health->current==500+(divine?38:0));++cases;
 }
 // Replacing an external application with self-cast must retire its credit.
 {Fixture f(500);const auto plan=*effects::reversal_fortune(12);
  check(f.w.complete_reversal_fortune(f.source,f.target,plan,0,1000));
  check(f.w.complete_reversal_fortune(f.target,f.target,plan,0,1100));f.box->drain();
  check(f.w.player_reversal_life_steal_recipient(f.attacker,f.target,72,1200));check(f.w.resources(2)->health==562);
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(2,2,67,1000))==1);++cases;}
 // A successful spell-use receipt rewards the source once, before adding the
 // new enchantment. Self casts must rebuild after source healing instead of
 // restoring the pre-reward registry snapshot.
 for(unsigned context=0;context<3;++context)for(unsigned renewal:{181u,2860u})
 for(unsigned rank:{0u,12u})for(unsigned divine:{0u,12u}){
  Fixture f(500,context==1,context==2);const auto target=context==2?*f.w.effect_scope(100):f.target;
  f.w.set_vitals(1,{context==1?500u:100u,1000,50,100},100);
  const auto reward=*effects::ether_renewal(renewal,15);
  check(f.w.status_mutation(f.source,renewal,80,15,100,100+reward.duration,true));
  check(f.w.before_spell_effect(f.source,307,1,1000));
  const auto plan=*effects::reversal_fortune(rank);
  check(f.w.complete_reversal_fortune(f.source,target,plan,divine,1000,nullptr,1));
  const auto source_health=(context==1?500u:100u)+20+(context==1&&divine?38u:0u);
  check(f.w.resources(1)->health==source_health);
  const auto energy=f.w.resources(1)->energy;check(energy==50+reward.energy_per_enchantment);
  const auto status=*f.w.status_snapshot(target,1000);
  check(std::count_if(status.begin(),status.end(),[](const auto& e){return e.skill==307;})==1);
  check(!f.w.complete_reversal_fortune(f.source,target,plan,divine,1000,nullptr,1));
  check(f.w.resources(1)->health==source_health&&f.w.resources(1)->energy==energy);++cases;
 }
 std::cout<<cases<<" player Reversal World completion cases: self/ally, caster lifetime, conversion routes, Divine Favor, duration, expiry and invalid receipts passed; Runtime307 unbound\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
