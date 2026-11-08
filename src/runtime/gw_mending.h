#pragma once
#include "gw_blessed_signet.h"
#include "gw_health_regeneration.h"
namespace fw::gw::skills::effects {
struct MendingPlan {unsigned rank=0,pips=1,upkeep=1,energy=10;Time activation=2000,aftercast=750;auto operator<=>(const MendingPlan&)const=default;};
inline std::optional<MendingPlan> mending(unsigned rank){
 const auto* row=find_skill(290);
 if(rank>20||!row||row->type!=6||row->profession!=3||row->attribute!=13||row->special||row->energy_cost()!=10||row->activation!=2.f||row->aftercast!=.75f||row->recharge||row->scale0!=1||row->scale15!=4||row->duration0!=131072||row->duration15!=131072||row->target_body_resource!=512)return {};
 return MendingPlan{rank,interpolate(1,4,rank)};
}
struct MendingApplication {Registry registry;regeneration::State regeneration;MaintainedEnchantments ownership;Entry entry;std::vector<Entry> removed;};
// Copy-only application;World must prove allied/live targets,settle both
// resource clocks,payment and source generation before committing this state.
inline std::optional<MendingApplication> prepare_mending(const MendingPlan& plan,Scope caster,Scope target,unsigned effect,Time now,const Registry& registry,const regeneration::State& regeneration,const MaintainedEnchantments& ownership){
 const auto canonical=mending(plan.rank);if(!canonical||*canonical!=plan||!registry.matches(target)||!regeneration.matches(target)||regeneration.cursor()!=now)return {};
 MendingApplication result{registry,regeneration,ownership,{effect,caster.actor,290,plan.rank,Category::enchantment,now,{}},{}};
 const auto applied=result.registry.apply(target,result.entry,Replace::same_skill);if(!applied.accepted)return {};
 for(const auto& old:applied.removed){if(old.skill!=290||!result.ownership.ended(target,old.instance,now)||!result.regeneration.remove(target,old.instance))return {};}
 if(!result.regeneration.apply(target,{effect,caster.actor,290,int(plan.pips),now,{}},true)||!result.ownership.add_mending(caster,target,effect,result.registry,now))return {};
 result.removed=applied.removed;return result;
}
struct MendingEnd {Registry registry;regeneration::State regeneration;MaintainedEnchantments ownership;Entry entry;Scope caster;};
// The supplied caster must be the exact maintainer,not merely an actor with
// the same ID. Settle upkeep Energy through now before committing this end.
// A future Health end retains prior regeneration without recursively settling
// another actor. World must commit/publish the resulting states together.
inline std::optional<MendingEnd> prepare_mending_end(Scope caster,Scope target,unsigned effect,Time now,
 const Registry& registry,const regeneration::State& regeneration,const MaintainedEnchantments& ownership){
 if(!registry.matches(target)||!regeneration.matches(target)||regeneration.cursor()>now||ownership.maintainer(target,effect,now)!=caster)return {};
 const auto entry=registry.lookup(target,effect);const auto source=regeneration.lookup(target,effect);
 if(!entry||(entry->skill!=290&&entry->skill!=308)||entry->kind!=Category::enchantment||entry->expires||entry->cause!=caster.actor||entry->applied>now||!source||source->skill!=entry->skill||source->cause!=caster.actor||source->begins!=entry->applied||source->ends)return {};
 if(entry->skill==290){const auto plan=mending(entry->rank);if(!plan||source->pips!=int(plan->pips))return {};}
 else if(!succor()||entry->rank||source->pips!=1||caster.actor==target.actor)return {};
 MendingEnd result{registry,regeneration,ownership,*entry,caster};
 if(!result.registry.remove(target,effect)||!result.regeneration.end(target,effect,now)||!result.ownership.ended(target,effect,now))return {};
 return result;
}

}
