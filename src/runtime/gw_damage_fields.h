#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_ray_of_judgment.h"
#include "gw_snow_storm.h"
#include <cmath>
#include <map>
#include <optional>
namespace fw::gw::skills::effects {
enum class FieldDamage {fire,holy,cold,earth};
struct DamageFieldPlan {
    unsigned skill=0,rank=0,damage=0,pulses=0;float radius=166;FieldDamage kind=FieldDamage::fire;bool overcast=false;unsigned blind_ms=0,knockdown_ms=0,first_ms=1000,interval_ms=1000;
    unsigned burning_ms=0;
    auto operator<=>(const DamageFieldPlan&)const=default;
};
// Local Symbol_of_Wrath/Kirin's_Wrath, Lava_Font and Fire_Storm; pinned2022
// magnitudes. Lava Font's radius is fixed at activation, not evaluated per tick.
inline std::optional<DamageFieldPlan> damage_field(unsigned skill,unsigned rank=0,bool overcast=false){
    const auto* row=find_skill(skill);if(!row||rank>20||(overcast&&skill!=195))return {};
    if(skill==2222){
        const auto snow=snow_storm(rank);if(!snow)return {};
        return DamageFieldPlan{2222,rank,snow->damage,snow->pulses,snow->radius,FieldDamage::cold};
    }
    if(skill==187||skill==192){if(row->type!=5||row->attribute!=10||row->scale0!=7||row->scale15!=112)return {};
        return DamageFieldPlan{skill,rank,interpolate(7,112,rank),skill==187?1u:3u,166,FieldDamage::fire,false,0,2000,skill==187?500u:3000u,3000};}
    if(skill==167){if(row->type!=5||row->attribute!=9||row->scale0!=10||row->scale15!=40||row->bonus0!=10||row->bonus15!=10||row->duration0!=5||row->duration15!=5||row->energy_cost()!=25)return {};
        return DamageFieldPlan{167,rank,interpolate(10,40,rank),5,252,FieldDamage::earth,false,10000};}
    if(skill==215){if(row->type!=5||row->attribute!=11||row->scale0!=10||row->scale15!=25||row->duration0!=10||row->duration15!=10||row->energy_cost()!=25||row->overcast!=10)return {};
        return DamageFieldPlan{215,rank,interpolate(10,25,rank),10,166,FieldDamage::cold,false};}
    if(skill==197||skill==195||skill==1094)return DamageFieldPlan{skill,rank,interpolate(row->scale0,row->scale15,rank),skill==197?10u:5u,overcast?252.f:166.f,FieldDamage::fire,overcast};
    if(skill==247||skill==1113)return DamageFieldPlan{skill,rank,interpolate(row->scale0,row->scale15,rank),5,166,FieldDamage::holy,false};
    if(skill==830){
        const auto ray=ray_of_judgment(rank);
        if(!ray)return {};
        DamageFieldPlan field{skill,rank,ray->damage,ray->pulses,ray->adjacent,FieldDamage::holy};
        field.first_ms=static_cast<unsigned>(ray->interval_ms);
        field.interval_ms=static_cast<unsigned>(ray->interval_ms);
        field.burning_ms=ray->burning_seconds*1000;
        return field;
    }
    return {};
}
struct DamageFieldPulse {Scope caster;unsigned level;float x,y;std::uint16_t plane;DamageFieldPlan plan;Time at;};
// One queue for all fields gives chronological ordering across overlapping
// spells. Equal deadlines use creation order, an explicit private-server tie
// policy. Consuming the queue entry before dispatch prevents recursive replay.
class DamageFieldSchedule {
    struct Field {DamageFieldPulse pulse;unsigned remaining;std::uint64_t order;};
    std::map<std::pair<Time,std::uint64_t>,Field> pending_;
    std::uint64_t next_order_=1;Time clock_=0;
public:
    bool add(Scope caster,unsigned level,float x,float y,std::uint16_t plane,const DamageFieldPlan& plan,Time now){
        const auto expected=damage_field(plan.skill,plan.rank,plan.overcast);
        if(!expected||*expected!=plan||!caster.instance||!caster.actor||!caster.generation||!level||level>255||
           !std::isfinite(x)||!std::isfinite(y)||now<clock_||!plan.pulses||!plan.interval_ms||now>UINT64_MAX-Time(plan.first_ms)-Time(plan.pulses-1)*plan.interval_ms||next_order_==UINT64_MAX||pending_.size()>=4096)return false;
        const auto order=next_order_++;pending_.emplace(std::pair{now+plan.first_ms,order},Field{{caster,level,x,y,plane,plan,now+plan.first_ms},plan.pulses,order});return true;
    }
    template<class Apply> void advance(Time now,Apply&& apply){
        if(now<clock_)return;clock_=now;
        while(!pending_.empty()&&pending_.begin()->first.first<=now){
            auto field=std::move(pending_.begin()->second);pending_.erase(pending_.begin());
            if(!apply(field.pulse))continue;
            if(--field.remaining){field.pulse.at+=field.pulse.plan.interval_ms;pending_.emplace(std::pair{field.pulse.at,field.order},std::move(field));}
        }
    }
    bool empty()const{return pending_.empty();}
    // A Health forecast must stop here: a pulse can kill its target before
    // degeneration or an end-heal deadline predicted from current Health.
    std::optional<Time> next_deadline()const{
        return pending_.empty()?std::nullopt:std::optional(pending_.begin()->first.first);
    }
    std::size_t size()const{return pending_.size();}
};
}
