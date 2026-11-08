#pragma once
#include "gw_backfire.h"
#include "gw_life_stealing.h"
#include "gw_hex_duration.h"
namespace fw::gw::skills::effects {
struct SoulLeechPlan {
 unsigned rank=0,amount=16;Time duration=10000;
 unsigned energy=10;Time activation=2000,aftercast=750,recharge=15000;
 float range=1248;
 auto operator<=>(const SoulLeechPlan&)const=default;
};
inline std::optional<SoulLeechPlan> soul_leech(unsigned rank){
 const auto* r=find_skill(128);
 if(rank>20||!r||r->profession!=4||r->attribute!=4||r->type!=4||r->special!=4||
    r->energy_cost()!=10||r->health_sacrifice||r->activation!=2.f||r->aftercast!=.75f||r->recharge!=15||
    r->scale0!=16||r->scale15!=80||r->duration0!=10||r->duration15!=10||
    r->caster_body_resource!=2077||r->target_body_resource!=237)return {};
 return SoulLeechPlan{rank,interpolate(16,80,rank)};
}
struct SoulLeechApplication {Registry registry;Applied change;Entry entry;};
// Copy-only proposal. Caller owns hostility, settled resources, Hex Breaker,
// source lifetime and all application reactions before committing this state.
inline std::optional<SoulLeechApplication> prepare_soul_leech_application(const Registry& registry,
 Scope caster,Scope target,unsigned effect,const SoulLeechPlan& plan,Time now,
 std::optional<HexDuration> adjusted={}){
 const auto canonical=soul_leech(plan.rank);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(caster)||!valid(target)||caster.instance!=target.instance||
    caster.actor==target.actor||!registry.matches(target)||!effect||registry.lookup(target,effect))return {};
 const auto duration=adjusted?adjusted->for_hex(128,plan.duration,caster.actor):std::optional(plan.duration);
 if(!duration||!*duration||now>UINT64_MAX-*duration)return {};
 const Entry entry{effect,caster.actor,128,plan.rank,Category::hex,now,now+*duration};
 auto proposed=registry;auto change=proposed.apply(target,entry,Replace::same_skill);
 if(!change.accepted)return {};
 return SoulLeechApplication{std::move(proposed),std::move(change),entry};
}
struct SoulLeechReaction {Entry hex;SoulLeechPlan plan;};
// Pure observation only. World must reserve an activation event, resolve the
// original caster lifetime and commit life stealing before the spell effect.
// Looking at a bar slot or starting/cancelling a cast is not a trigger.
inline std::optional<SoulLeechReaction> soul_leech_reaction(const Registry& registry,
 Scope owner,unsigned completed_spell,Time now){
 const auto* spell=find_skill(completed_spell);
 if(!spell||!is_spell(*spell))return {};
 const auto hex=registry.newest_skill(owner,128,now);
 if(!hex||hex->kind!=Category::hex||!hex->cause||hex->cause==owner.actor)return {};
 const auto plan=soul_leech(hex->rank);if(!plan)return {};
 return SoulLeechReaction{*hex,*plan};
}
struct SoulLeechOwnership {Scope caster,target;unsigned effect=0;Time applied=0;};
struct SoulLeechTransfer {SoulLeechOwnership owner;std::uint64_t event=0;Time at=0;LifeStealResult health;};
// Pure preflight. World must reserve event exactly once and pass its current
// resolved actor scopes, not infer source generation from the hex cause ID.
inline std::optional<SoulLeechTransfer> prepare_soul_leech_transfer(const Registry& registry,
 const SoulLeechOwnership& owner,Scope live_caster,Scope live_target,unsigned spell,
 std::uint64_t event,Time now,const combat::Health& caster_health,
 const combat::Health& target_health,double reduction=0){
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!event||!owner.effect||!valid(owner.caster)||!valid(owner.target)||
    owner.caster.instance!=owner.target.instance||owner.caster.actor==owner.target.actor||
    live_caster!=owner.caster||live_target!=owner.target)return {};
 const auto reaction=soul_leech_reaction(registry,live_target,spell,now);
 if(!reaction||reaction->hex.instance!=owner.effect||reaction->hex.cause!=owner.caster.actor||
    reaction->hex.applied!=owner.applied)return {};
 const auto transfer=steal_life_amount(reaction->plan.amount,caster_health,target_health,reduction);
 if(!transfer)return {};
 return SoulLeechTransfer{owner,event,now,*transfer};
}
}
