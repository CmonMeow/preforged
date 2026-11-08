#pragma once
#include "gw_weapon_intent.h"
#include "gw_weapon_attack_skills.h"
#include <atomic>
#include <memory>
namespace fw::gw::world {class Instance;}
namespace fw::gw::combat::weapon {
inline bool same_weapon(const Owned& a,const Owned& b){
 return a.item==b.item&&a.revision==b.revision&&a.damage.minimum==b.damage.minimum&&
  a.damage.maximum==b.damage.maximum&&a.damage.raw_damage_type==b.damage.raw_damage_type&&
  a.mastery==b.mastery&&a.customized==b.customized&&a.caster_weapon==b.caster_weapon&&a.item_type==b.item_type;
}
// Only World may construct/consume a capability. It binds a start-time fact
// to exact lifetimes/equipment/plan/control epochs and can never be reused.
class AttackAdmission {
 friend class fw::gw::world::Instance;
 const Scope source_,target_;const Owned weapon_;const AttackPlan plan_;
 const std::uint64_t began_,release_,control_,interrupt_;const bool weakness_;
 std::atomic_bool closed_{false};
 AttackAdmission(Scope a,Scope b,const Owned& w,const AttackPlan& p,std::uint64_t at,
                 std::uint64_t release,std::uint64_t control,std::uint64_t interrupt,bool weak):
  source_(a),target_(b),weapon_(w),plan_(p),began_(at),release_(release),control_(control),interrupt_(interrupt),weakness_(weak){}
 bool claim(Scope a,Scope b,const Owned& w,const AttackPlan& p,std::uint64_t now,
            std::uint64_t control,std::uint64_t interrupt){
  if(a!=source_||b!=target_||!same_weapon(w,weapon_)||p!=plan_||now<release_||now-began_>90000||control!=control_||interrupt!=interrupt_)return false;
  return !closed_.exchange(true);
 }
public:
 AttackAdmission(const AttackAdmission&)=delete;
 void cancel(){closed_.store(true);}
 bool open()const{return !closed_.load();}
 std::uint64_t release_time()const{return release_;}
};
using AttackPermit=std::shared_ptr<AttackAdmission>;
struct AttackLease {
 AttackPermit permit;
 explicit AttackLease(AttackPermit p):permit(std::move(p)){}
 ~AttackLease(){if(permit)permit->cancel();}
 AttackLease(const AttackLease&)=delete;
};
}
