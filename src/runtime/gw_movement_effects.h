#pragma once
#include "gw_flame_djinn_haste.h"
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_conditions.h"
#include "gw_armor_earth.h"
#include "gw_dolyak_signet.h"
#include "gw_flail.h"
#include "gw_escape.h"
#include "gw_natural_stride.h"
#include "gw_storm_chaser.h"
#include "gw_charge.h"
#include "gw_battle_rage.h"
#include "gw_bulls_charge.h"
#include "gw_burning_speed.h"
#include "gw_mind_freeze.h"
#include "gw_freezing_gust.h"
#include "gw_rust.h"
#include <algorithm>
#include <cmath>
namespace fw::gw::skills::effects {
struct MovementHexPlan {unsigned skill=76,rank=0;std::uint64_t duration=0;float multiplier=.5f,range=1248;unsigned pips=0;double damage=0;float radius=0;bool caster_center=false;double conditional_damage=0;auto operator<=>(const MovementHexPlan&)const=default;};
inline std::optional<MovementHexPlan> mind_freeze_hex(unsigned skill,unsigned rank){
    const auto plan=mind_freeze(skill,rank);if(!plan)return {};
    return MovementHexPlan{skill,rank,plan->duration,plan->multiplier,plan->range};
}
inline std::optional<MovementHexPlan> shard_storm_hex(unsigned rank){
    const auto* row=find_skill(213);
    if(rank>20||!row||row->type!=4||row->attribute!=11||row->scale0!=10||row->scale15!=85||
       row->bonus0!=66||row->bonus15!=66||row->duration0!=2||row->duration15!=6)return {};
    return MovementHexPlan{213,rank,1000ull*interpolate(2,6,rank),.34f,1248,0,damage_scale(10,85,rank)};
}
inline std::optional<MovementHexPlan> movement_hex(unsigned skill,unsigned rank=0,bool pvp=false){
    if(skill==204){const auto plan=rust(rank);if(!plan)return {};
        return MovementHexPlan{204,rank,plan->duration,1.f,plan->range,0,plan->damage,plan->radius};}
    if(skill==209||skill==2803)return mind_freeze_hex(skill,rank);
    if(skill==213)return shard_storm_hex(rank);
    if(skill==1382){const auto gust=freezing_gust(rank);if(!gust)return {};
        return MovementHexPlan{1382,rank,gust->snare_ms,gust->multiplier,gust->range,0,0,0,false,gust->damage};}
    if(skill==54&&pvp)skill=3152;
    const auto* row=find_skill(skill);
    if(skill==210){if(rank>20||!row||row->type!=4||row->attribute!=11||row->scale0!=66||row->scale15!=66||row->duration0!=8||row->duration15!=20)return {};
        return MovementHexPlan{skill,rank,1000ull*interpolate(8,20,rank),.34f};}
    if(skill==211||skill==212||skill==234){
        if(rank>20||!row||row->type!=4||row->attribute!=11||row->bonus0!=66||row->bonus15!=66)return {};
        const unsigned low=skill==211?20:10,high=skill==211?80:85;
        const unsigned duration0=skill==211?2:skill==212?3:10,duration15=skill==211?6:skill==212?8:10;
        if(row->scale0!=low||row->scale15!=high||row->duration0!=duration0||row->duration15!=duration15)return {};
        return MovementHexPlan{skill,rank,1000ull*interpolate(duration0,duration15,rank),.34f,1248,0,
            damage_scale(low,high,rank),skill==211?166.f:skill==212?252.f:322.f,skill==212};
    }
    if((skill!=76&&skill!=54&&skill!=3152)||rank>20||!row||row->type!=4||row->attribute!=1)return {};
    if(skill==54||skill==3152)return MovementHexPlan{skill,rank,1000ull*(5+rank),.5f,1248,interpolate(row->scale0,row->scale15,rank)};
    return MovementHexPlan{skill,rank,1000ull*interpolate(8,20,rank)};
}
inline bool valid_movement_hex(const MovementHexPlan& p){const auto expected=(p.skill==209||p.skill==2803)?mind_freeze_hex(p.skill,p.rank):p.skill==213?shard_storm_hex(p.rank):movement_hex(p.skill,p.rank);return expected&&*expected==p;}
// Reviewed subset: Dash, Sprint, Illusion of Haste, Imagined Burden, Crippling Anguish, Water hexes, Armor of Earth and Crippled. Effect_stacking.html states
// that movement modifiers multiply: their overlap is 1.5 * .5 = .75.
// Apply the34% combined cap, with Dash's explicit50% exception. Retain ended intervals until the movement cursor consumes them, so late ticks
// and real removal integrate their duration rather than losing the last slice.
class MovementEffects {
    struct Interval {unsigned effect,skill;Time starts,ends;float multiplier;bool forecast=false;};
    std::vector<Interval> intervals_;
public:
    float baseline=1.f;
    bool tracks(unsigned skill)const {return std::any_of(intervals_.begin(),intervals_.end(),[&](const auto& e){return e.skill==skill;});}
    void remove(unsigned effect,Time now){for(auto& e:intervals_)if((e.skill!=481||e.forecast)&&e.effect==effect)e.ends=std::min(e.ends,std::max(e.starts,now));}
    // Crippled is one condition, independent of effect-instance IDs and hex/stance replacement.
    // Keep ended slices until the movement cursor consumes them, including cleanses between ticks.
    void crippled(const combat::conditions::State& state,Time now){
        const auto condition=state.lookup(combat::conditions::Kind::crippled);
        const bool active=state.has(combat::conditions::Kind::crippled,now);
        for(auto& e:intervals_)if(e.skill==481&&!e.forecast&&now>=e.starts&&now<e.ends){
            if(active&&e.ends==condition->expires)return;
            e.ends=now;
        }
        if(active)intervals_.push_back({0,481,now,condition->expires,.5f});
    }
    void apply(const Entry& entry){
        const bool haste=entry.skill==37||entry.skill==3373;
        const auto snare=(entry.skill==209||entry.skill==2803)?mind_freeze_hex(entry.skill,entry.rank):entry.skill==213?shard_storm_hex(entry.rank):(entry.skill==210||entry.skill==211||entry.skill==212||entry.skill==234||entry.skill==1382)?movement_hex(entry.skill,entry.rank):std::nullopt;
        const auto armor=entry.skill==165?armor_earth(entry.rank):std::nullopt;
        const auto dolyak=entry.skill==361?dolyak_signet(entry.rank):std::nullopt;
        const bool flailing=entry.skill==1404&&flail(entry.rank).has_value();
        const bool escaping=(entry.skill==448||entry.skill==3060)&&escape(entry.skill,entry.rank).has_value();
        const auto natural=natural_stride_entry(entry,entry.applied);
        const auto storm=entry.skill==455?storm_chaser(entry.rank):std::nullopt;
        const auto charging=entry.skill==364&&entry.kind==Category::other&&entry.instance&&entry.cause?charge(entry.rank):std::nullopt;
        const auto rage=entry.skill==317&&entry.kind==Category::stance&&entry.instance&&entry.cause?battle_rage(entry.rank):std::nullopt;
        const auto bull=entry.skill==379&&entry.kind==Category::stance&&entry.instance&&entry.cause?bulls_charge(entry.rank):std::nullopt;
        const auto burning=entry.skill==823&&entry.kind==Category::enchantment&&entry.instance&&entry.cause?burning_speed(entry.rank):std::nullopt;
        const auto flame=entry.skill==1381&&entry.kind==Category::enchantment&&entry.instance&&entry.cause?flame_djinn_haste(entry.rank):std::nullopt;
        if(entry.skill==165&&!armor)return;
        if((entry.skill!=76&&entry.skill!=54&&entry.skill!=3152&&entry.skill!=1043&&entry.skill!=349&&!haste&&!armor&&!snare&&!dolyak&&!flailing&&!escaping&&!storm&&!charging&&!rage&&!bull&&!burning&&!natural&&!flame)||!entry.expires)return;
        std::vector<unsigned> replaced;for(const auto& e:intervals_)if(e.skill==entry.skill)replaced.push_back(e.effect);
        for(auto effect:replaced)remove(effect,entry.applied);
        intervals_.push_back({entry.instance,entry.skill,entry.applied,*entry.expires,flame?flame->movement:natural?natural->movement:burning?1.f+float(burning->movement_bonus_percent)/100.f:(rage||bull)?1.33f:charging?charging->multiplier:storm?storm->movement:escaping?1.33f:flailing?.67f:dolyak?dolyak->movement:snare?snare->multiplier:armor?float(armor->movement_multiplier()):haste?1.33f:entry.skill==1043?1.5f:entry.skill==349?1.25f:.5f});
        // Movement may advance before the health/condition tick. Forecast only
        // this known end penalty; stripping/renewal cancels it by effect ID.
        if(haste&&*entry.expires<=UINT64_MAX-3000)intervals_.push_back({entry.instance,481,*entry.expires,*entry.expires+3000,.5f,true});
    }
    float at_with_external(Time now,float external)const {
        if(!std::isfinite(external)||external<=0)return 0;
        float value=baseline*external,cap=std::max(1.34f,baseline),floor=std::min(.5f,external);bool crippled=false;
        for(const auto& e:intervals_)if(now>=e.starts&&now<e.ends){
            if(e.skill==481)crippled=true;else value*=e.multiplier;
            if(e.skill==1043||e.skill==823)cap=std::max(cap,e.multiplier);
            floor=std::min(floor,e.multiplier);
        }
        if(crippled)value*=.5f;return std::clamp(value,floor,cap);
    }
    float at(Time now)const{return at_with_external(now,1.f);}
    std::vector<Time> boundaries(Time from,Time until)const {
        std::vector<Time> result;for(const auto& e:intervals_)for(auto at:{e.starts,e.ends})if(at>from&&at<=until)result.push_back(at);
        std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());return result;
    }
    double integral(Time from,Time until)const {
        if(until<=from)return 0;auto points=boundaries(from,until);points.push_back(until);double result=0;
        for(auto end:points){result+=double(end-from)*at(from);from=end;}return result;
    }
    void discard_before(Time now){std::erase_if(intervals_,[&](const auto& e){return e.ends<=now;});}
    void clear(Time now){for(auto& e:intervals_)e.ends=std::min(e.ends,std::max(e.starts,now));}
};
}
