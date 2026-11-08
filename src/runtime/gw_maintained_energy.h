#pragma once
#include "gw_blessed_signet.h"
#include "gw_overcast_settlement.h"
namespace fw::gw::skills::effects {
struct MaintainedEnergy {
 overcast::Settlement energy;MaintainedEnchantments ownership;
 struct End {MaintainedEnchantments::Release release;Time at=0;};
 std::vector<End> ended;int final_pips=0;
};
// Copy-only constant-base settlement. Modifier expiry and the first exact zero
// crossing are event boundaries;never apply the final upkeep count backwards.
inline std::optional<MaintainedEnergy> prepare_maintained_energy(
 const MaintainedEnchantments& ownership,const overcast::CeilingCurve& curve,Scope caster,
 const vitals::Resources& resources,const energy::Regeneration& regeneration,std::uint32_t debt,
 Time at,int base_pips,unsigned tail=0){
 if(!resources.health||base_pips<0||base_pips>10||at<curve.cursor())return {};
 auto initial=overcast::settle_constant(curve,caster,resources,regeneration,debt,curve.cursor(),0,tail);if(!initial)return {};
 MaintainedEnergy result{*initial,ownership};std::uint64_t gained=0,lost=0;std::optional<Time> first_zero;std::optional<std::uint64_t> zero_before;
 for(;;){
  auto& state=result.energy;const auto from=state.curve.cursor();
  const auto upkeep=result.ownership.prepare_upkeep(caster,state.curve.external_energy_pips(base_pips,from),overcast::precise_current(state)<=0,from);
  if(!upkeep||!result.ownership.commit_upkeep(*upkeep))return {};
  const auto count=result.ownership.count(caster,from);if(!count||!state.curve.set_maintained_upkeep(caster,*count,from))return {};
  for(const auto& release:upkeep->releases)result.ended.push_back({release,from});result.final_pips=upkeep->pips;
  if(from==at)break;
  const auto edge=state.curve.next_energy_boundary(from);const auto until=edge?std::min(at,*edge):at;
  auto next=overcast::settle_constant(state.curve,caster,state.resources,state.regeneration,state.debt,until,upkeep->pips,state.fractional_tail);if(!next)return {};
  if(next->first_zero&&*next->first_zero<until){
   next=overcast::settle_constant(state.curve,caster,state.resources,state.regeneration,state.debt,*next->first_zero,upkeep->pips,state.fractional_tail);if(!next)return {};
  }
  if(next->curve.cursor()<=from)return {};
  gained+=next->gained_units;lost+=next->lost_units;
  if(next->first_zero&&!first_zero){first_zero=next->first_zero;zero_before=next->first_zero_before_units;}
  state=std::move(*next);
 }
 result.energy.gained_units=gained;result.energy.lost_units=lost;result.energy.first_zero=first_zero;result.energy.first_zero_before_units=zero_before;
 return result;
}
}
