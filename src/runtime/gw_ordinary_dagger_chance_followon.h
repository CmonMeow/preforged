#pragma once
#include <cmath>
#include <optional>
#include <string>
namespace fw::gw::combat::weapon {
// Trusted review policy. No default retail reconciliation or random enable.
struct OrdinaryDaggerChancePolicyFollowon {
 enum class Basis {unspecified,legacy_rank_only_declared,explicit_inherent_two};
 Basis basis=Basis::unspecified;std::string evidence;
 bool valid()const{return (basis==Basis::legacy_rank_only_declared||basis==Basis::explicit_inherent_two)&&!evidence.empty()&&evidence.size()<=1024;}
};
struct OrdinaryDaggerContactSelectionFollowon {unsigned percent=0,contacts=0;};
inline std::optional<OrdinaryDaggerContactSelectionFollowon> select_ordinary_dagger_contacts_followon(
 const OrdinaryDaggerChancePolicyFollowon& policy,unsigned item_type,unsigned mastery,bool locust_fury,double reviewed_draw){
 if(!policy.valid()||item_type!=32||mastery>20||!std::isfinite(reviewed_draw)||reviewed_draw<0||reviewed_draw>=1)return {};
 // 2021 Double_strike/Dagger_Mastery describe rank2% without excluding innate
 // chance. Dagger explicitly states inherent2%. Caller must select the basis.
 const unsigned chance=2*mastery+(policy.basis==OrdinaryDaggerChancePolicyFollowon::Basis::explicit_inherent_two?2:0)+(locust_fury?50:0);
 return OrdinaryDaggerContactSelectionFollowon{chance,reviewed_draw<double(chance)/100?2u:1u};
}
}
