#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_cooldown_bar_authority.h"
#include <span>
namespace fw::gw::skills::effects {
struct BonettiDefensePlan {
 unsigned rank=0,energy=5;Time duration=5000;double block=.75;
 bool operator==(const BonettiDefensePlan&)const=default;
};
inline std::optional<BonettiDefensePlan> bonetti_defense(unsigned rank){
 const auto* r=find_skill(380);
 if(rank>20||!r||r->type!=3||r->attribute!=21||r->energy_cost()!=0||r->adrenaline!=200||
    r->activation!=0||r->aftercast!=0||r->recharge!=0||r->duration0!=5||r->duration15!=11||
    r->scale0!=75||r->scale15!=75||r->bonus0!=5||r->bonus15!=5||
    r->caster_body_resource!=639||r->target_body_resource!=2077)return {};
 return BonettiDefensePlan{rank,5,1000ull*interpolate(5,11,rank)};
}
inline std::optional<Entry> active_bonetti_defense(const Registry& registry,Scope owner,Time now){
 const auto e=registry.newest_skill(owner,380,now);
 return e&&e->cause==owner.actor&&e->kind==Category::stance&&bonetti_defense(e->rank)?e:std::nullopt;
}
inline bool bonetti_defense_active(std::span<const Entry> entries,Time now){
 for(const auto& e:entries)if(e.skill==380&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires&&bonetti_defense(e.rank))return true;return false;
}
struct BonettiSkillUseProposal {Registry registry;std::optional<Entry> ended;};
// The caller must supply an accepted post-payment use, never an input request.
inline std::optional<BonettiSkillUseProposal> prepare_bonetti_accepted_use(const Registry& original,
 Scope owner,Slot used,const CooldownBarSnapshot& bar,Time now){
 if(!original.matches(owner)||!cooldown_slot(used)||bar.published.owner!=owner||!bar.published.order||
    !bar.revision||bar.published.at>now||std::find(bar.bar.begin(),bar.bar.end(),used)==bar.bar.end())return {};
 auto next=original;const auto e=active_bonetti_defense(original,owner,now);
 if(e&&!next.remove(owner,e->instance))return {};
 return BonettiSkillUseProposal{std::move(next),e};
}
}
