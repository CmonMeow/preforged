#pragma once
#include "gw_well_corpse_cast.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace fw::gw::skills::effects {
// Rank-0/15/20 anchors come from the local historical wiki. Intermediate
// rounding and the area radius are server conventions pending retail evidence.
struct WellFieldPlan {
    WellCastPlan cast;
    Time duration=0;
    int health_pips=0,energy_pips=0;
    bool affects_allies=false,strip_enchantments=false,block_enchantments=false;
    unsigned low_rank_failure_percent=0;
    float radius=322.f;
};
inline int well_anchor(unsigned rank,int at_zero,int at_fifteen,int at_twenty){
    if(rank<=15)return static_cast<int>(std::lround(at_zero+(at_fifteen-at_zero)*double(rank)/15.));
    return static_cast<int>(std::lround(at_fifteen+(at_twenty-at_fifteen)*double(rank-15)/5.));
}
inline std::optional<WellFieldPlan> well_field_plan(unsigned skill,unsigned rank){
    const auto cast=well_cast_plan(skill,rank);if(!cast)return {};
    WellFieldPlan result;result.cast=*cast;
    switch(skill){
    case 91:result.duration=1000ull*well_anchor(rank,8,18,20);
        result.health_pips=well_anchor(rank,1,5,6);result.energy_pips=2;result.affects_allies=true;break;
    case 92:result.duration=1000ull*well_anchor(rank,8,18,20);
        result.health_pips=well_anchor(rank,1,5,6);result.affects_allies=true;break;
    case 93:result.duration=1000ull*well_anchor(rank,10,26,30);
        result.health_pips=-well_anchor(rank,1,5,6);break;
    case 94:result.duration=1000ull*well_anchor(rank,8,18,20);
        result.strip_enchantments=result.block_enchantments=true;
        result.low_rank_failure_percent=rank<=4?50:0;break;
    default:return {};
    }
    return result;
}
struct WellField {
    WellFieldPlan plan;
    Scope caster,corpse;
    std::uint64_t team=0;
    unsigned plane=0;
    float x=0,z=0;
    Time begins=0,expires=0;
};
inline std::optional<WellField> create_well_field(const WellCorpseClaim& claim,
                                                    std::uint64_t team){
    const auto plan=well_field_plan(claim.plan.skill,claim.plan.rank);
    if(!plan||plan->cast!=claim.plan||!team||!valid_well_scope(claim.caster)||
       !valid_well_scope(claim.corpse)||!claim.completed||
       claim.completed>UINT64_MAX-plan->duration||claim.plane>65535||
       !std::isfinite(claim.x)||!std::isfinite(claim.z))return {};
    return WellField{*plan,claim.caster,claim.corpse,team,claim.plane,
                     claim.x,claim.z,claim.completed,claim.completed+plan->duration};
}
struct WellFieldEffect {int health_pips=0,energy_pips=0;bool strip_enchantments=false,block_enchantments=false;};
struct WellRouteLeg {
    unsigned plane=0;
    float from_x=0,from_z=0,to_x=0,to_z=0;
    Time begins=0,ends=0;
};
struct WellRouteInterval {double enter=0,leave=0;}; // Fractions of one leg.
// Exact straight-leg/circle crossing, clipped to the field's live interval.
// World supplies its authoritative route legs; this never trusts client path.
inline std::optional<WellRouteInterval> well_route_interval(const WellField& field,
                                                            const WellRouteLeg& leg,
                                                            std::uint64_t team){
    if(!team||(team==field.team)!=field.plan.affects_allies||leg.plane!=field.plane||
       leg.ends<=leg.begins||!std::isfinite(leg.from_x)||!std::isfinite(leg.from_z)||
       !std::isfinite(leg.to_x)||!std::isfinite(leg.to_z)||leg.ends<=field.begins||
       leg.begins>=field.expires)return {};
    const long double duration=leg.ends-leg.begins;
    double enter=static_cast<double>(std::max<long double>(0,
        (static_cast<long double>(field.begins)-leg.begins)/duration));
    double leave=static_cast<double>(std::min<long double>(1,
        (static_cast<long double>(field.expires)-leg.begins)/duration));
    if(enter>=leave)return {};
    const double dx=double(leg.to_x)-leg.from_x,dz=double(leg.to_z)-leg.from_z;
    const double ox=double(leg.from_x)-field.x,oz=double(leg.from_z)-field.z;
    const double a=dx*dx+dz*dz,b=2*(ox*dx+oz*dz);
    const double c=ox*ox+oz*oz-double(field.plan.radius)*field.plan.radius;
    if(a==0){if(c>0)return {};return WellRouteInterval{enter,leave};}
    const double discriminant=b*b-4*a*c;
    if(discriminant<=0)return {};
    const double root=std::sqrt(discriminant);
    enter=std::max(enter,(-b-root)/(2*a));
    leave=std::min(leave,(-b+root)/(2*a));
    if(enter>=leave)return {};
    return WellRouteInterval{enter,leave};
}
struct WellRouteSegment {
    double enter=0,leave=0;
    WellFieldEffect effect;
};
// Sweep active-field entry/exit events. Do not clamp pips here: World combines
// conditions, enchantments, equipment and Well pips before the global cap.
inline std::vector<WellRouteSegment> well_route_segments(std::span<const WellField> fields,
                                                          const WellRouteLeg& leg,
                                                          std::uint64_t team){
    struct Event {double at;int health,energy,strip,block;};
    std::vector<Event> events;
    events.reserve(fields.size()*2);
    for(const auto& field:fields){
        const auto interval=well_route_interval(field,leg,team);
        if(!interval)continue;
        const auto& p=field.plan;
        const Event start{interval->enter,p.health_pips,p.energy_pips,
            p.strip_enchantments?1:0,p.block_enchantments?1:0};
        events.push_back(start);
        events.push_back({interval->leave,-start.health,-start.energy,-start.strip,-start.block});
    }
    std::sort(events.begin(),events.end(),[](const Event& a,const Event& b){return a.at<b.at;});
    std::vector<WellRouteSegment> result;
    int health=0,energy=0,strip=0,block=0;double previous=0;
    for(std::size_t i=0;i<events.size();){
        const double at=events[i].at;
        if(previous<at)result.push_back({previous,at,{health,energy,strip>0,block>0}});
        while(i<events.size()&&events[i].at==at){
            health+=events[i].health;energy+=events[i].energy;
            strip+=events[i].strip;block+=events[i].block;++i;
        }
        previous=at;
    }
    if(previous<1)result.push_back({previous,1,{health,energy,strip>0,block>0}});
    return result;
}
inline WellFieldEffect well_field_effect(const WellField& field,unsigned plane,
                                        float x,float z,std::uint64_t team,Time now){
    if(!team||!std::isfinite(x)||!std::isfinite(z)||plane!=field.plane||
       now<field.begins||now>=field.expires||
       std::hypot(double(x)-field.x,double(z)-field.z)>field.plan.radius)return {};
    const bool ally=team==field.team;
    if(ally!=field.plan.affects_allies)return {};
    return {field.plan.health_pips,field.plan.energy_pips,
            field.plan.strip_enchantments,field.plan.block_enchantments};
}
}
