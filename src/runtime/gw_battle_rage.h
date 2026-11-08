#pragma once
#include "gw_cooldown_bar_authority.h"
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <optional>
#include <span>

namespace fw::gw::skills::effects {
struct BattleRagePlan {
 unsigned rank=0;
 Time duration=5000;
 unsigned adrenaline_cost=80;
 unsigned movement_bonus_percent=33;
 unsigned adrenaline_gain_multiplier=2;
 bool operator==(const BattleRagePlan&)const=default;
};

// One strike is 25 native units; the pinned cost80 is three strikes plus five
// units (the UI rounds this to four strikes). Runtime/World own payment/lifetime.
inline std::optional<BattleRagePlan> battle_rage(unsigned rank){
 const auto* r=find_skill(317);
 if(rank>20||!r||r->type!=3||r->profession!=1||r->attribute!=17||
    !r->elite()||r->energy_cost()!=0||r->adrenaline!=80||
    r->activation!=0||r->aftercast!=0||r->recharge!=0||
    r->duration0!=5||r->duration15!=20||r->scale0!=33||r->scale15!=33||
    r->caster_body_resource!=563||r->target_body_resource!=2077)return {};
 return BattleRagePlan{rank,1000ull*interpolate(5,20,rank)};
}

inline std::optional<bool> battle_rage_active(std::span<const Entry> entries,Time now){
 bool active=false;
 for(const auto& e:entries){
  if(e.skill!=317||e.applied>now||(e.expires&&now>=*e.expires))continue;
  if(e.kind!=Category::stance||!e.instance||!e.cause||!e.expires||!battle_rage(e.rank))return {};
  active=true;
 }
 return active;
}
inline std::optional<Entry> active_battle_rage(const Registry& registry,Scope owner,Time now){
 const auto e=registry.newest_skill(owner,317,now);
 return e&&e->cause==owner.actor&&e->kind==Category::stance&&e->instance&&e->expires&&battle_rage(e->rank)?e:std::nullopt;
}

struct BattleRageSkillUseProposal {Registry registry;std::optional<Entry> ended;};
// Only an accepted use of the exact published bar copy can end the stance.
// Unknown skill rows fail closed rather than being classified from client data.
inline std::optional<BattleRageSkillUseProposal> prepare_battle_rage_accepted_use(
 const Registry& original,Scope owner,Slot used,const CooldownBarSnapshot& bar,Time now){
 if(!original.matches(owner)||!cooldown_slot(used)||bar.published.owner!=owner||
    !bar.published.order||!bar.revision||bar.published.at>now||
    std::find(bar.bar.begin(),bar.bar.end(),used)==bar.bar.end())return {};
 const auto* row=find_skill(used.skill);
 if(!row)return {};
 auto next=original;
 const auto e=original.newest_skill(owner,317,now);
 if(e&&(e->cause!=owner.actor||e->kind!=Category::stance||!e->expires||
        !battle_rage(e->rank)))return {};
 if(e&&row->adrenaline==0&&!next.remove(owner,e->instance))return {};
 return BattleRageSkillUseProposal{std::move(next),e&&row->adrenaline==0?e:std::nullopt};
}
}
