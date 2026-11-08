#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_cooldown_bar_authority.h"
#include <span>
namespace fw::gw::skills::effects {
struct WaryStancePlan {
 unsigned rank=0,energy_reward=5;
 Time duration=1000;
 // Catalog35 is interpreted as shared native adrenaline units. Retail
 // block-delta capture is still needed to verify that interpretation.
 unsigned raw_adrenaline_parameter=35;
 bool operator==(const WaryStancePlan&)const=default;
};
inline std::optional<WaryStancePlan> wary_stance(unsigned rank){
 const auto* r=find_skill(377);
 if(rank>20||!r||r->type!=3||r->attribute!=21||r->energy_cost()!=10||r->adrenaline||
    r->activation!=0||r->aftercast!=0||r->recharge!=10||r->duration0!=1||r->duration15!=6||
    r->scale0!=5||r->scale15!=5||r->bonus0!=35||r->bonus15!=35||
    r->caster_body_resource!=636||r->target_body_resource!=2077)return {};
 return WaryStancePlan{rank,5,1000ull*interpolate(1,6,rank),35};
}
// A trusted contact carries its actual skill. Zero identifies an ordinary
// attack. Native PetAttack activates a pet's next attack rather than making
// the pet's contact an attack-skill activation (historical wiki exception).
inline std::optional<bool> wary_attack_skill(unsigned skill){
 if(!skill)return false;
 const auto* row=find_skill(skill);if(!row)return {};
 return row->type==14;
}
inline std::optional<bool> wary_would_block(std::span<const Entry> entries,unsigned attack_skill,Time now){
 const auto eligible=wary_attack_skill(attack_skill);if(!eligible)return {};
 bool active=false;
 for(const auto& e:entries)if(e.skill==377&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires){
  if(!wary_stance(e.rank))return {};active=true;
 }
 return active&&*eligible;
}
inline std::optional<Entry> active_wary_stance(const Registry& registry,Scope owner,Time now){
 const auto e=registry.newest_skill(owner,377,now);
 return e&&e->cause==owner.actor&&e->kind==Category::stance&&wary_stance(e->rank)?e:std::nullopt;
}
inline bool wary_stance_active(std::span<const Entry> entries,Time now){
 for(const auto& e:entries)if(e.skill==377&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires&&wary_stance(e.rank))return true;return false;
}
struct WarySkillUseProposal {Registry registry;std::optional<Entry> ended;};
inline std::optional<WarySkillUseProposal> prepare_wary_accepted_use(const Registry& original,
 Scope owner,Slot used,const CooldownBarSnapshot& bar,Time now){
 if(!original.matches(owner)||!cooldown_slot(used)||bar.published.owner!=owner||!bar.published.order||
    !bar.revision||bar.published.at>now||std::find(bar.bar.begin(),bar.bar.end(),used)==bar.bar.end())return {};
 auto next=original;const auto e=active_wary_stance(original,owner,now);
 if(e&&!next.remove(owner,e->instance))return {};
 return WarySkillUseProposal{std::move(next),e};
}
}
