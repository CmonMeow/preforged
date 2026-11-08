#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <cmath>
#include <optional>
#include <set>
#include <span>
#include <vector>
namespace fw::gw::skills::effects {
struct NecroticTraversalPlan {
 unsigned skill=97,rank=0,energy=5;
 Time activation=750,poison=5000;
 float range=1248,nearby=252,compass=5000;
 auto operator<=>(const NecroticTraversalPlan&)const=default;
};
inline std::optional<NecroticTraversalPlan> necrotic_traversal(unsigned rank){
 const auto* r=find_skill(97);
 if(rank>20||!r||r->campaign!=1||r->profession!=4||r->attribute!=5||r->type!=5||
    r->special!=262144||r->energy_cost()!=5||r->health_sacrifice||r->adrenaline||
    r->activation!=.75f||r->aftercast!=.75f||r->recharge||r->target||
    r->scale0!=5||r->scale15!=20||r->caster_body_resource!=2077||r->target_body_resource!=2077)return {};
 // Rank curve is pinned metadata; wiki range terms use supplied GWCA units.
 // Runtime aftercast is deliberately not resolved by this effect plan.
 return NecroticTraversalPlan{97,rank,5,750,Time(5000+1000*rank)};
}
struct TraversalCorpse {
 Scope scope;std::uint64_t revision=0;unsigned plane=0;float x=0,z=0;
 bool dead=false;std::optional<bool> exploitable;bool consumed=false;
 auto operator<=>(const TraversalCorpse&)const=default;
};
inline bool valid_traversal_scope(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
struct TraversalPool {
 NecroticTraversalPlan plan;Scope caster;float x=0,z=0;std::vector<TraversalCorpse> corpses;
};
// An unknown fresh corpse cannot be silently omitted from a random pool.
// Do not replace this pool with the Wells' nearest-corpse selection policy.
inline std::optional<TraversalPool> necrotic_traversal_pool(const NecroticTraversalPlan& plan,
 Scope caster,float x,float z,std::span<const TraversalCorpse> corpses){
 if(necrotic_traversal(plan.rank)!=plan||!valid_traversal_scope(caster)||
    !std::isfinite(x)||!std::isfinite(z))return {};
 TraversalPool pool{plan,caster,x,z,{}};std::set<Scope> seen;
 for(const auto& corpse:corpses){
  if(!valid_traversal_scope(corpse.scope)||!corpse.revision||corpse.revision==UINT64_MAX||corpse.plane>65535||
     !std::isfinite(corpse.x)||!std::isfinite(corpse.z)||!seen.insert(corpse.scope).second)return {};
  if(corpse.scope.instance!=caster.instance||corpse.scope==caster||!corpse.dead||corpse.consumed||
     std::hypot(double(corpse.x)-x,double(corpse.z)-z)>plan.range)continue;
  if(!corpse.exploitable)return {};
  if(*corpse.exploitable)pool.corpses.push_back(corpse);
 }
 return pool;
}
struct TraversalCorpseClaim {
 NecroticTraversalPlan plan;Scope caster;TraversalCorpse corpse;Time completed=0;
};
// World must draw an unbiased index in [0,pool.size), then validate navigation,
// bundle drop and the whole poison/reaction cohort before consuming this claim.
inline std::optional<TraversalCorpseClaim> select_necrotic_traversal_corpse(
 const TraversalPool& pool,std::size_t draw,Time completed){
 if(!completed||completed>UINT64_MAX-pool.plan.poison||necrotic_traversal(pool.plan.rank)!=pool.plan||
    !valid_traversal_scope(pool.caster)||draw>=pool.corpses.size())return {};
 const auto checked=necrotic_traversal_pool(pool.plan,pool.caster,pool.x,pool.z,pool.corpses);
 if(!checked||checked->corpses!=pool.corpses)return {};
 const auto& corpse=pool.corpses[draw];
 if(!valid_traversal_scope(corpse.scope)||corpse.scope.instance!=pool.caster.instance||corpse.scope==pool.caster||
    !corpse.dead||corpse.exploitable!=true||corpse.consumed||!corpse.revision||corpse.revision==UINT64_MAX||
    corpse.plane>65535||!std::isfinite(corpse.x)||!std::isfinite(corpse.z))return {};
 return TraversalCorpseClaim{pool.plan,pool.caster,corpse,completed};
}
inline std::optional<TraversalCorpse> consume_necrotic_traversal_corpse(
 const TraversalCorpseClaim& claim,const TraversalCorpse& current){
 if(!claim.completed||claim.completed>UINT64_MAX-claim.plan.poison||necrotic_traversal(claim.plan.rank)!=claim.plan||!valid_traversal_scope(claim.caster)||
    current!=claim.corpse||!valid_traversal_scope(current.scope)||current.scope.instance!=claim.caster.instance||current.scope==claim.caster||
    current.plane>65535||!std::isfinite(current.x)||!std::isfinite(current.z)||
    !current.dead||current.exploitable!=true||current.consumed||!current.revision||current.revision==UINT64_MAX)return {};
 auto next=current;next.consumed=true;++next.revision;return next;
}
}
