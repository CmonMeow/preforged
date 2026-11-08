#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <cstdint>
namespace fw::gw::combat::weapon {
// Server-owned effective attributes. Absence is unknown, never a non-Assassin.
struct CriticalProfile {
 unsigned primary=0,critical_strikes=0,level=0;
 bool valid()const{return primary>=1&&primary<=10&&level>=1&&level<=255&&critical_strikes<=20&&(primary==7||critical_strikes==0);}
 bool operator==(const CriticalProfile&)const=default;
};
struct CriticalProfileSnapshot {CriticalProfile profile;std::uint64_t revision=0,observed=0;};
inline std::optional<unsigned> critical_energy(const CriticalProfile& p){
 if(!p.valid())return {};return p.primary==7?(p.critical_strikes+2)/5:0;
}
// Local Damage_calculation formula43: a community model, not recovered retail
// RNG. Item penalty must be known; callers cannot substitute zero for unknown.
inline std::optional<double> critical_chance(const CriticalProfile& source,unsigned target_level,unsigned mastery,double item_penalty){
 if(!source.valid()||!target_level||target_level>255||mastery>20||!std::isfinite(item_penalty)||item_penalty<0||item_penalty>1)return {};
 const double level=source.level,w=mastery;
 const auto weapon=.05*std::exp2((8*level+4*w+6*std::min(w,(level+4)/2)-15*target_level-100)/40)*(1-.01*w)+.01*w-item_penalty;
 // Critical Strikes is an additional independent chance, per local article.
 const auto inherent=source.primary==7?.01*source.critical_strikes:0.;
 return std::clamp(1-(1-std::clamp(weapon,0.,1.))*(1-inherent),0.,1.);
}
inline std::optional<bool> critical_sample(double chance,double draw){
 if(!std::isfinite(chance)||chance<0||chance>1||!std::isfinite(draw)||draw<0||draw>=1)return {};
 return draw<chance;
}
}
