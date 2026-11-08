#pragma once
#include "gw_effect_registry.h"
#include <algorithm>
#include <cmath>
#include <span>
namespace fw::gw::skills::effects {
struct HealingRingPlan {unsigned rank=0,divine_rank=0,healing=30,caster_healing=0;float radius=166;auto operator<=>(const HealingRingPlan&)const=default;};
inline std::optional<HealingRingPlan> healing_ring(unsigned rank,unsigned divine){
 const auto* row=find_skill(1262);
 if(rank>20||divine>20||!row||row->type!=5||row->attribute!=13||row->scale0!=30||row->scale15!=180||row->target_body_resource!=499)return {};
 return HealingRingPlan{rank,divine,row->scale0+(row->scale15-row->scale0)*rank/15,static_cast<unsigned>(std::lround(3.2*divine)),166};
}
enum class RingCreature {unknown,player,creature,spirit};
struct RingCandidate {Scope scope;std::size_t plane=0;double x=0,y=0;bool alive=false;RingCreature kind=RingCreature::unknown;};
struct RingRecipient {Scope scope;unsigned healing=0;};
// Pure recipient proposal. The World owner must settle Health, validate NPC
// creature traits and apply actual healing modifiers/caps before publication.
// An unknown nearby creature aborts the proposal; it is not guessed healable.
inline std::optional<std::vector<RingRecipient>> healing_ring_recipients(const HealingRingPlan& plan,
 Scope caster,std::span<const RingCandidate> candidates){
 const auto canonical=healing_ring(plan.rank,plan.divine_rank);if(!canonical||*canonical!=plan)return {};
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!valid(caster))return {};
 std::set<Scope> seen;std::set<std::uint32_t> actors;const RingCandidate* source=nullptr;
 for(const auto& c:candidates){
  if(!valid(c.scope)||c.scope.instance!=caster.instance||!std::isfinite(c.x)||!std::isfinite(c.y)||!seen.insert(c.scope).second||!actors.insert(c.scope.actor).second)return {};
  if(c.scope==caster)source=&c;
 }
 if(!source||!source->alive||source->kind!=RingCreature::player)return {};
 std::vector<RingRecipient> result;
 for(const auto& c:candidates){
  if(!c.alive||c.plane!=source->plane||std::hypot(c.x-source->x,c.y-source->y)>plan.radius)continue;
  if(c.kind==RingCreature::unknown)return {};
  if(c.kind==RingCreature::spirit)continue;
  if(c.kind!=RingCreature::player&&c.kind!=RingCreature::creature)return {};
  const auto amount=c.scope==caster?plan.caster_healing:plan.healing;
  if(amount)result.push_back({c.scope,amount});
 }
 std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.scope<b.scope;});return result;
}
}
