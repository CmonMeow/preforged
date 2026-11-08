#pragma once
#include "gw_elemental_damage.h"
#include <set>
namespace fw::gw::skills::effects {
struct IcyPrismPlan {
 unsigned rank=0,damage=0,secondary_damage=0,energy=10,overcast=5;
 std::uint64_t activation=1000,aftercast=750,recharge=5000;
 float range=1248,radius=252;combat::Kind kind=combat::Kind::cold;
 auto operator<=>(const IcyPrismPlan&)const=default;
};
inline std::optional<IcyPrismPlan> icy_prism(unsigned rank){
 const auto* r=find_skill(903);
 if(rank>20||!r||r->type!=5||r->profession!=6||r->attribute!=11||r->special!=1||r->energy_cost()!=10||
    r->overcast!=5||r->health_sacrifice||r->activation!=1.f||r->aftercast!=.75f||r->recharge!=5||
    r->scale0!=15||r->scale15!=75||r->bonus0!=15||r->bonus15!=75||r->caster_body_resource!=2077||r->target_body_resource!=1251)return {};
 return IcyPrismPlan{rank,interpolate(15,75,rank),interpolate(15,75,rank)};
}
struct IcyPrismAnchor {Scope caster,target;float x=0,y=0;std::size_t plane=0;Registry registry;};
struct IcyPrismRecipient {Scope owner;float x=0,y=0;std::size_t plane=0;bool hostile=false,alive=false,damageable=false;std::optional<int> cold_armor;};
struct IcyPrismSelection {IcyPrismPlan plan;Scope primary;bool expanded=false;std::vector<Scope> recipients;};
// Caller supplies settled completion-time positions/status and owns World
// range/navigation/protection checks. A status mask alone is not a Water hex.
inline std::optional<IcyPrismSelection> select_icy_prism(const IcyPrismPlan& plan,const IcyPrismAnchor& anchor,
 const std::vector<IcyPrismRecipient>& candidates,Time now){
 const auto canonical=icy_prism(plan.rank);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(anchor.caster)||!valid(anchor.target)||anchor.caster.instance!=anchor.target.instance||
    anchor.caster.actor==anchor.target.actor||!anchor.registry.matches(anchor.target)||!std::isfinite(anchor.x)||!std::isfinite(anchor.y)||candidates.size()>4096)return {};
 const bool expanded=water_hex(anchor.registry,anchor.target,now);
 IcyPrismSelection result{plan,anchor.target,expanded,{}};std::set<unsigned> actors;bool primary=false;
 for(const auto& c:candidates){
  if(!valid(c.owner)||c.owner.instance!=anchor.target.instance||!actors.insert(c.owner.actor).second||!std::isfinite(c.x)||!std::isfinite(c.y))return {};
  if(c.owner.actor==anchor.target.actor){
   if(c.owner!=anchor.target||c.x!=anchor.x||c.y!=anchor.y||c.plane!=anchor.plane||!c.hostile||!c.alive||!c.damageable||!c.cold_armor)return {};
   primary=true;result.recipients.push_back(c.owner);continue;
  }
  if(c.owner==anchor.caster||!expanded||!c.hostile||!c.alive||!c.damageable||c.plane!=anchor.plane||
     std::hypot(c.x-anchor.x,c.y-anchor.y)>plan.radius)continue;
  if(!c.cold_armor)return {};result.recipients.push_back(c.owner);
 }
 if(!primary)return {};return result;
}
struct IcyPrismDamageInput {Scope owner;combat::Actor actor;combat::conditions::State conditions;};
struct IcyPrismHit {Scope owner;combat::Hit hit;};
// Copies only. World commits no Health until every selected actor passes
// lifetime,known cold armor and the existing combat protection resolver.
inline std::optional<std::vector<IcyPrismHit>> prepare_icy_prism_damage(const IcyPrismSelection& selected,
 const std::vector<IcyPrismDamageInput>& inputs,unsigned level,Time now){
 const auto canonical=icy_prism(selected.plan.rank);
 if(!canonical||*canonical!=selected.plan||!level||level>255||selected.recipients.empty()||
    selected.recipients.size()!=inputs.size()||(!selected.expanded&&inputs.size()!=1))return {};
 std::set<unsigned> seen;bool primary=false;std::vector<IcyPrismHit> hits;
 for(const auto owner:selected.recipients){
  if(!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||owner.instance!=selected.primary.instance||!seen.insert(owner.actor).second)return {};
  const auto input=std::find_if(inputs.begin(),inputs.end(),[&](const auto& value){return value.owner==owner;});
  if(input==inputs.end()||std::count_if(inputs.begin(),inputs.end(),[&](const auto& value){return value.owner==owner;})!=1||
     input->actor.id!=owner.actor||input->actor.generation!=owner.generation||!input->actor.cold_armor)return {};
  auto actor=input->actor;actor.cold_armor=input->conditions.armor(*actor.cold_armor,now);
  const bool is_primary=owner==selected.primary;primary|=is_primary;
  const auto hit=combat::apply(actor,{combat::Kind::cold,double(is_primary?selected.plan.damage:selected.plan.secondary_damage),level,903},true);
  if(hit.reason!=combat::Reason::accepted)return {};hits.push_back({owner,hit});
 }
 if(!primary)return {};return hits;
}

}

