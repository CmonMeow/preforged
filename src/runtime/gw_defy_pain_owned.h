#pragma once
#include "gw_temporary_health.h"
#include "gw_effect_registry.h"
namespace fw::gw::skills::effects {
// Mitigation requires the same exact status identity as the Health ledger.
inline std::optional<DefyPainPlan> active_defy_pain(const Registry& registry,const TemporaryHealth& ledger,Scope owner,Time now){
 if(!registry.matches(owner)||ledger.owner()!=owner||now<ledger.cursor()||!ledger.defy_pain_source())return {};
 const auto& source=*ledger.defy_pain_source();const auto entry=registry.lookup(owner,source.effect);if(!entry)return {};
 const auto plan=defy_pain(source.rank,entry->skill==3204);
 if(!plan||entry->skill!=plan->skill||entry->cause!=owner.actor||entry->rank!=source.rank||source.bonus!=plan->health_bonus||
    entry->kind!=(plan->stance?Category::stance:Category::other)||entry->applied!=source.begins||entry->expires!=source.ends||
    source.ends<source.begins||source.ends-source.begins!=plan->duration||now<source.begins||now>=source.ends)return {};
 return plan;
}
}
