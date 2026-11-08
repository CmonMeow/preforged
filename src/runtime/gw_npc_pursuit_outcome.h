#pragma once
#include "gw_npc_pursuit_motion.h"

namespace fw::gw::combat::ai::motion {
// Observations of one movement attempt, not a disengagement/retry policy.
// In particular, a valid detour need not reduce straight-line goal distance.
enum class OutcomeKind {
    progressing,reached,deferred,route_unavailable,unsupported_layer,
    leash_boundary,invalid_request
};
enum class Deferral {none,observation,zero_budget};
struct Outcome {
    OutcomeKind kind=OutcomeKind::invalid_request;
    double travelled=0;
    Deferral deferral=Deferral::none;
    // Compatibility with arrival checks only. A false result is not evidence
    // of an unreachable target: callers needing that fact must inspect kind.
    explicit operator bool()const{return kind==OutcomeKind::reached;}
    static Outcome deferred(Deferral reason){return {OutcomeKind::deferred,0,reason};}
};
// Call after authoritative route/time/rate observation and movement admission.
// No route remains distinct from absent observations; neither is inferred from
// zero displacement or elapsed wall time. The existing leash owner decides
// whether a clipped attempt actually broke pursuit before passing that flag.
inline Outcome describe(const Advance& step,bool pursuit_broken=false){
    if(!valid(step.point)||!std::isfinite(step.spent)||step.spent<0||
       (step.facing&&!std::isfinite(*step.facing)))return {};
    if(step.reached)return {OutcomeKind::reached,step.spent};
    if(pursuit_broken)return {OutcomeKind::leash_boundary,step.spent};
    if(step.spent>0)return {OutcomeKind::progressing,step.spent};
    return Outcome::deferred(Deferral::zero_budget);
}
}
