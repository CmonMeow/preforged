#pragma once
#include "gw_conditions.h"
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <algorithm>
namespace fw::gw::skills::effects {
struct MelandruResiliencePlan {
 unsigned rank=0;Time duration=8000;
 bool operator==(const MelandruResiliencePlan&)const=default;
};
inline std::optional<MelandruResiliencePlan> melandru_resilience(unsigned rank){
 const auto* r=find_skill(451);
 if(rank>20||!r||r->profession!=2||r->attribute!=24||r->type!=3||r->special!=4||
    r->energy_cost()!=5||r->adrenaline||r->health_sacrifice||r->activation!=0||r->aftercast!=0||
    r->recharge!=15||r->duration0!=8||r->duration15!=20||r->scale0!=4||r->scale15!=4||
    r->bonus0!=1||r->bonus15!=1||r->caster_body_resource!=763||r->target_body_resource!=2077)return {};
 return MelandruResiliencePlan{rank,1000ull*interpolate(8,20,rank)};
}
struct MelandruResilienceProposal {Registry registry;Entry applied;std::vector<Entry> removed;};
inline std::optional<MelandruResilienceProposal> prepare_melandru_resilience(
 const MelandruResiliencePlan& plan,const Registry& original,Scope owner,unsigned effect,Time now){
 const auto canonical=melandru_resilience(plan.rank);
 if(!canonical||*canonical!=plan||!original.matches(owner)||owner.actor>65535||!effect||now>UINT64_MAX-plan.duration)return {};
 Entry entry{effect,owner.actor,451,plan.rank,Category::stance,now,now+plan.duration};
 auto next=original;auto applied=next.apply(owner,entry,Replace::category);
 if(!applied.accepted)return {};
 return MelandruResilienceProposal{std::move(next),entry,std::move(applied.removed)};
}
struct MelandruResilienceRegeneration {
 unsigned conditions=0,hexes=0;int health_pips=0,energy_pips=0;
 bool operator==(const MelandruResilienceRegeneration&)const=default;
};
// Local wiki revision2648991: +4 Health/+1 Energy per condition and hex.
// Return the uncapped contribution: joint resource integrators own stacking,
// degeneration and caps. Multiple copies of the same hex count only once.
inline std::optional<MelandruResilienceRegeneration> melandru_resilience_regeneration(
 const Registry& registry,Scope owner,const combat::conditions::State& conditions,Time now){
 if(!registry.matches(owner)||owner.actor>65535)return {};
 const auto stance=registry.newest_skill(owner,451,now);
 if(!stance)return MelandruResilienceRegeneration{};
 const auto plan=melandru_resilience(stance->rank);
 if(!plan||stance->kind!=Category::stance||stance->cause!=owner.actor||!stance->expires||
    stance->applied>UINT64_MAX-plan->duration||*stance->expires!=stance->applied+plan->duration)return {};
 std::set<unsigned> hexes;
 for(const auto& entry:registry.active(owner,now))if(entry.kind==Category::hex)hexes.insert(entry.skill);
 const unsigned count=conditions.count(now),total=count+unsigned(hexes.size());
 return MelandruResilienceRegeneration{count,unsigned(hexes.size()),int(4*total),int(total)};
}
// Snapshot-only expiry schedule. Settle resources before any condition/hex
// application, removal, replacement or stance change, then take a new snapshot.
inline std::optional<std::vector<Time>> melandru_resilience_boundaries(
 const Registry& registry,Scope owner,const combat::conditions::State& conditions,Time after,Time before){
 if(before<after||!melandru_resilience_regeneration(registry,owner,conditions,after))return {};
 auto result=conditions.expiry_boundaries(after,before);
 for(unsigned i=0;i<unsigned(combat::conditions::Kind::count);++i)
  if(const auto e=conditions.lookup(static_cast<combat::conditions::Kind>(i));e&&e->applied>after&&e->applied<before)result.push_back(e->applied);
 for(const auto& e:registry.active(owner,after))if((e.skill==451||e.kind==Category::hex)&&e.expires&&*e.expires>after&&*e.expires<before)result.push_back(*e.expires);
 std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());
 return result;
}
}
