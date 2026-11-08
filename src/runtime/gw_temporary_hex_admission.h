#pragma once
#include "gw_temporary_hex_slots.h"
namespace fw::gw::skills::effects {
struct TemporaryHexAdmission {
 Scope owner;unsigned index=0;skills::Slot requested;
 std::optional<TemporaryHexCastGrant> copied;
};
// Validate ordinary slots against ordinary account policy. Only exact current
// temporary grants substitute their original learned ability. The caller must
// still check actual skill implementation, mode, target, resources and timing.
inline std::optional<TemporaryHexAdmission> admit_temporary_hex_cast(const TemporaryHexSlots& copies,
 Scope owner,const skills::Slots& bar,const skills::Policy& policy,unsigned index,skills::Slot requested,Time now){
 if(index>=bar.size()||!requested.skill||bar[index]!=requested)return {};
 const auto eligibility=copies.eligibility_bar(owner,bar,now);
 if(!eligibility||skills::validate(*eligibility,policy)!=skills::Reason::accepted)return {};
 const auto grant=copies.authorize(owner,bar,index,requested,now);
 if(grant&&grant->require_campaign){
  const auto* skill=find_skill(requested.skill);
  if(!skill||(skill->campaign&&!(policy.campaigns&(1u<<skill->campaign)))||
     (policy.pvp_match&&(skill->special&0x80000u)))return {};
 }
 return TemporaryHexAdmission{owner,index,requested,grant};
}
inline bool valid_temporary_hex_admission(const TemporaryHexAdmission& admission,const TemporaryHexSlots& copies,
 const skills::Slots& bar,const skills::Policy& policy,Time now){
 const auto current=admit_temporary_hex_cast(copies,admission.owner,bar,policy,admission.index,admission.requested,now);
 if(!current||bool(current->copied)!=bool(admission.copied))return false;
 return !admission.copied||copies.valid(*admission.copied,bar,now);
}
}
