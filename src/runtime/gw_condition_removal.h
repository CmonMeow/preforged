#pragma once
#include "gw_skill_catalog_generated.h"
#include <cmath>
#include <optional>
namespace fw::gw::skills::effects {
enum class HealBasis {none,removed,remaining,enchanted};
struct ConditionRemovalPlan {unsigned skill,rank,count,per_condition_heal;HealBasis healing;unsigned divine_heal;float range=1248.f;bool self=true;};
inline std::optional<ConditionRemovalPlan> condition_removal(unsigned skill,unsigned rank,unsigned divine){
 const auto* row=find_skill(skill);if(!row||rank>20||divine>20||((skill<275||skill>278)&&skill!=1691&&skill!=1401))return {};
 // Mending Touch: two covered conditions; healing applies per removal.
 if(skill==1401)return ConditionRemovalPlan{skill,rank,2u,static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.)),HealBasis::removed,static_cast<unsigned>(std::lround(3.2*divine)),144.f,true};
 const auto heal=skill==278?0u:static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.));
 return ConditionRemovalPlan{skill,rank,skill==276||skill==278?10u:1u,heal,skill==1691?HealBasis::enchanted:skill==278?HealBasis::none:skill==277?HealBasis::remaining:HealBasis::removed,static_cast<unsigned>(std::lround(3.2*divine)),1248.f,skill==1691||skill==277||skill==278};
}
inline std::optional<unsigned> condition_heal(const ConditionRemovalPlan& plan,unsigned removed,unsigned remaining,bool enchanted=false){
 if(removed>10||remaining>10||removed+remaining>10)return {};
 const unsigned count=plan.healing==HealBasis::removed?removed:plan.healing==HealBasis::remaining?remaining:plan.healing==HealBasis::enchanted?unsigned(enchanted):0;
 return plan.divine_heal+plan.per_condition_heal*count;
}
}
