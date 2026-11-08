#pragma once
#include "gw_combat.h"
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct StormChaserPlan {
 unsigned rank=0,energy=1;Time duration=8000;float movement=1.25f;
 auto operator<=>(const StormChaserPlan&)const=default;
};
inline std::optional<StormChaserPlan> storm_chaser(unsigned rank){
 const auto* row=find_skill(455);
 if(rank>20||!row||row->type!=3||row->attribute!=24||row->energy_cost()!=10||row->recharge!=20||
    row->scale0!=1||row->scale15!=5||row->bonus0!=25||row->bonus15!=25||row->duration0!=8||row->duration15!=20)return {};
 return StormChaserPlan{rank,interpolate(1,5,rank),1000ull*interpolate(8,20,rank),1.25f};
}
inline std::optional<StormChaserPlan> active_storm_chaser(const Registry& registry,Scope owner,Time now){
 const auto entry=registry.newest(owner,Category::stance,now);
 return entry&&entry->skill==455&&entry->instance&&entry->cause&&entry->applied<=now&&entry->expires&&now<*entry->expires?
     storm_chaser(entry->rank):std::nullopt;
}
inline unsigned storm_chaser_energy(const StormChaserPlan& plan,const combat::Hit& hit){
 const auto canonical=storm_chaser(plan.rank);
 if(!canonical||*canonical!=plan||hit.reason!=combat::Reason::accepted||!std::isfinite(hit.damage)||hit.damage<0)return 0;
 switch(hit.kind){case combat::Kind::earth:case combat::Kind::fire:case combat::Kind::cold:case combat::Kind::lightning:return plan.energy;
 default:return 0;}
}
}
