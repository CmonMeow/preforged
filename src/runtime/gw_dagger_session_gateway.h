#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"

namespace fw::gw::skills::casting {
// Deliberate enrollment switch, separate from supported() and metadata coverage.
// Production remains closed until native and rendered-client enrollment review.
enum class DaggerSessionEnrollment { closed, reviewed };
inline constexpr auto dagger_session_enrollment = DaggerSessionEnrollment::closed;

// Callbacks borrow this storage: temporaries (including moved const values)
// cannot be retained. Persistent mutable and const owner storage remain valid.
inline bool bind_dagger_session_gateway(Hooks&,world::Instance&,
        std::optional<effects::Scope>&&,
        DaggerSessionEnrollment=DaggerSessionEnrollment::closed)=delete;
inline bool bind_dagger_session_gateway(Hooks&,world::Instance&,
        const std::optional<effects::Scope>&&,
        DaggerSessionEnrollment=DaggerSessionEnrollment::closed)=delete;

// Scope storage and World must outlive Hooks, just as the Session's other hooks.
// This adapter neither pays nor finalizes: those remain existing Session owners.
inline bool bind_dagger_session_gateway(Hooks& hooks,world::Instance& world,
        const std::optional<effects::Scope>& session_owner,
        DaggerSessionEnrollment enrollment=DaggerSessionEnrollment::closed) {
    if(enrollment!=DaggerSessionEnrollment::reviewed)return false;
    // Install only as a complete extension of the existing authoritative owner.
    if(!hooks.prepare_skill_use||!hooks.finalize_skill_use||!hooks.abort_skill_use||
       !hooks.cooldown_completion_source||!hooks.cooldown_control_source||
       !hooks.current_skillbar||!hooks.skill_disabled_slot||!hooks.target_scope||
       !hooks.control_epoch||!hooks.events)return false;
    auto current=[&world,&session_owner]()->std::optional<effects::Scope>{
        if(!session_owner||world.effect_scope(session_owner->actor)!=session_owner)return {};
        return session_owner;
    };
    hooks.prepare_dagger_attack=[&world,current](auto target,const auto& weapon,const auto& plan,auto began,auto release){
        const auto source=current();
        return source?world.prepare_dagger_cast_draft(*source,target,weapon,plan,began,release):combat::weapon::AttackPermit{};
    };
    hooks.complete_dagger_attack=[&world,current](auto target,const auto& weapon,const auto& plan,const auto& permit,auto now,unsigned copy){
        const auto source=current();
        return source?world.complete_dagger_cast_draft(*source,target,weapon,plan,permit,now,copy):combat::weapon::DaggerPaidCompletionDraft{};
    };
    hooks.complete_dagger_failed_second=[&world,current](const auto& phase,auto now){
        const auto source=current();
        return source&&phase&&phase->source()==*source?world.complete_dagger_failed_second_draft(phase,now):combat::weapon::DaggerPaidCompletionDraft{};
    };
    hooks.complete_dagger_successful_second=[&world,current](const auto& phase,const auto& before,auto now){
        const auto source=current();
        return source&&phase&&phase->source()==*source&&before.owner==*source?
            world.complete_dagger_successful_second_draft(phase,before,now):combat::weapon::DaggerPaidCompletionDraft{};
    };
    hooks.cooldown_amendment_source=[&world,current](auto event)->std::optional<effects::CooldownAmendment>{
        const auto source=current();return source?world.cooldown_amendment_source(*source,event):std::nullopt;
    };
    return true;
}
}
