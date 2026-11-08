#pragma once
// Detached Ward177 route integrator. No World, actor, navigation or timer owner.
#include "gw_ward_against_foes.h"
#include <functional>
#include <span>
#include <vector>

namespace fw::gw::skills::effects {
// Each leg has an explicitly supplied plane. Callers validate connectivity and
// any plane transition through their navigation authority before invoking this.
struct WardFoeRouteLeg {float x=0,y=0,end_x=0,end_y=0;unsigned plane=0;};
struct WardFoeRouteTravel {
 double distance=0,elapsed_ms=0,x=0,y=0;unsigned plane=0;bool reached=false;
};
// Return units/second after composing the external Ward factor BEFORE movement
// floors/caps. The callback is piecewise constant between supplied boundaries;
// it also owns pauses/knockdown. Multiplying an already-clamped speed is unsafe.
using WardFoeRouteSpeed=std::function<std::optional<double>(double at_ms,float ward_factor)>;
inline std::optional<WardFoeRouteTravel> ward_foes_route_travel(
 const WardFoeRecipient& recipient,std::span<const WardAgainstFoes> wards,
 std::span<const WardFoeRouteLeg> route,double from,double through,
 std::span<const double> speed_boundaries,const WardFoeRouteSpeed& speed){
 constexpr double exact_time_limit=9007199254740991.;
 if(!valid_ward_foes_scope(recipient.scope)||!std::isfinite(recipient.x)||!std::isfinite(recipient.y)||
    !std::isfinite(from)||!std::isfinite(through)||from<0||through<from||through>exact_time_limit||
    wards.size()>4096||route.size()>4096||speed_boundaries.size()>65536||!speed)return {};
 std::vector<double> times{through};
 for(double at:speed_boundaries){if(!std::isfinite(at)||at<0||at>exact_time_limit)return {};
  if(at>from&&at<through)times.push_back(at);}
 for(const auto& ward:wards){
  if(!valid_ward_against_foes(ward)||ward.expires>std::uint64_t(exact_time_limit))return {};
  for(const auto at:{ward.created,ward.expires})if(double(at)>from&&double(at)<through)times.push_back(double(at));
 }
 float prior_x=recipient.x,prior_y=recipient.y;
 for(const auto& leg:route){
  if(!std::isfinite(leg.x)||!std::isfinite(leg.y)||!std::isfinite(leg.end_x)||!std::isfinite(leg.end_y)||
     leg.x!=prior_x||leg.y!=prior_y)return {};
  prior_x=leg.end_x;prior_y=leg.end_y;
 }
 if(!route.empty()&&route.front().plane!=recipient.plane)return {};
 std::sort(times.begin(),times.end());times.erase(std::unique(times.begin(),times.end()),times.end());
 WardFoeRouteTravel out{0,0,recipient.x,recipient.y,recipient.plane,false};double cursor=from;
 std::size_t work=0;
 for(const auto& leg:route){
  const double dx=double(leg.end_x)-leg.x,dy=double(leg.end_y)-leg.y,length=std::hypot(dx,dy);
  out.plane=leg.plane;
  if(length==0){out.x=leg.end_x;out.y=leg.end_y;continue;}
  struct Band {double enter,leave;Time created,expires;};std::vector<Band> bands;
  std::vector<double> cuts{0,length};auto sample=recipient;sample.x=leg.x;sample.y=leg.y;sample.plane=leg.plane;
  for(const auto& ward:wards){
   if(++work>262144)return {};
   // Caller prefilters each ward's allegiance. The shared recipient flags
   // represent that already-established relation, never a new team inference.
   const auto interval=ward_foes_segment_interval(ward,sample,leg.end_x,leg.end_y,ward.created);
   if(!interval||interval->leave<=interval->enter)continue;
   const double enter=length*interval->enter,leave=length*interval->leave;
   bands.push_back({enter,leave,ward.created,ward.expires});cuts.push_back(enter);cuts.push_back(leave);
  }
  std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
  for(std::size_t cell=1;cell<cuts.size();++cell){
   double distance=cuts[cell-1];const double end=cuts[cell],middle=(distance+end)/2;
   while(distance<end){
    if(cursor>=through){out.elapsed_ms=cursor-from;return out;}
    if(++work>262144)return {}; // Fail closed on excessive integration work.
    bool warded=false;
    for(const auto& band:bands){
     if(++work>262144)return {};
     if(middle>band.enter&&middle<band.leave&&cursor>=double(band.created)&&cursor<double(band.expires)){warded=true;break;}
    }
    // Same-skill overlap supplies ONE .5 factor. This explicit private policy
    // does not assert recovered retail overlap/other-snare stacking behavior.
    const auto rate=speed(cursor,warded?WardAgainstFoes::movement_multiplier:1.f);
    if(!rate||!std::isfinite(*rate)||*rate<0||*rate>1000000)return {};
    const auto next=std::upper_bound(times.begin(),times.end(),cursor);
    if(next==times.end())return {};
    const double available=*next-cursor,needed=*rate>0?(end-distance)*1000. / *rate:std::numeric_limits<double>::infinity();
    double moved=0;
    // Geometric roots can leave sub-nanosecond roundoff at an
    // otherwise exact time/edge intersection. Snap only that numeric residue.
    if(needed<=available||(std::isfinite(needed)&&needed-available<=1e-9)){
     moved=end-distance;distance=end;cursor=std::min(*next,cursor+needed);
    }
    else {moved=std::min(end-distance,*rate*available/1000.);distance+=moved;cursor=*next;}
    out.distance+=moved;out.x=leg.x+dx*distance/length;out.y=leg.y+dy*distance/length;
   }
  }
  out.x=leg.end_x;out.y=leg.end_y;
 }
 out.elapsed_ms=cursor-from;out.reached=true;return out;
}
}
