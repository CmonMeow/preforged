#pragma once
#include "gw_skill_catalog_generated.h"
#include <array>
#include <cmath>
#include <optional>

namespace fw::gw::skills::effects {
enum class Condition {none,weakness,cracked_armor,blind};
struct Strike {
    float radius=0;
    double earth_damage=0,knocked_down_bonus=0;
    Condition condition=Condition::none;
    unsigned duration_seconds=0;
};
// An immutable reviewed plan, never supplied by the network client. The instance
// must preflight every target/mechanic before committing any mutation. Unknown
// hostile health/armor is not filled with invented values. Empty foes is success.
struct AoEPlan {unsigned skill=0,rank=0;std::array<Strike,3> strikes{};unsigned count=0;};
inline unsigned interpolate(unsigned low,unsigned high,unsigned rank) {
    return static_cast<unsigned>(std::lround(double(low)+(double(high)-low)*rank/15.));
}
inline double damage_scale(unsigned low,unsigned high,unsigned rank){return double(low)+(double(high)-low)*rank/15.;}
inline std::optional<AoEPlan> caster_earth_aoe(unsigned skill,unsigned rank) {
    if(rank>20)return std::nullopt;
    AoEPlan p;p.skill=skill;p.rank=rank;
    // Local pre-Reforged wiki descriptions plus GWCA Range constants. These
    // descriptions have explicit formulas; catalog numeric scale fields alone
    // are insufficient to identify damage or conditional behavior.
    // Aftershock has the documented 240-unit exception to nearby range.
    if(skill==174){p.count=1;p.strikes[0]={240.f,damage_scale(26,100,rank),damage_scale(10,68,rank),Condition::none,0};}
    else if(skill==937){p.count=3;const auto damage=damage_scale(15,60,rank);const auto duration=interpolate(1,10,rank);
        p.strikes[0]={322.f,damage,0,Condition::weakness,duration};
        p.strikes[1]={252.f,damage,0,Condition::cracked_armor,duration};
        p.strikes[2]={166.f,damage,0,Condition::blind,duration};}
    else return std::nullopt;
    return p;
}
inline double base_damage(const AoEPlan& plan,float distance,bool knocked_down) {
    if(!std::isfinite(distance)||distance<0||plan.count>3)return 0;
    double damage=0;for(unsigned i=0;i<plan.count;++i)if(distance<=plan.strikes[i].radius)
        damage+=plan.strikes[i].earth_damage+(knocked_down?plan.strikes[i].knocked_down_bonus:0);
    return damage;
}
}
