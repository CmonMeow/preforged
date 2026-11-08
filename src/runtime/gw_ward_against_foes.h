#pragma once
// Detached Ward177 movement policy. World owns allegiance, lifetimes and wire.
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace fw::gw::skills::effects {
struct WardAgainstFoes {
    Scope caster;
    unsigned effect=0,rank=0,plane=0;
    float x=0,y=0;
    Time created=0,expires=0;
    static constexpr float radius=322.f;
    static constexpr float movement_multiplier=.5f;
};
struct WardFoeRecipient {
    Scope scope;
    unsigned plane=0;
    float x=0,y=0;
    bool authoritative_foe=false,spirit=false,living=false;
};
inline bool valid_ward_foes_scope(Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
inline std::optional<WardAgainstFoes> ward_against_foes(Scope caster,unsigned effect,
        unsigned rank,unsigned plane,float x,float y,Time now){
    const auto* row=find_skill(177);
    if(!row||row->type!=11||row->profession!=6||row->attribute!=9||row->special||
       row->energy_cost()!=15||row->health_sacrifice||row->activation!=1.f||
       row->aftercast!=.75f||row->recharge!=20||row->duration0!=8||row->duration15!=20||
       row->scale0!=50||row->scale15!=50||row->caster_body_resource!=316||
       row->target_body_resource!=2077||!valid_ward_foes_scope(caster)||!effect||rank>20||
       !std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1000000||std::abs(y)>1000000)return {};
    const Time duration=Time(interpolate(8,20,rank))*1000;
    if(now>std::numeric_limits<Time>::max()-duration)return {};
    return WardAgainstFoes{caster,effect,rank,plane,x,y,now,now+duration};
}
struct WardFoesPlan {unsigned rank=0;Time duration=0;auto operator<=>(const WardFoesPlan&)const=default;};
inline std::optional<WardFoesPlan> ward_foes_plan(unsigned rank){
    const auto ward=ward_against_foes({1,1,1},1,rank,0,0,0,0);
    if(!ward)return {};return WardFoesPlan{rank,ward->expires};
}
inline bool valid_ward_against_foes(const WardAgainstFoes& ward){
    const auto canonical=ward_against_foes(ward.caster,ward.effect,ward.rank,
        ward.plane,ward.x,ward.y,ward.created);
    return canonical&&canonical->expires==ward.expires;
}
inline bool ward_foes_covers(const WardAgainstFoes& ward,
        const WardFoeRecipient& recipient,Time now){
    // 322 is the archived in-the-area convention; same-plane exclusion is a
    // private navigation barrier, not a recovered retail vertical rule.
    if(!valid_ward_against_foes(ward)||!valid_ward_foes_scope(recipient.scope)||
       ward.caster.instance!=recipient.scope.instance||ward.plane!=recipient.plane||
       !recipient.authoritative_foe||recipient.spirit||!recipient.living||
       !std::isfinite(recipient.x)||!std::isfinite(recipient.y)||
       now<ward.created||now>=ward.expires)return false;
    const double dx=double(recipient.x)-ward.x,dy=double(recipient.y)-ward.y;
    return dx*dx+dy*dy<=double(WardAgainstFoes::radius)*WardAgainstFoes::radius;
}
inline std::optional<float> ward_foes_movement_multiplier(const WardAgainstFoes& ward,
        const WardFoeRecipient& recipient,Time now){
    if(!ward_foes_covers(ward,recipient,now))return {};
    return WardAgainstFoes::movement_multiplier;
}

// Normalized boundaries of the portion of a straight segment inside the ward.
// World can split movement precisely at entry and exit rather than slowing an
// entire route edge. Expiry and other speed modifiers are separate time rules.
struct WardFoeSegment {double enter=0,leave=0;};
inline std::optional<WardFoeSegment> ward_foes_segment_interval(const WardAgainstFoes& ward,
        const WardFoeRecipient& recipient,float end_x,float end_y,Time now){
    if(!valid_ward_against_foes(ward)||!valid_ward_foes_scope(recipient.scope)||
       ward.caster.instance!=recipient.scope.instance||ward.plane!=recipient.plane||
       !recipient.authoritative_foe||recipient.spirit||!recipient.living||
       now<ward.created||now>=ward.expires||!std::isfinite(recipient.x)||
       !std::isfinite(recipient.y)||!std::isfinite(end_x)||!std::isfinite(end_y))return {};
    const double sx=double(recipient.x)-ward.x,sy=double(recipient.y)-ward.y;
    const double dx=double(end_x)-recipient.x,dy=double(end_y)-recipient.y;
    const double a=dx*dx+dy*dy,r=WardAgainstFoes::radius;
    if(a==0)return WardFoeSegment{};
    const double b=2*(sx*dx+sy*dy),c=sx*sx+sy*sy-r*r;
    const double discriminant=b*b-4*a*c;
    if(discriminant<=0)return c<0?WardFoeSegment{0.,1.}:WardFoeSegment{};
    const double root=std::sqrt(discriminant);
    const double enter=std::clamp((-b-root)/(2*a),0.,1.);
    const double leave=std::clamp((-b+root)/(2*a),0.,1.);
    return WardFoeSegment{enter,std::max(enter,leave)};
}
inline std::optional<double> ward_foes_segment_fraction(const WardAgainstFoes& ward,
        const WardFoeRecipient& recipient,float end_x,float end_y,Time now){
    const auto interval=ward_foes_segment_interval(ward,recipient,end_x,end_y,now);
    if(!interval)return {};
    return interval->leave-interval->enter;
}
// Travel time for one straight segment and one active ward, with an expiry
// boundary that may occur before entry or while crossing the field. This is a
// movement-policy building block; World must compose other snares and wards.
inline std::optional<double> ward_foes_segment_travel_ms(const WardAgainstFoes& ward,
        const WardFoeRecipient& recipient,float end_x,float end_y,Time now,
        double base_speed_per_second){
    if(!std::isfinite(base_speed_per_second)||base_speed_per_second<=0)return {};
    const auto interval=ward_foes_segment_interval(ward,recipient,end_x,end_y,now);
    if(!interval)return {};
    const double length=std::hypot(double(end_x)-recipient.x,double(end_y)-recipient.y);
    const double before=length*interval->enter,inside=length*(interval->leave-interval->enter);
    const double after=length*(1.-interval->leave);
    const double before_ms=before/base_speed_per_second*1000.;
    const double active_ms=std::max(0.,double(ward.expires-now)-before_ms);
    const double slow_distance=std::min(inside,base_speed_per_second*
        WardAgainstFoes::movement_multiplier*active_ms/1000.);
    return before_ms+slow_distance/(base_speed_per_second*
        WardAgainstFoes::movement_multiplier)*1000.+
        (inside-slow_distance+after)/base_speed_per_second*1000.;
}
}
