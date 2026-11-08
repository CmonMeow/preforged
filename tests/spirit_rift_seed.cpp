// Detached regression for whole-pulse Spirit Rift/Seed admission and feedback.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Rift Seed line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
struct Fixture{
 world::Instance w;effects::Scope source,early,later,monk,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,unsigned later_hp,bool early_seed){
  world::Instance::Npc body;body.id=body.definition.id=100;body.x=910;
  body.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition definition;
  definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health={500,1000};
  definition.damageable=true;definition.healing_allowed=allowed;
  if(allowed.has_value())definition.healing_evidence="synthetic Rift Seed fixture";
  body.combat_definition=definition;w.configure_npcs({body});
  for(unsigned id:{1u,2u,3u,4u}){
   // Pulse center600: later760 isinside166; monk920 isoutsidepulse butinside
   // later's Seed166. NPC910 isnear later, outside earlier's Seed radius.
   w.join(id,"Actor",id==1?0:id==2?600:id==3?760:920,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?1000u:id==3?later_hp:500u,1000,100,100},100);
   check(w.lightning_armor(*w.effect_scope(id),60));
  }
  source=*w.effect_scope(1);early=*w.effect_scope(2);later=*w.effect_scope(3);
  monk=*w.effect_scope(4);npc=*w.effect_scope(100);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
  check(w.complete_healing_seed(monk,later,*effects::healing_seed(12),0,100));
  if(early_seed)check(w.complete_healing_seed(monk,early,*effects::healing_seed(12),0,100));
  for(const auto target:{early,later})check(w.status_mutation(target,1399,800,12,100,6100,true));
 }
 void release(){
  std::vector<world::Packet> visuals;std::function<void()> publish;
  check(w.spirit_rift(source,early,*effects::spirit_rift(12),20,1000,&visuals,&publish));
  check(visuals.size()==1&&bool(publish));w.skill_event(1,visuals,1000);publish();
  w.tick(3900);box->drain();
 }
};
int main()try{
 const auto plan=*effects::spirit_rift(12);const auto seed=*effects::healing_seed(12);
 const auto fragility=*effects::fragility(2998,12);
 check(plan.radius>=160&&plan.radius<320&&seed.adjacent>=160&&seed.adjacent<310);
 const double rift_loss=std::max(0.,plan.damage-5.);
 // A late recipient's unknown Seed NPC must reject the entire released pulse,
 // not merely skip its own hit after earlier targets were damaged.
 {
  Fixture f(std::nullopt,500,false);f.release();f.w.tick(4000);
  check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.resources(4)->health==500);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500);
  const auto packets=f.box->drain();
  check(std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,910))==packets.end());
  check(!f.w.has_condition(f.early,combat::conditions::Kind::cracked_armor,4000)&&!f.w.has_condition(f.later,combat::conditions::Kind::cracked_armor,4000));
  f.w.tick(4100);check(f.w.resources(2)->health==500);
  const auto burn=*effects::energy_drain(42,12);
  check(f.w.before_spell_effect(f.source,42,1,4100));
  check(f.w.energy_drain(f.source,f.early,burn,20,4100,nullptr,1));
  check(f.w.resources(2)->health==500-(burn.amount*burn.damage_per_energy-5));
 }
 // Earlier damage heals the later target BEFORE its turn. Its own Seed then
 // keeps it alive for Cracked Armor -> single-target PvP Fragility2998 -> Seed.
 // PvE19 is deliberately avoided: its area application would also hex allies.
 const unsigned hp=unsigned(rift_loss)-seed.healing+1;
 check(hp<rift_loss&&hp+seed.healing>rift_loss);
 for(bool allowed:{false,true}){
  Fixture f(allowed,hp,true);
  check(f.w.degeneration_hex(f.source,f.later,*effects::degeneration_hex(2998,12),100));
  f.release();f.w.tick(4000);
  const double reaction_loss=std::max(0.,double(fragility.damage)-10.);
  check(f.w.resources(2)->health==unsigned(500-rift_loss+3*seed.healing));
  check(f.w.resources(3)->health==unsigned(hp+3*seed.healing-rift_loss-reaction_loss));
  check(f.w.resources(4)->health==500+2*seed.healing);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500+(allowed?2*seed.healing:0));
  check(f.w.has_condition(f.early,combat::conditions::Kind::cracked_armor,4000)&&f.w.has_condition(f.later,combat::conditions::Kind::cracked_armor,4000));
  const auto packets=f.box->drain();std::vector<unsigned> order;
  const auto damage=combat::detail::integer(10,1,910);
  const auto heal=skills::particles::health_change_2022(3,4,seed.healing,1000);
  const auto reaction=combat::detail::integer(10,1,2998);
  const auto condition_skill=combat::conditions::skills[unsigned(combat::conditions::Kind::cracked_armor)];
  for(const auto& packet:packets){
   if(packet==damage)order.push_back(1);
   else if(packet==heal)order.push_back(2);
   else if(packet.size()==20&&packet[0]==65&&word(packet,2)==3&&(unsigned(packet[6])|(unsigned(packet[7])<<8))==condition_skill)order.push_back(3);
   else if(packet==reaction)order.push_back(4);
  }
  check(order==std::vector<unsigned>({1,2,1,2,3,4,2}));
  const auto later_health=f.w.resources(3)->health;f.w.tick(4100);check(f.w.resources(3)->health==later_health);
 }
 std::cout<<"PASS Rift whole-pulse Seed refusal, shared recipient survival, immunity and Cracked/Fragility feedback order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
