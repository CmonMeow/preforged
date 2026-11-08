#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <set>
namespace fw::gw::skills::effects {
struct CryFrustrationPlan {
 unsigned skill=57,rank=0,damage=15;
 float range=1248,radius=322;
 auto operator<=>(const CryFrustrationPlan&)const=default;
};
inline std::optional<CryFrustrationPlan> cry_frustration(unsigned rank){
 const auto* row=find_skill(57);
 if(rank>20||!row||row->type!=5||row->attribute!=2||row->scale0!=15||row->scale15!=75)return {};
 return CryFrustrationPlan{57,rank,interpolate(15,75,rank),1248,322};
}
enum class CryActivity {idle,skill,basic_attack};
struct CryContact {
 Scope owner;float distance=0;bool alive=true,hostile=true;
 CryActivity activity=CryActivity::idle;
};
struct CryRecipient {Scope owner;unsigned damage;bool interrupt_attempt;};
struct CryProposal {bool triggered=false;std::vector<CryRecipient> recipients;};
// World supplies current activity and target-centered distances after scoped
// range/plane/visibility checks. Protection affects the eventual interrupt,
// never the initial trigger, other recipients, or their damage eligibility.
inline std::optional<CryProposal> prepare_cry_frustration(const CryFrustrationPlan& plan,
 Scope selected,const std::vector<CryContact>& contacts){
 const auto expected=cry_frustration(plan.rank);
 if(!expected||*expected!=plan||!selected.instance||!selected.actor||selected.actor>65535||!selected.generation)return {};
 std::set<std::uint32_t> unique;const CryContact* target=nullptr;
 for(const auto& contact:contacts){
  if(contact.owner.instance!=selected.instance||!contact.owner.actor||contact.owner.actor>65535||!contact.owner.generation||
     !unique.insert(contact.owner.actor).second||!std::isfinite(contact.distance)||contact.distance<0||
     (contact.activity!=CryActivity::idle&&contact.activity!=CryActivity::skill&&contact.activity!=CryActivity::basic_attack))return {};
  if(contact.owner==selected)target=&contact;
 }
 if(!target||target->distance!=0||!target->alive||!target->hostile)return {};
 CryProposal result;
 // A basic attack alone does not satisfy the selected foe's using-a-skill gate.
 if(target->activity!=CryActivity::skill)return result;
 result.triggered=true;
 for(const auto& contact:contacts)if(contact.alive&&contact.hostile&&contact.distance<=plan.radius)
  result.recipients.push_back({contact.owner,plan.damage,contact.activity!=CryActivity::idle});
 return result;
}
}
