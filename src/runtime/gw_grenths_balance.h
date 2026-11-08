#pragma once
#include "gw_combat.h"
#include <algorithm>
namespace fw::gw::skills::effects {
struct BalanceResult {
 combat::Health caster,target;
 double caster_gain=0,caster_loss=0,target_gain=0,target_loss=0;
};
// Reviewed local Grenth's Balance revision2663563. Health gain/loss is neither
// damage nor ordinary healing/life stealing. A caster killed by the preceding
// spell reaction can still cause target loss, but cannot gain Health.
inline std::optional<BalanceResult> grenths_balance(const combat::Health& caster,
 const combat::Health& target,bool reaction_killed_caster=false){
 if(!combat::valid(caster)||!combat::valid(target)||target.current<=0||
    (caster.current<=0&&!reaction_killed_caster)||
    (reaction_killed_caster&&caster.current!=0))return {};
 BalanceResult result{caster,target};
 if(target.current>caster.current){
  result.target_loss=std::min((target.current-caster.current)/2.,double(caster.maximum));
  result.target.current-=result.target_loss;
  if(!reaction_killed_caster){
   result.caster_gain=std::min(result.target_loss,caster.maximum-caster.current);
   result.caster.current+=result.caster_gain;
  }
 }else if(caster.current>target.current){
  result.caster_loss=(caster.current-target.current)/2.;
  result.caster.current-=result.caster_loss;
  result.target_gain=std::min(result.caster_loss,target.maximum-target.current);
  result.target.current+=result.target_gain;
 }
 return result;
}
}
