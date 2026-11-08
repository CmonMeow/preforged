#pragma once
#include "gw_shadow_damage.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
struct FeastOfCorruptionPlan {
    unsigned rank=0,damage=16,steal=8;float range=1248,radius=166;
    bool retain_dead_selected_anchor=true;
    auto operator<=>(const FeastOfCorruptionPlan&)const=default;
};
inline std::optional<FeastOfCorruptionPlan> feast_of_corruption(unsigned rank){
    const auto* row=find_skill(151);
    if(rank>20||!row||row->type!=5||!(row->special&4)||row->attribute!=7||
       row->scale0!=16||row->scale15!=80||row->bonus0!=8||row->bonus15!=40)return {};
    return FeastOfCorruptionPlan{rank,interpolate(16,80,rank),interpolate(8,40,rank)};
}
struct FeastAnchorObservation {Scope target;double x=0,y=0;std::uint16_t plane=0;bool alive=false;Time observed=0;};
struct FeastCastAnchor {Scope caster,selected;Time started=0;};
inline bool valid_feast_anchor_scope(Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
inline std::optional<FeastCastAnchor> acquire_feast_anchor(Scope caster,const FeastAnchorObservation& selected,Time now){
    if(!valid_feast_anchor_scope(caster)||!valid_feast_anchor_scope(selected.target)||caster.instance!=selected.target.instance||
       caster.actor==selected.target.actor||!selected.alive||selected.observed!=now||!std::isfinite(selected.x)||!std::isfinite(selected.y))return {};
    return FeastCastAnchor{caster,selected.target,now};
}
// Completion follows a fresh live location or a retained verified death
// observation for the original actor. Missing observations never fall back to
// a saved start position;replacement identities cannot inherit the anchor.
// World/runtime still validate caster life,range/layer,cast cancellation and
// consume completion once. This resolves only anchor identity and position.
inline std::optional<FeastAnchorObservation> resolve_feast_anchor(const FeastCastAnchor& anchor,Scope caster,
        const std::optional<FeastAnchorObservation>& selected,Time now){
    if(!valid_feast_anchor_scope(anchor.caster)||!valid_feast_anchor_scope(anchor.selected)||
       anchor.caster.instance!=anchor.selected.instance||anchor.caster.actor==anchor.selected.actor||
       caster!=anchor.caster||now<anchor.started||!selected||selected->target!=anchor.selected||
       selected->observed<anchor.started||selected->observed>now||(selected->alive&&selected->observed!=now)||
       !std::isfinite(selected->x)||!std::isfinite(selected->y))return {};
    return selected;
}
struct FeastOfCorruptionResult {combat::Health source;combat::Actor target;combat::Hit shadow;std::optional<LifeStealResult> steal;};
// Candidate per-recipient transaction,not a world/casting binding. Shadow
// damage precedes conditional life steal here;retail ordering on killing blows
// still needs capture verification. World must select adjacent foes and retain
// a dead selected actor only as an anchor,never as a living damage recipient.
inline std::optional<FeastOfCorruptionResult> feast_of_corruption_target(const FeastOfCorruptionPlan& plan,
        combat::Health source,combat::Actor target,bool hexed,unsigned level){
    const auto expected=feast_of_corruption(plan.rank);
    if(!expected||*expected!=plan||!combat::valid(source)||source.current<=0||!target.health)return {};
    const auto hit=combat::apply(target,{combat::Kind::armor_ignoring,double(plan.damage),level,151},true);
    if(hit.reason!=combat::Reason::accepted)return {};
    FeastOfCorruptionResult result{source,target,hit,{}};
    if(hexed&&!hit.killed){
        const auto transfer=steal_life_amount(plan.steal,source,*target.health,target.life_steal_reduction.value_or(target.incoming_reduction));if(!transfer)return {};
        result.source=transfer->source;result.target.health=transfer->target;result.steal=transfer;
        if(transfer->killed)result.target.effects|=0x10;
    }
    return result;
}
}
