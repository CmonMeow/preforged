#pragma once
#include "gw_lightning.h"
#include <cmath>
#include <optional>
#include "gw_effect_registry.h"
#include <span>
#include <set>
#include <algorithm>
#include "gw_projectile_flight.h"
namespace fw::gw::skills::effects {
// Pre-Reforged Lightning_Javelin wiki and pinned row230. Kept separate from
// lightning() eligibility until the world supports swept hits and attack-only
// interruption. Do not enable an endpoint-only approximation of this skill.
struct LightningJavelinPlan {
    unsigned rank=0;double damage=0;unsigned penetration=25;
    float range=1248;bool interrupts_attacks=true;
    auto operator<=>(const LightningJavelinPlan&)const=default;
};
inline std::optional<LightningJavelinPlan> lightning_javelin(unsigned rank){
    if(rank>20)return {};
    const auto* row=find_skill(230);
    if(!row||row->type!=5||row->attribute!=8)return {};
    return LightningJavelinPlan{rank,damage_scale(row->scale0,row->scale15,rank)};
}
// Caller supplies the reviewed projectile collision radius; no unmeasured
// retail width is guessed here. Returns along-path fraction for ordered hits.
// Only the segment between origin and aim qualifies, not its infinite line.
inline std::optional<double> javelin_path_hit(double ox,double oy,double ax,double ay,
        double x,double y,double radius){
    if(!std::isfinite(ox)||!std::isfinite(oy)||!std::isfinite(ax)||!std::isfinite(ay)||
       !std::isfinite(x)||!std::isfinite(y)||!std::isfinite(radius)||radius<0)return {};
    const double dx=ax-ox,dy=ay-oy,length=dx*dx+dy*dy;
    if(!std::isfinite(length)||length<=0)return {};
    const double along=((x-ox)*dx+(y-oy)*dy)/length;
    if(!std::isfinite(along)||along<0||along>1)return {};
    const double distance=std::hypot(x-(ox+along*dx),y-(oy+along*dy));
    if(!std::isfinite(distance)||distance>radius)return {};
    return along;
}
struct JavelinCandidate {Scope scope;double x=0,y=0;bool alive=false,hostile=false;};
struct JavelinHit {Scope scope;double along=0;};
// Wiki distinguishes the selected foe (must actually be hit at the aim point)
// from other foes adjacent to the flight path/end point. Positions are sampled
// by the world at impact. Radius/speed are explicit simulation policy until
// calibrated; this planner does not advertise a bound native skill handler.
inline std::optional<std::vector<JavelinHit>> javelin_hits(Scope caster,Scope target,
        double ox,double oy,double ax,double ay,double adjacent_radius,double target_radius,
        std::span<const JavelinCandidate> candidates){
    if(!caster.instance||!caster.actor||!caster.generation||target.instance!=caster.instance||
       !target.actor||!target.generation||target==caster||candidates.size()>4096||
       !std::isfinite(adjacent_radius)||adjacent_radius<0||adjacent_radius>5000||
       !std::isfinite(target_radius)||target_radius<0||target_radius>adjacent_radius||
       !javelin_path_hit(ox,oy,ax,ay,ax,ay,0))return {};
    std::set<Scope> seen;std::vector<JavelinHit> hits;
    const double dx=ax-ox,dy=ay-oy,length=dx*dx+dy*dy;
    for(const auto& c:candidates){
        if(!c.scope.actor||!c.scope.generation||c.scope.instance!=caster.instance||
           !std::isfinite(c.x)||!std::isfinite(c.y)||!seen.insert(c.scope).second)return {};
        if(c.scope==caster||!c.alive||!c.hostile)continue;
        const double endpoint=std::hypot(c.x-ax,c.y-ay);
        if(c.scope==target){if(endpoint<=target_radius)hits.push_back({c.scope,1});continue;}
        const auto along=javelin_path_hit(ox,oy,ax,ay,c.x,c.y,adjacent_radius);
        if(along){hits.push_back({c.scope,*along});continue;}
        // Endpoint bubble is forward-facing; do not include foes behind caster.
        const double projection=((c.x-ox)*dx+(c.y-oy)*dy)/length;
        if(std::isfinite(projection)&&projection>0&&endpoint<=adjacent_radius)hits.push_back({c.scope,1});
    }
    std::sort(hits.begin(),hits.end(),[](const auto& a,const auto& b){return a.along!=b.along?a.along<b.along:a.scope<b.scope;});
    return hits;
}
class JavelinFlight {
    Scope caster_,target_;LightningJavelinPlan plan_;double ox_=0,oy_=0,ax_=0,ay_=0;
    double adjacent_radius_=0,target_radius_=0;Time arrives_=0,last_=0;bool pending_=false;
public:
    bool launch(Scope caster,Scope target,const LightningJavelinPlan& plan,
            projectiles::Policy policy,double adjacent_radius,double ox,double oy,
            double tx,double ty,double vx,double vy,Time now){
        const auto reviewed=lightning_javelin(plan.rank);
        if(pending_||!reviewed||*reviewed!=plan||!policy.valid()||
           !std::isfinite(adjacent_radius)||adjacent_radius<policy.hit_radius||adjacent_radius>5000||
           !caster.instance||!caster.actor||!caster.generation||target.instance!=caster.instance||
           !target.actor||!target.generation||target.actor==caster.actor||
           !std::isfinite(ox)||!std::isfinite(oy)||!std::isfinite(tx)||!std::isfinite(ty)||
           std::hypot(tx-ox,ty-oy)>plan.range)return false;
        const auto aim=projectiles::intercept(policy,ox,oy,tx,ty,vx,vy);
        if(!aim||now>UINT64_MAX-aim->flight_ms||!javelin_path_hit(ox,oy,aim->x,aim->y,aim->x,aim->y,0))return false;
        caster_=caster;target_=target;plan_=plan;ox_=ox;oy_=oy;ax_=aim->x;ay_=aim->y;
        adjacent_radius_=adjacent_radius;target_radius_=policy.hit_radius;
        arrives_=now+aim->flight_ms;last_=now;pending_=true;return true;
    }
    bool pending()const{return pending_;}
    Time arrives()const{return arrives_;}
    const LightningJavelinPlan& plan()const{return plan_;}
    void cancel(){pending_=false;}
    std::optional<std::vector<JavelinHit>> impact(Scope current_caster,bool caster_alive,
            Time now,std::span<const JavelinCandidate> candidates){
        if(!pending_)return {};
        if(current_caster!=caster_||!caster_alive||now<last_){cancel();return {};}
        last_=now;if(now<arrives_)return {};cancel();
        // A new lifetime occupying the selected raw actor ID must not become
        // collateral damage from the old projectile either.
        std::vector<JavelinCandidate> current;current.reserve(candidates.size());
        if(candidates.size()>4096)return {};
        for(const auto& candidate:candidates){
            if(candidate.scope.actor==target_.actor&&candidate.scope!=target_)continue;
            current.push_back(candidate);
        }
        return javelin_hits(caster_,target_,ox_,oy_,ax_,ay_,adjacent_radius_,target_radius_,current);
    }
};
}
