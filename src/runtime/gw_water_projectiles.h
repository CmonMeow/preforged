#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_projectile_flight.h"
#include "gw_effect_registry.h"
#include "gw_combat.h"
#include "gw_conditions.h"
namespace fw::gw::skills::effects {
// Reviewed native rows and historical wiki mechanics. Speeds are capture-based
// calibration hypotheses; hit tolerance remains the shared simulation policy.
struct WaterProjectilePlan {
    unsigned skill=0,rank=0,model=0,maximum_targets=1;
    double damage=0;
    float range=1248.f,secondary_radius=0,movement_multiplier=1;
    std::uint64_t snare_ms=0,overcast_regeneration_ms=0,moving_knockdown_ms=0;
    unsigned overcast_regeneration_pips=0;
    projectiles::Policy flight;
};
inline std::optional<WaterProjectilePlan> water_projectile(unsigned skill,unsigned rank){
    if(rank>20)return {};
    WaterProjectilePlan p;p.skill=skill;p.rank=rank;
    if(skill==213){p.model=378;p.damage=damage_scale(10,85,rank);p.flight.speed=1800;
        p.movement_multiplier=.34f;p.snare_ms=std::uint64_t(interpolate(2,6,rank))*1000;}
    else if(skill==214){p.model=379;p.damage=damage_scale(10,60,rank);p.flight.speed=1200;
        p.overcast_regeneration_pips=interpolate(1,4,rank);p.overcast_regeneration_ms=5000;}
    else if(skill==237){p.model=417;p.damage=damage_scale(10,90,rank);p.flight.speed=2400;
        p.maximum_targets=3;p.secondary_radius=166;p.moving_knockdown_ms=2000;}
    else return {};
    return p;
}
inline bool valid_water_projectile(const WaterProjectilePlan& p){
    const auto expected=water_projectile(p.skill,p.rank);
    if(!expected)return false;
    // Policy intentionally has no equality operator: compare every reviewed field.
    return p.skill==expected->skill&&p.rank==expected->rank&&p.model==expected->model&&
        p.maximum_targets==expected->maximum_targets&&p.damage==expected->damage&&p.range==expected->range&&
        p.secondary_radius==expected->secondary_radius&&p.movement_multiplier==expected->movement_multiplier&&
        p.snare_ms==expected->snare_ms&&p.overcast_regeneration_ms==expected->overcast_regeneration_ms&&
        p.moving_knockdown_ms==expected->moving_knockdown_ms&&p.overcast_regeneration_pips==expected->overcast_regeneration_pips&&
        p.flight.speed==expected->flight.speed&&p.flight.hit_radius==expected->flight.hit_radius;
}
struct WaterProjectileTarget {
    Scope scope;double x=0,y=0;std::uint16_t plane=0;
    bool alive=false,hostile=false,moving=false;
};
struct WaterProjectileImpact {Scope caster,target;WaterProjectilePlan plan;bool moving=false;Time at=0;};
struct WaterProjectileResult {
    bool accepted=false;combat::Actor actor;combat::Hit hit;
    std::uint64_t snare_ms=0,knockdown_ms=0;
};
// Pure impact proposal. World owns Hex Breaker triggering, duration modifiers,
// effect IDs, knockdown protections and damage/death publication. Regeneration
// belongs to Ice Spear's cast completion, never this potentially missed impact.
inline WaterProjectileResult water_projectile_target(const WaterProjectileImpact& impact,
        combat::Actor actor,const combat::conditions::State& conditions,unsigned level,
        bool hex_prevented=false,std::optional<bool> allows_knockdown=std::nullopt,const combat::DamageProjection& projection={}){
    const auto& p=impact.plan;
    if(!valid_water_projectile(p)||!impact.caster.instance||!impact.caster.actor||!impact.caster.generation||
       impact.target.instance!=impact.caster.instance||impact.target.actor==impact.caster.actor||
       !impact.target.generation||actor.id!=impact.target.actor||actor.generation!=impact.target.generation||
       !actor.cold_armor)return {};
    actor.cold_armor=conditions.armor(*actor.cold_armor,impact.at);
    const auto step=combat::project_damage(actor,{combat::Kind::cold,p.damage,level,p.skill},projection);
    if(!step)return {};
    const auto hit=step->incoming;
    WaterProjectileResult result{true,actor,hit};
    if(actor.health->current<=0)return result;
    if(p.snare_ms&&!hex_prevented){
        if(impact.at>UINT64_MAX-p.snare_ms)return {};
        result.snare_ms=p.snare_ms;
    }
    if(p.moving_knockdown_ms&&impact.moving){
        if(!allows_knockdown||impact.at>UINT64_MAX-p.moving_knockdown_ms)return {};
        if(*allows_knockdown)result.knockdown_ms=p.moving_knockdown_ms;
    }
    return result;
}
// One immutable launch per selected recipient (including Trident secondaries).
// Selection and damage remain World responsibilities. A released spell survives
// caster death, but never departure/reuse of either scoped actor identity.
class WaterProjectileFlight {
    Scope caster_,target_;WaterProjectilePlan plan_;projectiles::Aim aim_{};
    std::uint16_t plane_=0;Time arrives_=0,last_=0;bool pending_=false;
public:
    bool launch(Scope caster,const WaterProjectileTarget& target,const WaterProjectilePlan& plan,
            double sx,double sy,std::uint16_t plane,double vx,double vy,Time now,bool secondary=false){
        if(pending_||!valid_water_projectile(plan)||!caster.instance||!caster.actor||!caster.generation||
           target.scope.instance!=caster.instance||!target.scope.actor||!target.scope.generation||
           target.scope.actor==caster.actor||!target.alive||!target.hostile||plane!=target.plane||
           !std::isfinite(sx)||!std::isfinite(sy)||!std::isfinite(target.x)||!std::isfinite(target.y)||
           (secondary&&plan.skill!=237)||std::hypot(target.x-sx,target.y-sy)>plan.range+(secondary?plan.secondary_radius:0))return false;
        const auto aim=projectiles::intercept(plan.flight,sx,sy,target.x,target.y,vx,vy);
        if(!aim||now>UINT64_MAX-aim->flight_ms)return false;
        caster_=caster;target_=target.scope;plan_=plan;aim_=*aim;plane_=plane;
        arrives_=now+aim->flight_ms;last_=now;pending_=true;return true;
    }
    bool pending()const{return pending_;}
    const WaterProjectilePlan& plan()const{return plan_;}
    const projectiles::Aim& aim()const{return aim_;}
    Time arrives()const{return arrives_;}
    void cancel(){pending_=false;}
    std::optional<WaterProjectileImpact> impact(Scope caster,const WaterProjectileTarget& target,Time now){
        if(!pending_)return {};
        if(caster!=caster_||target.scope!=target_||now<last_){cancel();return {};}
        last_=now;if(now<arrives_)return {};
        cancel();
        if(!target.alive||!target.hostile||target.plane!=plane_||!std::isfinite(target.x)||!std::isfinite(target.y)||
           std::hypot(target.x-aim_.x,target.y-aim_.y)>plan_.flight.hit_radius)return {};
        return WaterProjectileImpact{caster_,target_,plan_,target.moving,arrives_};
    }
};
}
