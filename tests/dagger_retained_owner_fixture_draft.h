#pragma once
// Test owning connection adapter. Actual production Runtime owns payment and
// completion; World owns both physical events. This creates no fee receipt,
// event ledger, synthetic QualifiedHit or alternate dual attack implementation.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_character_skills.h"
#include <stdexcept>
namespace fw::gw::test {
class RetainedDaggerOwnerDraft {
 world::Instance& world_;skills::effects::Scope source_,target_{};
 std::shared_ptr<world::Outbox> box_;skills::casting::Runtime runtime_;
 skills::casting::Hooks hooks_;skills::Policy policy_;skills::Slots bar_{};
 attributes::Ranks ranks_{};unsigned skill_=0;std::uint64_t now_=0,use_=0;
public:
 unsigned payments=0,paid=0,completions=0,completed_observers=0;
 combat::weapon::AttackPermit permit;
 combat::weapon::DaggerPaidCompletionDraft first_result,second_result;
 skills::Profile durable;
 RetainedDaggerOwnerDraft(world::Instance& w,skills::effects::Scope source,std::shared_ptr<world::Outbox> box):world_(w),source_(source),box_(std::move(box)),runtime_(source.actor){
  hooks_.events=[this](const auto& packets){box_->enqueue(packets);};
  hooks_.resources=[this](unsigned id){return world_.resources(id);};
  hooks_.target_scope=[this](unsigned id){return world_.effect_scope(id);};
  hooks_.consume_energy_at=[this](unsigned n,auto at){const bool accepted=world_.consume_energy(source_.actor,n,source_,at);if(accepted){++payments;paid+=n;}return accepted;};
  hooks_.weapon_snapshot=[this]{return world_.attack_weapon(source_);};
  hooks_.weapon_attack_timing=[this](auto at){return world_.weapon_attack_timing(source_,at);};
  hooks_.attack_duration=[this](auto base,auto at){return world_.attack_duration(source_,base,at,true);};
  hooks_.melee_target=[this](unsigned id,auto at){return id==target_.actor&&world_.melee_target(source_,id,at,skill_);};
  hooks_.control_epoch=[this]{return world_.control_epoch(source_);};
  hooks_.attack_activity=[this](auto until){world_.attack_activity(source_,until,now_);};
  hooks_.cast_slot_activity=[this](auto slot,auto begins,auto ends){world_.cast_activity(source_,slot,begins,ends);};
  hooks_.prepare_dagger_attack=[this](auto target,const auto& gear,const auto& plan,auto began,auto release){permit=world_.prepare_dagger_cast_draft(source_,target,gear,plan,began,release);return permit;};
  hooks_.complete_dagger_attack=[this](auto target,const auto& gear,const auto& plan,const auto& attempt,auto at,unsigned copy){++completions;first_result=world_.complete_dagger_cast_draft(source_,target,gear,plan,attempt,at,copy);return first_result;};
  hooks_.complete_dagger_successful_second=[this](const auto& phase,const auto& before,auto at){second_result=world_.complete_dagger_successful_second_draft(phase,before,at);return second_result;};
  hooks_.cooldown_amendment_source=[this](auto event){return world_.cooldown_amendment_source(source_,event);};
  hooks_.complete_dagger_failed_second=[this](const auto& phase,auto at){return world_.complete_dagger_failed_second_draft(phase,at);};
  hooks_.prepare_skill_use=[this](auto slot,auto event,auto at){use_=event;return world_.prepare_diversion_skill_use(source_,slot,event,at);};
  hooks_.finalize_skill_use=[this](auto event,auto at,auto base,auto existing,bool zero){return world_.finalize_diversion_skill_use(source_,event,at,base,existing,zero);};
  hooks_.abort_skill_use=[this](auto event){return world_.abort_diversion_skill_use(source_,event);};
  hooks_.cooldown_completion_source=[this](auto event){return world_.cooldown_completion_stamp(source_,event);};
  hooks_.cooldown_control_source=[this](auto after){return world_.cooldown_control_source(source_,after);};
  hooks_.skill_disabled_slot=[this](auto slot,auto at){return world_.skill_disabled_slot(source_,slot,at);};
  hooks_.current_skillbar=[this]()->std::optional<skills::Slots>{return bar_;};
  hooks_.completed_skill_use=[this](const skills::effects::CooldownCompletion& receipt){const auto actual=world_.cooldown_completion_stamp(source_,receipt.skill_use);if(!actual||*actual!=receipt)return false;++completed_observers;return true;};
 }
 skills::casting::Reason begin(skills::effects::Scope target,unsigned skill,unsigned rank,std::uint64_t at){
  if(!combat::weapon::dagger_dual_role(skill)||rank>20||runtime_.active()||world_.effect_scope(source_.actor)!=source_)return skills::casting::Reason::policy;
  if(world_.effect_scope(target.actor)!=target)return skills::casting::Reason::target;
  target_=target;skill_=skill;now_=at;first_result={};second_result={};
  const auto identity=*combat::weapon::dagger_identity(skill);
  durable={};durable.primary=7;durable.learned[static_cast<unsigned>(identity.family)]=1;durable.bar[0]={static_cast<unsigned>(identity.family),0};
  const auto match=skills::match_profile(durable,identity.pvp);policy_=skills::policy(match,255,false,identity.pvp);bar_=match.bar;
  if(bar_[0]!=skills::Slot{skill,0}||!world_.configure_adrenaline(source_,bar_,at))return skills::casting::Reason::policy;
  ranks_={};ranks_[skills::find_skill(skill)->attribute]=rank;
  return runtime_.start(at,{skill,0,target.actor},bar_,policy_,ranks_,true,hooks_);
 }
 void update(std::uint64_t at){now_=at;runtime_.update(at,hooks_);}
 bool active()const{return runtime_.active();}
 bool execute(skills::effects::Scope target,unsigned skill,unsigned rank,std::uint64_t at){
  if(begin(target,skill,rank,at)!=skills::casting::Reason::accepted)return false;
  update(permit->release_time());const bool accepted=first_result.accepted;
  if(const auto phase=first_result.successful_second)update(phase->due());
  return accepted;
 }
 std::uint64_t final_contact_at()const{return !second_result.outcomes.empty()?second_result.outcomes.back().at:!first_result.outcomes.empty()?first_result.outcomes.back().at:now_;}
};
}
