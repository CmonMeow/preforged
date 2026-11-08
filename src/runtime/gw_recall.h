#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <cmath>
namespace fw::gw::skills::effects {
// Assassin Recall925, distinct from Mesmer Mantra of Recall82.
struct RecallShadowPlan {
 unsigned energy=15,upkeep=1;Time activation=1000,aftercast=750,recharge=10000,disable=10000;
 float range=1248,compass=5000;
 bool operator==(const RecallShadowPlan&)const=default;
};
inline std::optional<RecallShadowPlan> recall_shadow(){
 const auto* r=find_skill(925);
 if(!r||r->profession!=7||r->attribute!=51||r->type!=6||r->special||r->energy_cost()!=15||
    r->adrenaline||r->health_sacrifice||r->activation!=1.f||r->aftercast!=.75f||
    r->recharge!=10||r->target!=4||r->equip_type!=1||r->scale0!=10||r->scale15!=10||
    r->caster_body_resource!=2077||r->target_body_resource!=928)return {};
 return RecallShadowPlan{};
}
struct RecallAnchor {Scope caster,target;unsigned effect=0;Time applied=0;};
inline bool recall_scope(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
// The enchantment belongs to the caster. Only the destination is anchored to
// the selected ally. World supplies authoritative relationship and liveness.
inline std::optional<RecallAnchor> prepare_recall_anchor(const RecallShadowPlan& plan,
 Scope caster,Scope target,unsigned effect,Time now,bool caster_alive,bool target_alive,
 bool ally,double distance){
 const auto canonical=recall_shadow();
 if(!canonical||*canonical!=plan||!recall_scope(caster)||!recall_scope(target)||
    caster.instance!=target.instance||caster.actor==target.actor||!effect||
    !caster_alive||!target_alive||!ally||!std::isfinite(distance)||distance<0||distance>plan.range)return {};
 return RecallAnchor{caster,target,effect,now};
}
struct RecallEndAction {bool step=false;Time disabled_until=0;};
// Stateless proposal: actual movement, paid upkeep, exactly-once retirement and
// all-slot disable publication must share the World end transaction.
inline std::optional<RecallEndAction> recall_end_action(const RecallShadowPlan& plan,
 const RecallAnchor& anchor,Scope caster,Scope target,bool caster_alive,bool target_alive,Time now){
 const auto canonical=recall_shadow();
 if(!canonical||*canonical!=plan||!recall_scope(anchor.caster)||!recall_scope(anchor.target)||
    anchor.caster.instance!=anchor.target.instance||anchor.caster.actor==anchor.target.actor||
    !anchor.effect||caster!=anchor.caster||now<anchor.applied)return {};
 if(!caster_alive||target!=anchor.target||!target_alive)return RecallEndAction{};
 if(now>UINT64_MAX-plan.disable)return {};
 return RecallEndAction{true,now+plan.disable};
}
}
