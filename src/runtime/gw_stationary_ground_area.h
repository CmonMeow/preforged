#pragma once
#include "gw_effect_registry.h"
#include <cmath>
#include <optional>
namespace fw::gw::skills::effects {
// Geometry/lifetime authority shared by stationary beneficial fields. Skill
// plans validate their own radius/duration before constructing this record.
struct StationaryGroundArea {
 Scope caster;unsigned effect=0;std::uint64_t team=0;unsigned plane=0;
 float x=0,y=0,radius=0;Time created=0,expires=0;
};
struct GroundAreaRecipient {
 Scope scope;std::uint64_t team=0;unsigned plane=0;float x=0,y=0;
 bool living=false,spirit=false;
};
inline bool ground_area_scope(Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
inline std::optional<StationaryGroundArea> stationary_ground_area(Scope caster,unsigned effect,
        std::uint64_t team,unsigned plane,float x,float y,float radius,Time duration,Time now){
 if(!ground_area_scope(caster)||!effect||!team||plane>65535||!duration||now>UINT64_MAX-duration||
    !std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1000000||std::abs(y)>1000000||
    !std::isfinite(radius)||radius<=0||radius>1000000)return {};
 return StationaryGroundArea{caster,effect,team,plane,x,y,radius,now,now+duration};
}
inline bool valid_ground_area(const StationaryGroundArea& area){
 return area.expires>area.created&&bool(stationary_ground_area(area.caster,area.effect,area.team,area.plane,
     area.x,area.y,area.radius,area.expires-area.created,area.created));
}
inline bool ground_area_recipient(const StationaryGroundArea& area,const GroundAreaRecipient& recipient){
 return valid_ground_area(area)&&ground_area_scope(recipient.scope)&&recipient.scope.instance==area.caster.instance&&
     recipient.team&&recipient.team==area.team&&recipient.plane==area.plane&&recipient.living&&!recipient.spirit&&
     std::isfinite(recipient.x)&&std::isfinite(recipient.y);
}
inline bool ground_area_covers(const StationaryGroundArea& area,const GroundAreaRecipient& recipient,Time now){
 if(!ground_area_recipient(area,recipient)||now<area.created||now>=area.expires)return false;
 const auto dx=double(recipient.x)-area.x,dy=double(recipient.y)-area.y;
 return dx*dx+dy*dy<=double(area.radius)*area.radius;
}
struct GroundAreaSegmentInterval {double enter=0,leave=0;};
// Spatial fractions only. Time clipping belongs to the effect consumer; this
// routine never treats a tangent's zero-measure contact as a whole interval.
inline std::optional<GroundAreaSegmentInterval> ground_area_segment_interval(const StationaryGroundArea& area,
        const GroundAreaRecipient& from,float end_x,float end_y){
 if(!ground_area_recipient(area,from)||!std::isfinite(end_x)||!std::isfinite(end_y))return {};
 const double x=double(from.x)-area.x,y=double(from.y)-area.y;
 const double dx=double(end_x)-from.x,dy=double(end_y)-from.y,a=dx*dx+dy*dy;
 const double c=x*x+y*y-double(area.radius)*area.radius;
 if(a==0)return c<=0?std::optional(GroundAreaSegmentInterval{0,1}):std::nullopt;
 const double b=2*(x*dx+y*dy),discriminant=b*b-4*a*c;
 if(discriminant<0)return {};
 const double root=std::sqrt(discriminant);
 const double enter=std::max(0.,(-b-root)/(2*a)),leave=std::min(1.,(-b+root)/(2*a));
 if(leave<enter||leave<0||enter>1)return {};
 return GroundAreaSegmentInterval{enter,leave};
}

}
