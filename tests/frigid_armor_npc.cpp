#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Frigid Armor NPC line "+std::to_string(at.line()));
}
int main()try{
 using namespace combat;using namespace combat::npcs;using namespace skills::effects;
 const Lifetime life{1,1,1,146,100};
 Definition d;d.binding.actor=100;d.binding.map=146;d.spawn_health=Health{1000,1000};
 d.damageable=d.conditions_allowed=true;d.relation=Relation::hostile_to_players;
 d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=60;
 const auto plan=*frigid_armor(12);
 for(const auto kind:{Kind::slashing,Kind::piercing,Kind::earth,Kind::fire,Kind::cold,Kind::lightning,Kind::holy,Kind::armor_ignoring}){
  LiveActor base(d,life),protected_actor(d,life);
  check(protected_actor.apply_frigid_armor(life,plan,500,100));
  check(protected_actor.final_physical_armor(life,100)==60+int(plan.physical_armor));
  check(protected_actor.final_physical_armor(life,100+plan.duration)==60);
  const auto normal=base.apply_damage(life,{kind,100,20,1},101);
  const auto reduced=protected_actor.apply_damage(life,{kind,100,20,1},101);
  check(normal.reason==Reason::accepted&&reduced.reason==Reason::accepted);
  const bool physical=kind==Kind::slashing||kind==Kind::piercing;
  check(std::abs(reduced.applied-normal.applied*(physical?std::exp2(-double(plan.physical_armor)/40):1))<.001);
 }
 LiveActor actor(d,life);check(actor.begin_external_health(life,100));
 check(actor.apply_frigid_armor(life,plan,500,100));
 const auto before=actor.actor(life)->health->current;
 check(!actor.apply_condition(life,conditions::Kind::burning,1,200,1000));
 check(actor.health_clock(life)==100&&actor.actor(life)->health->current==before);
 check(!actor.has_condition(life,conditions::Kind::burning,200));
 check(actor.apply_condition(life,conditions::Kind::blind,1,200,1000));
 check(actor.apply_condition(life,conditions::Kind::burning,1,100+plan.duration,1000));
 LiveActor transfer(d,life);check(transfer.apply_frigid_armor(life,plan,500,100));
 conditions::State source;check(source.apply(conditions::Kind::burning,1,100,5000));
 check(!transfer.receive_conditions(life,*condition_transfer(154),source,1,200));
 check(!transfer.has_condition(life,conditions::Kind::burning,200));
 check(transfer.remove_reviewed_enchantment(life,500,200));
 check(transfer.apply_condition(life,conditions::Kind::burning,1,200,1000));
 // Elemental packets may carry damage and Burning in one proposal. Frigid
 // Armor rejects only the condition; the accepted damage still settles.
 LiveActor elemental_target(d,life);
 check(elemental_target.apply_frigid_armor(life,plan,500,100));
 const auto fire=elemental_target.apply_elemental(life,*elemental(191),20,1,101);
 check(fire&&fire->accepted&&fire->hits.size()==1);
 check(elemental_target.actor(life)->health->current<1000);
 check(!elemental_target.has_condition(life,conditions::Kind::burning,101));
 check(bool(elemental_target.apply_elemental(life,*elemental(191),20,1,100+plan.duration)));
 check(elemental_target.has_condition(life,conditions::Kind::burning,100+plan.duration));
 {
  auto definition=d;definition.healing_allowed=true;definition.healing_evidence="authored zero slices";
  LiveActor target(definition,life);const auto scope=target.hex_scope();
  check(target.stage_shield_absorption(life,12,100,700,100));
  check(target.apply_reversal_fortune(life,0,100,701,100));
  const auto result=target.apply_elemental(life,*elemental(89,0),20,1,100,true);
  check(result&&result->hits.size()==2&&result->hits[0].damage==0&&result->hits[1].damage==0);
  check(target.owned_effect_snapshot(life)->newest_skill(scope,307,100).has_value());
  const auto next=target.apply_elemental(life,*elemental(194,0),20,1,100,true);
  check(next&&next->hits[0].damage==5&&target.actor(life)->health->current==1000);
  check(!target.owned_effect_snapshot(life)->newest_skill(scope,307,100));
 }
 {
  LiveActor target(d,life);check(target.stage_shield_absorption(life,12,100,700,100));
  const auto only_condition=target.apply_elemental(life,*elemental(884,0),20,1,100,true);
  check(only_condition&&only_condition->hits.empty()&&target.has_condition(life,conditions::Kind::burning,100));
  const auto next=target.apply_damage(life,{Kind::armor_ignoring,100,20,194},100);
  check(next.reason==Reason::accepted&&next.damage==95);
 }
 for(unsigned skill:{194u,89u})for(bool reversal:{false,true}){
  auto definition=d;definition.healing_allowed=true;definition.healing_evidence="authored elemental protection";
  LiveActor target(definition,life);const auto scope=target.hex_scope();
  check(target.stage_shield_absorption(life,12,100,700,100));
  if(reversal)check(target.apply_reversal_fortune(life,0,100,701,100));
  const auto skill_plan=*elemental(skill,12);
  const auto result=target.apply_elemental(life,skill_plan,20,1,100,true);check(result&&result->accepted);
  double hp=1000;bool conversion=reversal;unsigned slice=0;
  for(double raw:skill==89?std::vector<double>{skill_plan.damage,skill_plan.shadow_above_half}:std::vector<double>{skill_plan.damage}){
   const auto amount=std::max(0.,raw-5*++slice);
   check(result->hits[slice-1].damage==amount);
   const auto converted=conversion&&amount>0?std::min(15.,amount):0.;
   if(converted>0){hp=std::min(1000.,hp+converted);conversion=false;}
   hp=std::max(0.,hp-(amount-converted));
  }
  check(target.actor(life)->health->current==hp);
  const auto next=target.apply_damage(life,{Kind::armor_ignoring,100,20,194},100);
  check(next.reason==Reason::accepted&&next.damage==100-5*(slice+1));
 }
 {
  auto definition=d;definition.spawn_health=Health{50,1000};definition.fire_armor=400;
  LiveActor target(definition,life);
  check(target.apply_condition(life,conditions::Kind::deep_wound,1,100,10000));
  const auto hit=target.apply_elemental(life,*elemental(191,0),20,1,100,true);
  check(hit&&target.actor(life)->health->current==0&&hit->hits[0].killed&&hit->hits[0].health_settlement);
  check(!target.has_condition(life,conditions::Kind::burning,100));
  check(!damage_packets_2022(1,hit->hits[0]).empty());
 }
 std::cout<<"Frigid Armor NPC physical-only damage, exact expiry, direct Burning no-side-effect rejection, transfer rejection and removal passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
