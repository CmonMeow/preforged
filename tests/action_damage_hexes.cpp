#include "gw_action_damage_hexes.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using namespace combat::weapon;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Action damage hex line "+std::to_string(at.line()));
}
int main()try{
 const AttackActionFacts idle{},attack{AttackTargetAction::attack,{}},
  attack_skill{AttackTargetAction::attack,skills::Slot{320,0}},queued_attack{AttackTargetAction::skill,skills::Slot{320,0}},
  spell{AttackTargetAction::skill,skills::Slot{281,0}},signet{AttackTargetAction::skill,skills::Slot{1,0}};
 for(unsigned skill:{898u,899u})for(unsigned rank=0;rank<=20;++rank){
  const auto plan=action_damage_hex(skill,rank);check(bool(plan));
  check(plan->pips==interpolate(1,3,rank)&&plan->damage==interpolate(skill==898?15:10,skill==898?75:52,rank));
  check(plan->duration==std::uint64_t(interpolate(5,skill==898?5:10,rank))*1000);
  for(const auto& facts:{idle,attack,attack_skill,queued_attack,spell,signet}){
   const bool trigger=skill==898?facts.skill.has_value():facts.kind==AttackTargetAction::attack||(facts.skill&&facts.skill->skill==320);
   const auto result=prepare_action_damage_hex(*plan,facts);check(result&&result->hex==*plan);
   check(result->initial_damage==(trigger?plan->damage:0u));
   check(result->damage_radius==(trigger&&skill==898?166.f:0.f));
  }
  for(const AttackActionFacts invalid:std::initializer_list<AttackActionFacts>{{AttackTargetAction::skill,{}},{AttackTargetAction::idle,skills::Slot{281,0}},{AttackTargetAction::attack,skills::Slot{281,0}},{AttackTargetAction::skill,skills::Slot{0,0}},{AttackTargetAction::skill,skills::Slot{281,65536}}})
   check(!prepare_action_damage_hex(*plan,invalid));
  auto forged=*plan;++forged.damage;check(!prepare_action_damage_hex(forged,spell));
  forged=*plan;++forged.pips;check(!prepare_action_damage_hex(forged,spell));
  forged=*plan;++forged.duration;check(!prepare_action_damage_hex(forged,spell));
  forged=*plan;forged.damage_radius=252;check(!prepare_action_damage_hex(forged,spell));
 }
 check(!action_damage_hex(898,21)&&!action_damage_hex(899,21)&&!action_damage_hex(31,12));
 check(action_damage_hex(898,20)->pips==4&&action_damage_hex(898,20)->damage==95);
 check(action_damage_hex(899,20)->pips==4&&action_damage_hex(899,20)->damage==66&&action_damage_hex(899,20)->duration==12000);
 const Scope caster{1,1,1},target{1,100,3};
 for(unsigned skill:{898u,899u}){
  const auto plan=*action_damage_hex(skill,12);Registry registry(target);
  const auto application=prepare_action_damage_hex_application(registry,caster,target,1,plan,spell,1000);
  check(application&&application->change.accepted&&application->change.removed.empty());
  check(registry.active(target,1000).empty()&&application->entry.expires==1000+plan.duration);
  const auto renewal=prepare_action_damage_hex_application(application->registry,caster,target,2,plan,attack,2000);
  check(renewal&&renewal->change.removed.size()==1&&renewal->change.removed[0].instance==1);
  check(!prepare_action_damage_hex_application(application->registry,caster,target,1,plan,spell,2000));
  check(!prepare_action_damage_hex_application(registry,{2,1,1},target,1,plan,spell,1000));
  check(!prepare_action_damage_hex_application(registry,caster,{1,100,4},1,plan,spell,1000));
  check(!prepare_action_damage_hex_application(registry,caster,caster,1,plan,spell,1000));
  check(!prepare_action_damage_hex_application(registry,caster,target,0,plan,spell,1000));
  check(!prepare_action_damage_hex_application(registry,caster,target,1,plan,spell,UINT64_MAX));
  const std::vector<Entry> stance{{42,caster.actor,14,12,Category::stance,100,30000}};
  const auto duration=hex_duration(skill,plan.duration,caster,stance,1000);check(bool(duration));
  const auto extended=prepare_action_damage_hex_application(registry,caster,target,1,plan,spell,1000,duration);
  check(extended&&extended->entry.expires==(skill==899?13060u:6000u));
  const auto other=hex_duration(skill==899?898:899,5000,caster,{},1000);
  check(other&&!prepare_action_damage_hex_application(registry,caster,target,1,plan,spell,1000,other));
 }
 std::cout<<"PASS Overload/Images of Remorse native rank0..20 plans,distinct skill-use/attack predicates,primary-only degeneration versus conditional area damage,forgery and unknown-action rejection;World/Runtime verification separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

