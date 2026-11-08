#pragma once
#include "gw_cooldown_bar_authority.h"
#include "gw_skill_effect_plan.h"
#include <span>

namespace fw::gw::skills::effects {
struct BerserkerStancePlan {
 unsigned rank=0;Time duration=5000;unsigned adrenaline_numerator=3,adrenaline_denominator=2;
 bool operator==(const BerserkerStancePlan&)const=default;
};
inline std::optional<BerserkerStancePlan> berserker_stance(unsigned rank){
 const auto* r=find_skill(370);
 if(rank>20||!r||r->type!=3||r->attribute!=17||r->energy_cost()!=5||r->adrenaline||
    r->activation!=0||r->aftercast!=0||r->recharge!=20||r->duration0!=5||r->duration15!=11||
    r->scale0!=33||r->scale15!=33||r->bonus0!=50||r->bonus15!=50||
    r->caster_body_resource!=629||r->target_body_resource!=2077)return {};
 return BerserkerStancePlan{rank,1000ull*interpolate(5,11,rank)};
}
inline bool berserker_stance_active(std::span<const Entry> entries,Time now){
 for(const auto& e:entries)if(e.skill==370&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires&&berserker_stance(e.rank))return true;return false;
}
inline std::optional<Entry> active_berserker_stance(const Registry& registry,Scope owner,Time now){
 const auto e=registry.newest_skill(owner,370,now);
 return e&&e->cause==owner.actor&&e->kind==Category::stance&&berserker_stance(e->rank)?e:std::nullopt;
}
struct BerserkerSkillUseProposal {Registry registry;std::optional<Entry> ended;};
// Called only for an owner-accepted skill start after cost/adrenaline payment.
// A rejected request or an ordinary weapon attack is not an accepted skill use.
// Bar membership validates the exact copy,not just a matching numeric skill ID.
inline std::optional<BerserkerSkillUseProposal> prepare_berserker_accepted_use(const Registry& original,
 Scope owner,Slot used,const CooldownBarSnapshot& bar,Time now){
 if(!original.matches(owner)||!cooldown_slot(used)||bar.published.owner!=owner||!bar.published.order||
    !bar.revision||bar.published.at>now||std::find(bar.bar.begin(),bar.bar.end(),used)==bar.bar.end())return {};
 auto next=original;const auto e=active_berserker_stance(original,owner,now);
 if(e&&!next.remove(owner,e->instance))return {};
 return BerserkerSkillUseProposal{std::move(next),e};
}
}
