#pragma once
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct EarthProjectilePlan {
 unsigned skill=0,rank=0,energy_storage_rank=0;
 double earth_damage=0;
 float range=1248.f;
 uint64_t weakness_ms=0;
 bool knockdown_if_weak=false,refund_if_weak=false;
};
inline std::optional<EarthProjectilePlan> earth_projectile(unsigned skill,unsigned rank,unsigned energy_storage_rank){
 if(rank>20||energy_storage_rank>20)return std::nullopt;
 EarthProjectilePlan p;p.skill=skill;p.rank=rank;p.energy_storage_rank=energy_storage_rank;
 if(skill==171){p.earth_damage=damage_scale(45,105,rank);p.knockdown_if_weak=true;}
 else if(skill==1374){p.earth_damage=damage_scale(10,85,rank);p.weakness_ms=uint64_t(interpolate(5,15,rank))*1000;}
 else if(skill==1661){p.earth_damage=damage_scale(5,50,rank);p.refund_if_weak=true;}
 else return std::nullopt;
 return p;
}
inline bool valid_earth_projectile(const EarthProjectilePlan& p){
 const auto e=earth_projectile(p.skill,p.rank,p.energy_storage_rank);
 return e&&p.earth_damage==e->earth_damage&&p.range==e->range&&p.weakness_ms==e->weakness_ms&&p.knockdown_if_weak==e->knockdown_if_weak&&p.refund_if_weak==e->refund_if_weak;
}
inline unsigned glowstone_refund(const EarthProjectilePlan& p,bool actual_weakness_at_hit){return valid_earth_projectile(p)&&p.refund_if_weak&&actual_weakness_at_hit?5+p.energy_storage_rank/2:0;}
}
