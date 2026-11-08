#pragma once
#include "gw_ward_against_harm.h"
#include <vector>
namespace fw::gw::skills::effects {
// A caller supplies already-authoritative straight movement legs, including
// stops, control pauses and plane/allegiance changes. This module owns no
// World clocks, routes, Health, condition effects or regeneration caps.
struct TimedGroundAreaLeg {GroundAreaRecipient recipient;float end_x=0,end_y=0;double from=0,through=0;};
struct WardRegenerationBand {double from=0,through=0;unsigned pips=0;};
inline std::optional<std::vector<WardRegenerationBand>> ward_harm_regeneration_bands(
        std::span<const WardAgainstHarm> wards,std::span<const TimedGroundAreaLeg> legs){
 constexpr double exact_time=9007199254740991.;
 if(wards.size()>4096||legs.size()>4096)return {};
 for(const auto& ward:wards)if(!valid_ward_harm(ward)||double(ward.area.expires)>exact_time)return {};
 std::vector<WardRegenerationBand> out;std::size_t work=0;
 const TimedGroundAreaLeg* previous=nullptr;
 for(const auto& leg:legs){
  if(!ground_area_scope(leg.recipient.scope)||!std::isfinite(leg.recipient.x)||!std::isfinite(leg.recipient.y)||
     !std::isfinite(leg.end_x)||!std::isfinite(leg.end_y)||!std::isfinite(leg.from)||!std::isfinite(leg.through)||
     leg.from<0||leg.through<leg.from||leg.through>exact_time)return {};
  if(previous&&(previous->through!=leg.from||previous->end_x!=leg.recipient.x||previous->end_y!=leg.recipient.y||
      previous->recipient.scope!=leg.recipient.scope))return {};
  previous=&leg;if(leg.from==leg.through)continue;
  std::vector<WardRegenerationBand> intervals;std::vector<double> cuts{leg.from,leg.through};
  for(const auto& ward:wards){
   if(++work>262144)return {};
   const auto range=ground_area_segment_interval(ward.area,leg.recipient,leg.end_x,leg.end_y);if(!range)continue;
   const double from=std::max(leg.from+(leg.through-leg.from)*range->enter,double(ward.area.created));
   const double through=std::min(leg.from+(leg.through-leg.from)*range->leave,double(ward.area.expires));
   if(through<=from)continue;
   intervals.push_back({from,through,ward.plan.regeneration});cuts.push_back(from);cuts.push_back(through);
  }
  std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
  for(std::size_t i=1;i<cuts.size();++i){
   if(cuts[i]<=cuts[i-1])continue;
   const double midpoint=cuts[i-1]+(cuts[i]-cuts[i-1])/2;unsigned pips=0;
   for(const auto& interval:intervals){if(++work>262144)return {};
    if(midpoint>=interval.from&&midpoint<interval.through)pips=std::max(pips,interval.pips);}
   if(!out.empty()&&out.back().through==cuts[i-1]&&out.back().pips==pips)out.back().through=cuts[i];
   else out.push_back({cuts[i-1],cuts[i],pips});
  }
 }
 return out;
}
}
