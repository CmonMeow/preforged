#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <compare>
#include <algorithm>
namespace fw::gw::skills::effects {
struct AttunementPlan{unsigned skill=0,rank=0,attribute=0,attribute_bonus=0;std::uint64_t duration=0;bool all_elements=false;auto operator<=>(const AttunementPlan&)const=default;};
inline std::optional<AttunementPlan> attunement(unsigned skill,unsigned rank=0){
 if(rank>20)return {};AttunementPlan p;p.skill=skill;p.rank=rank;p.duration=std::uint64_t(interpolate(36,60,rank))*1000;
 switch(skill){case 169:p.attribute=9;break;case 184:p.attribute=10;break;case 208:p.attribute=11;break;case 225:p.attribute=8;break;
 case 164:p.all_elements=true;p.duration=std::uint64_t(interpolate(25,60,rank))*1000;p.attribute_bonus=interpolate(1,2,rank);break;default:return {};}
 return p;
}
inline unsigned standard_attunement_refund(unsigned cost){const auto n=10+3*cost,q=n/10,r=n%10;return q+(r>5||(r==5&&(q&1)));}
inline bool active_attunement(const Entry& entry,std::uint64_t now){return entry.kind==Category::enchantment&&now>=entry.applied&&(!entry.expires||now<*entry.expires)&&bool(attunement(entry.skill,entry.rank));}
inline unsigned attunement_cast_refund(const std::vector<Entry>& entries,const SkillRecord& cast,std::uint64_t now){
 unsigned result=0;std::vector<unsigned> seen;
 for(const auto& e:entries)if(active_attunement(e,now)&&std::find(seen.begin(),seen.end(),e.skill)==seen.end()){seen.push_back(e.skill);const auto plan=*attunement(e.skill,e.rank);
  if(plan.all_elements){if(cast.attribute>=8&&cast.attribute<=11)result+=cast.energy_cost()/2;}
  else if(cast.attribute==plan.attribute)result+=standard_attunement_refund(cast.energy_cost());
 }return result;
}
inline unsigned elemental_attribute_bonus(const std::vector<Entry>& entries,std::uint64_t now){unsigned result=0;for(const auto& e:entries)if(e.skill==164&&active_attunement(e,now))result=std::max(result,attunement(e.skill,e.rank)->attribute_bonus);return result;}
}
