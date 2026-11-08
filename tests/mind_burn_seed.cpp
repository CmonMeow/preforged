// Detached whole-cohort Mind Burn admission/feedback regression.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mind Burn Seed line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& packet,unsigned at){return unsigned(packet[at])|(unsigned(packet[at+1])<<8)|(unsigned(packet[at+2])<<16)|(unsigned(packet[at+3])<<24);}
struct Fixture{
 world::Instance w;effects::Scope source,early,later,monk,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,unsigned later_hp,unsigned later_energy,bool early_seed){
  world::Instance::Npc body;body.id=body.definition.id=100;body.x=910;
  body.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition definition;
  definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health={500,1000};
  definition.damageable=true;definition.healing_allowed=allowed;
  if(allowed.has_value())definition.healing_evidence="synthetic Mind Burn Seed fixture";
  body.combat_definition=definition;w.configure_npcs({body});
  for(unsigned id:{1u,2u,3u,4u}){
   // Anchor600; later760 isinside166. Monk920/NPC910 areinside later's
   // Seed166 but outside the cast cohort and outside early's Seed166.
   w.join(id,"Actor",id==1?0:id==2?600:id==3?760:920,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   const unsigned energy=id==1?80:id==2?10:id==3?later_energy:20;
   w.set_vitals(id,{id==1?1000u:id==3?later_hp:500u,1000,energy,100},100);
   check(w.energy_capacity(id,6,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(id),60,{}));
  }
  source=*w.effect_scope(1);early=*w.effect_scope(2);later=*w.effect_scope(3);monk=*w.effect_scope(4);npc=*w.effect_scope(100);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
  check(w.complete_healing_seed(monk,later,*effects::healing_seed(12),0,100));
  if(early_seed)check(w.complete_healing_seed(monk,early,*effects::healing_seed(12),0,100));
  for(const auto target:{early,later})check(w.status_mutation(target,1399,800,12,100,6100,true));
 }
 bool cast(){
  box->drain();std::vector<world::Packet> visuals;std::function<void()> publication;
  const bool accepted=w.mind_burn(source,early,*effects::mind_burn(185,12),20,1000,&visuals,&publication);
  if(accepted){box->enqueue(visuals);if(publication)publication();}
  else check(!publication);
  return accepted;
 }
};
int main()try{
 const auto plan=*effects::mind_burn(185,12);const auto seed=*effects::healing_seed(12);
 const auto fragility=*effects::fragility(2998,12);
 check(plan.radius>=160&&plan.radius<310&&seed.adjacent>=160&&seed.adjacent<310&&fragility.damage>15);
 const double first=std::max(0.,plan.damage-5.),second=std::max(0.,plan.damage-10.);
 {
  Fixture f(std::nullopt,500,20,false);check(!f.cast());
  check(f.w.resources(2)->health==500&&f.w.resources(3)->health==500&&f.w.resources(4)->health==500);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500);
  check(!f.w.has_condition(f.early,combat::conditions::Kind::burning,1000)&&!f.w.has_condition(f.later,combat::conditions::Kind::burning,1000));
  const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,185))==packets.end());
  const auto burn=*effects::energy_drain(42,12);check(f.w.before_spell_effect(f.source,42,1,1000));
  check(f.w.energy_drain(f.source,f.early,burn,20,1000,nullptr,1));
  check(f.w.resources(2)->health==500-(std::min(burn.amount,10u)*burn.damage_per_energy-5));
 }
 // Initial later Health is insufficient for its first hit. Earlier target's
 // two Seed heals make the complete later sequence/Burning reaction possible.
 const unsigned hp=unsigned(std::ceil(first+second-2*seed.healing+1));
 check(hp<first&&hp+2*seed.healing>first+second);
 for(bool allowed:{false,true})for(unsigned later_energy:{20u,80u})for(bool frigid:{false,true}){
  if(frigid&&later_energy==80)continue;
  Fixture f(allowed,hp,later_energy,true);
  //2998 is single-target, unlike19 which would hex nearby allies too.
  check(f.w.degeneration_hex(f.source,f.later,*effects::degeneration_hex(2998,12),100));
  if(frigid){const auto armor=*effects::frigid_armor(12);
   check(f.w.status_mutation(f.later,1261,900,12,100,100+armor.duration,true));}
  check(f.cast());const bool bonus=later_energy<80;
  const bool condition=bonus&&!frigid;const unsigned own_triggers=(bonus?2u:1u)+unsigned(condition);
  const double reaction=condition?std::max(0.,double(fragility.damage)-15.):0.;
  const double later_loss=first+(bonus?second:0.)+reaction;
  check(f.w.resources(2)->health==unsigned(std::ceil(500-first-second+(2+own_triggers)*seed.healing)));
  check(f.w.resources(3)->health==unsigned(std::ceil(hp+(2+own_triggers)*seed.healing-later_loss)));
  check(f.w.resources(4)->health==500+own_triggers*seed.healing);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==500+(allowed?own_triggers*seed.healing:0));
  check(f.w.has_condition(f.early,combat::conditions::Kind::burning,1000));
  check(f.w.has_condition(f.later,combat::conditions::Kind::burning,1000)==condition);
  const auto packets=f.box->drain();std::vector<unsigned> order;
  const auto early_body=*skills::particles::targeted_body_2022(2,1,185),later_body=*skills::particles::targeted_body_2022(3,1,185);
  const auto damage=combat::detail::integer(10,1,185),heal=skills::particles::health_change_2022(3,4,seed.healing,1000);
  const auto reactive=combat::detail::integer(10,1,2998);
  const auto burning_skill=combat::conditions::skills[unsigned(combat::conditions::Kind::burning)];
  for(const auto& packet:packets){
   if(packet==early_body)order.push_back(5);else if(packet==later_body)order.push_back(6);
   else if(packet==damage)order.push_back(1);else if(packet==heal)order.push_back(2);
   else if(packet.size()==20&&packet[0]==65&&word(packet,2)==3&&(unsigned(packet[6])|(unsigned(packet[7])<<8))==burning_skill)order.push_back(3);
   else if(packet==reactive)order.push_back(4);
  }
  std::vector<unsigned> expected{5,1,2,1,2,6,1,2};
  if(bonus){expected.push_back(1);expected.push_back(2);}
  if(condition){expected.push_back(3);expected.push_back(4);expected.push_back(2);}
  check(order==expected);
 }
 // One Watch charge must advance from actual remaining damage, not raw
 // incoming damage converted by Reversal. Keep the healer outside the area.
 for(int reversal:{-1,0,20}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:id==2?100:300,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{500,1000,id==1?80u:20u,100},100);check(w.energy_capacity(id,6,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(id),60,{}));
  }
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),healer=*w.effect_scope(3);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(healer,target,seed,0,100));
  check(w.stage_watch_yourself_recipient(healer,target,*effects::watch_yourself(0,true),100));
  if(reversal>=0)check(w.status_mutation(target,307,801,unsigned(reversal),100,8100,true));
  box->drain();std::vector<world::Packet> visuals;std::function<void()> publication;
  check(w.mind_burn(source,target,plan,20,1000,&visuals,&publication));check(publication&&visuals.empty()&&box->drain().empty());
  const double first=plan.damage*std::exp2(-5./40.);
  const double converted=reversal<0?0:std::min(first,double(effects::reversal_fortune(unsigned(reversal))->maximum_healing));
  const double second=converted==first?first:plan.damage;const unsigned triggers=converted==first?1:2;
  check(std::abs(double(w.resources(2)->health)-(500+converted-(first-converted)-second+triggers*seed.healing))<1.01);
  check(w.watch_yourself_armor(target,1000)==0&&w.has_condition(target,combat::conditions::Kind::burning,1000));
  auto copied=publication;publication();const auto packets=box->drain();copied();check(box->drain().empty());
  check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(2,3,seed.healing,1000))==triggers);
 }
 std::cout<<"PASS Mind Burn whole-cohort Seed admission, per-foe Energy, Burning/Fragility ordering and Frigid immunity\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}
