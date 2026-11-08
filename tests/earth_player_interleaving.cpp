// Native World regression for authoritative Earth contact/condition ordering.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Earth player interleaving line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 effects::Scope source,target;
 explicit Fixture(unsigned hp,float x=100){
  w.join(1,"Earth",0,0,box,0,{}, {},100,1);
  w.join(2,"Recipient",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{1000,1000,100,100},100);w.set_vitals(2,{hp,1000,100,100},100);
  source=*w.effect_scope(1);target=*w.effect_scope(2);
  check(w.combat_armor(target,80));check(w.elemental_armor(target,80,80));
  box->drain();
 }
};
unsigned attributed(const std::vector<world::Packet>& packets,unsigned skill){
 const auto marker=combat::detail::integer(10,1,skill);
 return unsigned(std::count(packets.begin(),packets.end(),marker));
}
int main()try{
 const auto plan=*effects::caster_earth_aoe(937,12);
 // Canonical rings install Weakness, Cracked Armor, then Blind. At AR80 the
 // first hit is36; rank12 Fragility17 must kill the remaining14 immediately.
 // The old final-condition batch instead delivered a second937 hit first.
 {
  Fixture f(50);
  check(plan.strikes[0].condition==effects::Condition::weakness&&
        plan.strikes[1].condition==effects::Condition::cracked_armor&&
        plan.strikes[2].condition==effects::Condition::blind);
  check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),100));
  f.box->drain();
  check(f.w.shockwave(f.source,plan,20,100));
  const auto packets=f.box->drain();
  check(f.w.resources(2)->health==0);
  check(attributed(packets,937)==1&&attributed(packets,19)==1);
  check(!f.w.has_condition(f.target,combat::conditions::Kind::cracked_armor,100));
  check(!f.w.has_condition(f.target,combat::conditions::Kind::blind,100));
  f.w.tick(101);check(f.w.resources(2)->health==0&&attributed(f.box->drain(),937)==0);
 }
 // Each actual hit must rebuild conditioned armor. Cracked Armor is installed
 // after the second hit, so the third uses AR60:36+36+51=123, not36*3.
 {
  Fixture f(1000);check(f.w.shockwave(f.source,plan,20,100));
  check(f.w.resources(2)->health==877);
  check(attributed(f.box->drain(),937)==3);
  for(const auto kind:{combat::conditions::Kind::weakness,combat::conditions::Kind::cracked_armor,combat::conditions::Kind::blind})
   check(f.w.has_condition(f.target,kind,100));
 }
 // Interleaved condition reactions are damage contacts too. Reduced-zero
 // contacts advance1399 but do not consume307 or cause positive-loss feedback.
 for(int reversal:{-1,0,12}){
  Fixture f(500);check(f.w.combat_armor(f.target,60));check(f.w.elemental_armor(f.target,60,60));
  check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),100));
  check(f.w.status_mutation(f.target,1399,800,12,100,6100,true));
  if(reversal>=0)check(f.w.status_mutation(f.target,307,801,unsigned(reversal),100,8100,true));
  f.box->drain();check(f.w.shockwave(f.source,plan,20,100));
  const unsigned expected=reversal<0?385:reversal==0?415:477;
  check(f.w.resources(2)->health==expected);
  const auto packets=f.box->drain();std::vector<unsigned> order;
  for(const auto& packet:packets){
   if(packet==combat::detail::integer(10,1,937))order.push_back(937);
   else if(packet==combat::detail::integer(10,1,19))order.push_back(19);
  }
  check(order==(reversal==12?std::vector<unsigned>({19,937,937}):std::vector<unsigned>({937,19,937,937})));
  const auto status=*f.w.status_snapshot(f.target,100);
  check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;}));
  check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==1399;}));
  check(f.w.before_spell_effect(f.source,42,1,100));
  check(f.w.energy_drain(f.source,f.target,*effects::energy_drain(42,12),20,100,nullptr,1));
  check(f.w.resources(2)->health==expected-37); // seventh1399contact:72-35
 }
 // Keep the monk outside Shockwave's322 radius but within166 of its victim.
 // Every actual positive loss must heal before the next condition/contact.
 for(bool reversal:{false,true}){
  Fixture f(500,160);check(f.w.combat_armor(f.target,60));check(f.w.elemental_armor(f.target,60,60));
  f.w.join(3,"Monk",323,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  f.w.set_vitals(3,{500,1000,100,100},100);const auto monk=*f.w.effect_scope(3);
  check(f.w.party_action(2,158,3,100)&&f.w.party_action(3,154,2,100));
  const auto seed=*effects::healing_seed(12);
  check(f.w.complete_healing_seed(monk,f.target,seed,0,100));
  check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),100));
  check(f.w.status_mutation(f.target,1399,800,12,100,6100,true));
  if(reversal)check(f.w.status_mutation(f.target,307,801,12,100,8100,true));
  f.box->drain();check(f.w.shockwave(f.source,plan,20,100));
  const auto triggers=reversal?3u:4u;
  check(f.w.resources(2)->health==(reversal?477u:385u)+triggers*seed.healing);
  check(f.w.resources(3)->health==500+triggers*seed.healing);
  const auto packets=f.box->drain();
  const auto first_earth=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,937));
  const auto first_fragility=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,19));
  const auto first_heal=std::find(packets.begin(),packets.end(),skills::particles::health_change_2022(3,3,seed.healing,1000));
  check(first_earth!=packets.end()&&first_fragility!=packets.end()&&first_heal!=packets.end());
  check(reversal?first_fragility<first_heal&&first_heal<first_earth:first_earth<first_heal&&first_heal<first_fragility);
 }
 // Local projection predicts first-condition death without external healing.
 // Real Seed feedback keeps this recipient alive; fresh commits must then
 // continue to the third hit, rather than replaying that short preview.
 // The fatal third hit still heals the nearby monk, but cannot resurrect
 // the damaged player. No third Fragility contact is allowed.
 {
  Fixture f(50,160);
  f.w.join(3,"Monk",323,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  f.w.set_vitals(3,{500,1000,100,100},100);const auto monk=*f.w.effect_scope(3);
  check(f.w.party_action(2,158,3,100)&&f.w.party_action(3,154,2,100));
  const auto seed=*effects::healing_seed(12);
  check(f.w.complete_healing_seed(monk,f.target,seed,0,100));
  check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),100));
  f.box->drain();check(f.w.shockwave(f.source,plan,20,100));
  check(f.w.resources(2)->health==0);
  check(f.w.resources(3)->health==500+5*seed.healing);
  const auto packets=f.box->drain();check(attributed(packets,937)==3&&attributed(packets,19)==2);
  check(f.w.player_death_times({2})[0]==100);
 }
 std::cout<<"PASS player Earth first-condition death, current armor, interleaved1399/307/Fragility and Healing Seed feedback\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
