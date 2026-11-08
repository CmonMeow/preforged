#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include <compare>
#include <set>
#include <span>
namespace fw::gw::skills::effects {
struct DemonicFleshPlan {
 unsigned rank=0,damage=5;Time duration=30000;float radius=166;
 auto operator<=>(const DemonicFleshPlan&)const=default;
};
inline std::optional<DemonicFleshPlan> demonic_flesh(unsigned rank){
 const auto* row=find_skill(130);
 if(rank>20||!row||row->type!=6||row->attribute!=4||row->encoded_energy!=5||row->activation!=1.f||
    row->aftercast!=.75f||row->recharge!=30||row->pvp_skill!=3433||row->scale0!=5||row->scale15!=20||
    row->duration0!=30||row->duration15!=60||row->caster_body_resource!=239||row->target_body_resource!=240)return {};
 return DemonicFleshPlan{rank,interpolate(5,20,rank),std::uint64_t(interpolate(30,60,rank))*1000,166};
}
inline bool demonic_scope(Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
struct DemonicFleshUse {
 Scope caster,target;unsigned skill=0;std::uint64_t event=0;Time at=0;bool success=false,foe=false;
};
struct DemonicFleshReservation {
 Scope caster;std::uint64_t event=0,id=0;
 auto operator<=>(const DemonicFleshReservation&)const=default;
};
struct DemonicFleshTrigger {bool accepted=false;unsigned effect=0;std::optional<DemonicFleshPlan> plan;};
// World supplies actual successful skill-use and allegiance facts. An ordinary
// attack/damage notification is not a substitute for this event. Failed dagger
// sequences must not be reported as successful uses. Replay ownership is World.
inline DemonicFleshTrigger demonic_flesh_trigger(const Registry& registry,const DemonicFleshUse& use){
 const auto* row=find_skill(use.skill);
 if(!demonic_scope(use.caster)||!demonic_scope(use.target)||use.caster.instance!=use.target.instance||
    !use.event||!row||!row->id||!row->type||!registry.matches(use.caster)||(use.foe&&use.caster==use.target))return {};
 if(!use.success||!use.foe)return {true,0,{}};
 const auto entry=registry.newest_skill(use.caster,130,use.at);if(!entry)return {true,0,{}};
 const auto plan=demonic_flesh(entry->rank);
 if(!plan||entry->kind!=Category::enchantment||entry->cause!=use.caster.actor||!entry->instance||
    entry->applied>use.at||entry->applied>UINT64_MAX-plan->duration||!entry->expires||*entry->expires!=entry->applied+plan->duration)return {};
 return {true,entry->instance,plan};
}
struct DemonicFleshRecipient {Scope owner;double x=0,y=0;std::uint16_t plane=0;bool alive=false,foe=false;};
inline std::optional<std::vector<Scope>> demonic_flesh_recipients(const DemonicFleshPlan& plan,const DemonicFleshUse& use,
 double x,double y,std::uint16_t plane,std::span<const DemonicFleshRecipient> candidates){
 const auto expected=demonic_flesh(plan.rank);
 if(!expected||plan!=*expected||!use.success||!use.foe||!demonic_scope(use.caster)||!demonic_scope(use.target)||
    use.caster.instance!=use.target.instance||use.caster==use.target||!std::isfinite(x)||!std::isfinite(y))return {};
 std::set<unsigned> seen;std::vector<Scope> selected;
 for(const auto& target:candidates){
  if(!demonic_scope(target.owner)||target.owner.instance!=use.caster.instance||!std::isfinite(target.x)||!std::isfinite(target.y)||
     !seen.emplace(target.owner.actor).second)return {};
  if(target.owner==use.caster||target.owner==use.target||!target.alive||!target.foe||target.plane!=plane)continue;
  if(std::hypot(target.x-x,target.y-y)<=plan.radius)selected.push_back(target.owner);
 }
 return selected; // Caller iteration order is an explicit host policy.
}
inline std::optional<combat::Hit> demonic_flesh_hit(const DemonicFleshPlan& plan,combat::Actor& actor,unsigned level){
 const auto expected=demonic_flesh(plan.rank);if(!expected||plan!=*expected||!actor.generation)return {};
 auto proposed=actor;const auto hit=combat::apply(proposed,{combat::Kind::armor_ignoring,double(plan.damage),level,130},true);
 if(hit.reason!=combat::Reason::accepted)return {};actor=std::move(proposed);return hit;
}
}
