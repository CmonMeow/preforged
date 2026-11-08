#pragma once
#include "gw_skill_catalog_generated.h"
#include <optional>
namespace fw::gw::combat::weapon {
enum class DaggerFamilyRole:unsigned {death_blossom=775,twisting_fangs=776,horns=777,falling_spider=778,black_lotus=779,fox_fangs=780,jagged=782,unsuspecting=783,repeating=976};
struct DaggerSkillIdentity {unsigned actual=0;DaggerFamilyRole family;bool pvp=false;};
inline std::optional<DaggerSkillIdentity> dagger_identity(unsigned skill){
 unsigned canonical=skill;
 if(skill==3061)canonical=775;else if(skill==3251)canonical=780;
 switch(canonical){case 775:case 776:case 777:case 778:case 779:case 780:case 782:case 783:case 976:break;default:return {};}
 const auto* row=skills::find_skill(skill);const auto* base=skills::find_skill(canonical);
 if(!row||!base||(base->special&0x400000u))return {};
 const bool alias=skill!=canonical;
 if(alias){if(!(row->special&0x400000u)||base->pvp_skill!=skill||row->pvp_skill!=canonical)return {};}
 else if(row->special&0x400000u)return {};
 return DaggerSkillIdentity{skill,static_cast<DaggerFamilyRole>(canonical),alias};
}
inline unsigned dagger_role(unsigned skill){const auto id=dagger_identity(skill);return id?static_cast<unsigned>(id->family):0u;}
inline bool dagger_dual_role(unsigned skill){const auto role=dagger_role(skill);return role>=775&&role<=777;}
inline bool dagger_mode_allowed(unsigned skill,bool pvp){
 const auto id=dagger_identity(skill);if(!id)return false;
 if(id->pvp)return pvp;
 // Match-profile mapping already replaces these two canonical slots by their
 // aliases. Never execute their PvE recharge/damage on a PvP request.
 return !pvp||(skill!=775&&skill!=780);
}
}
