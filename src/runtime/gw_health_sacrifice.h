#pragma once
#include "gw_skill_catalog_generated.h"
#include <algorithm>
#include <cmath>
#include <optional>
namespace fw::gw::skills::sacrifice {
struct Loss {double requested=0,lost=0,remaining=0;bool killed=false;};
// Unavoidable health loss, not damage/healing: armor and damage/heal modifiers
// do not participate. The world retains fractional health as for other costs.
inline std::optional<Loss> resolve(unsigned skill,double current,unsigned maximum){
 const auto* row=find_skill(skill);
 if(!row||!row->health_sacrifice||row->health_sacrifice>100||!maximum||maximum>1000000||
    !std::isfinite(current)||current<=0||current>maximum)return {};
 double requested=double(maximum)*row->health_sacrifice/100.;
 // Historical Blood is Power notes explicitly specify half-to-even rounding,
 // including zero sacrifice at1 maximum Health. Do not generalize that reviewed
 // skill rule to other sacrifices without evidence. Integer arithmetic avoids
 // floating-point rounding-mode dependence at exact halves.
 if(skill==119){
  const auto numerator=std::uint64_t(maximum)*row->health_sacrifice;
  const auto whole=numerator/100,remainder=numerator%100;
  requested=double(whole+(remainder>50||(remainder==50&&(whole&1))));
 }
 const double lost=std::min(requested,current+1.);
 return Loss{requested,lost,std::max(0.,current-lost),lost>=current};
}
}
