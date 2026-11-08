#pragma once
#include "gw_cooldown_bar_authority.h"
#include "gw_skill_effect_plan.h"
#include "gw_adrenaline.h"
#include "gw_for_great_justice.h"
#include "gw_control_hexes.h"

namespace fw::gw::skills::effects {
struct DefensiveStancePlan {
 unsigned rank=0,maximum_strikes=0;Time duration=1000;double block=.75;
 bool operator==(const DefensiveStancePlan&)const=default;
};
inline std::optional<DefensiveStancePlan> defensive_stance(unsigned rank){
 const auto* r=find_skill(345);
 if(rank>20||!r||r->type!=3||r->attribute!=21||r->energy_cost()!=0||r->adrenaline!=120||
    r->activation!=0||r->aftercast!=0||r->recharge!=8||r->duration0!=1||r->duration15!=5||
    r->scale0!=75||r->scale15!=75||r->bonus0!=0||r->bonus15!=4||
    r->caster_body_resource!=2077||r->target_body_resource!=618)return {};
 return DefensiveStancePlan{rank,interpolate(0,4,rank),1000ull*interpolate(1,5,rank)};
}
inline std::optional<unsigned> defensive_melee_skills(const Slots& bar){
 std::set<std::pair<unsigned,unsigned>> seen;unsigned count=0;
 for(const auto slot:bar){
  if(!slot.skill){if(slot.copy)return {};continue;}
  if(!cooldown_slot(slot)||!seen.emplace(slot.skill,slot.copy).second)return {};
  const auto& r=*find_skill(slot.skill);
  // Pinned weapon_req: generic melee b9; sword80,axe1,hammer10,
  // daggers8,scythe20. Bow2 and spear40 are ranged attacks.
  if(r.type==14&&r.weapon_requirement&&(r.weapon_requirement&~0xb9u)==0)++count;
 }
 return count;
}
enum class DefensiveEnd {expiration,replacement,removal,death,retirement};
struct DefensiveEndProposal {
 Registry registry;Entry ended;Time at=0;unsigned strikes=0;
};
// Prepare on a copy. The owner commits removal and adrenaline together under
// its lock; replay against the committed registry cannot award a second reward.
// The end-time bar must be supplied by the same lifetime's cooldown authority.
inline std::optional<DefensiveEndProposal> prepare_defensive_end(const Registry& original,Scope owner,
 unsigned effect,Time now,DefensiveEnd reason,std::optional<CooldownBarSnapshot> bar,bool alive){
 const auto e=original.lookup(owner,effect);
 if(!e||e->skill!=345||e->cause!=owner.actor||e->kind!=Category::stance||!e->expires||
    now<e->applied||!defensive_stance(e->rank))return {};
 const auto plan=*defensive_stance(e->rank);
 if(e->applied>UINT64_MAX-plan.duration||*e->expires!=e->applied+plan.duration)return {};
 const bool retired=reason==DefensiveEnd::death||reason==DefensiveEnd::retirement;
 if(reason!=DefensiveEnd::expiration&&reason!=DefensiveEnd::replacement&&reason!=DefensiveEnd::removal&&!retired)return {};
 if(reason==DefensiveEnd::expiration&&now<*e->expires)return {};
 const auto at=retired?now:std::min(now,*e->expires);
 unsigned strikes=0;
 if(alive&&!retired){
  if(!bar||bar->published.owner!=owner||!bar->published.order||!bar->revision||bar->published.at>at)return {};
  const auto count=defensive_melee_skills(bar->bar);if(!count)return {};
  strikes=std::min(*count,plan.maximum_strikes);
 }
 auto next=original;if(!next.remove(owner,effect))return {};
 return DefensiveEndProposal{std::move(next),*e,at,strikes};
}
struct DefensiveRewardProposal {
 Registry registry;combat::adrenaline::State adrenaline;CooldownBarAuthority authority;
 Entry ended;Time at=0;unsigned strikes=0;bool forbidden=false;
};
// This preparation cannot apply an expired reward to a newer set of pools.
// World must settle the deadline before publishing a later bar or pool event.
inline std::optional<DefensiveRewardProposal> prepare_defensive_reward(const Registry& original,
 const combat::adrenaline::State& pools,const CooldownBarAuthority& authority,Scope owner,
 unsigned effect,Time now,DefensiveEnd reason,bool alive){
 const auto e=original.lookup(owner,effect);if(!e||!e->expires)return {};
 const auto observed=authority.observed_bar(owner,effect,*e->expires);
 if(!observed)return {};
 const auto end=prepare_defensive_end(original,owner,effect,now,reason,observed,alive);if(!end)return {};
 const auto current=authority.snapshot(owner);
 const bool retired=reason==DefensiveEnd::death||reason==DefensiveEnd::retirement;
 if(!retired&&(!current||current->published.order!=observed->published.order||pools.cursor()>end->at))return {};
 auto next=pools;auto clock=authority;
 if(!clock.release_bar_observation(owner,effect))return {};
 const bool forbidden=alive&&!retired&&adrenaline_forbidden(original.active(owner,end->at),end->at);
 if(end->strikes&&!forbidden)next.gain_halves(25ull*end->strikes*adrenaline_half_rate(original,owner,end->at),end->at);
 return DefensiveRewardProposal{end->registry,std::move(next),std::move(clock),end->ended,end->at,end->strikes,forbidden};
}

}
