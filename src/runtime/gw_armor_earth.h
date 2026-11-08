#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_enchantment_duration.h"
namespace fw::gw::skills::effects {
struct ArmorEarthPlan {
 unsigned rank=0,armor=24,slow_percent=50;
 Time duration=30000;
 auto operator<=>(const ArmorEarthPlan&)const=default;
 double movement_multiplier()const{return 1.-double(slow_percent)/100.;}
};
inline std::optional<ArmorEarthPlan> armor_earth(unsigned rank){
 const auto* row=find_skill(165);
 if(rank>20||!row||row->type!=6||row->attribute!=9||row->duration0!=30||row->duration15!=30||
    row->scale0!=24||row->scale15!=60||row->bonus0!=50||row->bonus15!=14)return {};
 return ArmorEarthPlan{rank,interpolate(24,60,rank),interpolate(50,14,rank),30000};
}
struct ArmorEarthApplication {Registry registry;Entry entry;Applied change;};
// Prepare both modifiers from one owned enchantment. Nothing writes permanent
// equipment armor or baseline movement, so expiry/removal cannot leave a bonus.
inline std::optional<ArmorEarthApplication> prepare_armor_earth(const Registry& original,Scope owner,
 unsigned effect,const ArmorEarthPlan& plan,Time now,const EnchantmentDuration* adjusted=nullptr,
 std::optional<Time> observed_at={}){
 const auto canonical=armor_earth(plan.rank);
 const auto observation=observed_at.value_or(now);
 const auto duration=adjusted?adjusted->for_enchantment(165,plan.duration,owner,observation):std::optional(plan.duration);
 if(!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||!effect||
    observation>now||!duration||!canonical||*canonical!=plan||!original.matches(owner)||now>UINT64_MAX-*duration||original.lookup(owner,effect))return {};
 Entry entry{effect,owner.actor,165,plan.rank,Category::enchantment,now,now+*duration};
 auto registry=original;auto change=registry.apply(owner,entry,Replace::same_skill);
 if(!change.accepted)return {};
 return ArmorEarthApplication{std::move(registry),entry,std::move(change)};
}
inline std::optional<ArmorEarthPlan> active_armor_earth(const Registry& registry,Scope owner,Time now){
 const auto entry=registry.newest_skill(owner,165,now);
 if(!entry||entry->kind!=Category::enchantment||entry->cause!=owner.actor||!entry->instance||!entry->expires)return {};
 return armor_earth(entry->rank);
}
// Sole reviewed armor-buff source. Do not combine arbitrary future armor buffs
// additively: skill stacking has a +25 cap and single-source exceptions.
// Absence of a reviewed base rating remains unknown, not invented as zero.
inline std::optional<int> armor_earth_rating(std::optional<int> base,const std::optional<ArmorEarthPlan>& plan){
 if(!base||*base< -100||*base>500)return {};
 if(plan){const auto canonical=armor_earth(plan->rank);if(!canonical||*canonical!=*plan)return {};}
 const int final=*base+(plan?int(plan->armor):0);
 if(!combat::valid_effective_armor(final))return {};
 return final;
}
inline std::optional<combat::Actor> armor_earth_actor(combat::Actor actor,const Registry& registry,Scope owner,Time now){
 if(!registry.matches(owner)||actor.id!=owner.actor||actor.generation!=owner.generation)return {};
 const auto plan=active_armor_earth(registry,owner,now);
 for(auto* rating:{&actor.earth_armor,&actor.lightning_armor,&actor.fire_armor,&actor.cold_armor}){
  if(!*rating)continue;
  const auto adjusted=armor_earth_rating(*rating,plan);if(!adjusted)return {};
  *rating=adjusted;
 }
 return actor;
}
}
