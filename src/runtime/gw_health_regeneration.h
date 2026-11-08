#pragma once
#include "gw_effect_registry.h"
#include "gw_blocking.h"
#include "gw_watchful_healing.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <span>

namespace fw::gw::skills::regeneration {
struct Plan {unsigned skill,rank;int pips;std::uint64_t duration;bool self_only;};
inline std::optional<Plan> reviewed(unsigned skill,unsigned rank){
    const auto* row=find_skill(skill);if(!row||rank>20)return {};
    if(skill==1392){const auto plan=effects::watchful_healing(rank);if(!plan)return {};return Plan{skill,rank,int(plan->pips),plan->duration,false};}
    if(skill==258){const auto plan=effects::guardian_block(rank);if(!plan)return {};return Plan{skill,rank,0,plan->duration,false};}
    if(skill==2061)return Plan{skill,rank,0,2000,false};
    if(skill==299)return Plan{skill,rank,0,8000,false};
    // Restful Breeze: the +10 source is owned by the recipient and expires
    // on that recipient's next accepted skill use or actual attack start.
    // The early-end consumer is separate from this immutable magnitude plan.
    if(skill==886)return Plan{skill,rank,10,
        1000u*static_cast<std::uint64_t>(std::lround(row->duration0+
            (double(row->duration15)-row->duration0)*rank/15.)),false};
    // Pinned 2022 Hexer's Vigor: Soul Reaping regeneration, no end heal.
    // Successful non-hex skill completion is observed separately in World.
    if(skill==2138){
        if(row->type!=6||row->profession!=4||row->attribute!=6||row->energy_cost()!=5||
           row->activation!=.25f||row->aftercast!=.75f||row->recharge!=10||
           row->duration0!=10||row->duration15!=10||row->scale0!=1||row->scale15!=8||
           row->caster_body_resource!=1814||row->target_body_resource!=2077)return {};
        return Plan{skill,rank,static_cast<int>(std::lround(1.+7.*rank/15.)),10000,true};
    }
    if(skill==814){
        if(row->type!=6||row->profession!=7||row->attribute!=31||row->duration0!=6||row->duration15!=6||
           row->bonus0!=5||row->bonus15!=10||row->scale0!=40||row->scale15!=100)return {};
        return Plan{skill,rank,static_cast<int>(std::lround(5.+5.*rank/15.)),6000,true};
    }
    if(skill!=288&&skill!=446&&skill!=115)return {};
    return Plan{skill,rank,static_cast<int>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.)),skill==288?15000u:skill==115?7000u:13000u,skill!=288};
}
inline unsigned end_healing(unsigned skill,unsigned rank){
    const auto* row=find_skill(skill);
    if(skill==2061&&row&&rank<=20)return static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.));
    if(skill==814&&reviewed(skill,rank))return static_cast<unsigned>(std::lround(40.+60.*rank/15.));
    return (skill==115||skill==299)&&row&&rank<=20?static_cast<unsigned>(std::lround(row->bonus0+(double(row->bonus15)-row->bonus0)*rank/15.)):0;
}
struct Source {std::uint32_t effect,cause;unsigned skill;int pips;effects::Time begins;std::optional<effects::Time> ends;};
// External effects supply a complete, piecewise-constant timeline. Fractional
// crossings remain fractional; no recipient buff/expiry record is fabricated.
struct ExternalPips {double begins=0,ends=0;int pips=0;};
struct BandedAdvance {std::optional<double> death_at;};
// World supplies explicit health and all other degeneration/regeneration.
// This integrates actual timed-source overlap, rather than treating a delayed
// tick as if a just-expired source never contributed. No automatic out-of-combat
// regeneration or condition degeneration is invented here.
class State {
    effects::Scope owner_;
    effects::Time cursor_;
    std::map<std::uint32_t,Source> sources_;
    bool owns(effects::Scope scope)const{return scope==owner_&&scope.instance&&scope.actor&&scope.generation;}
    static double integrate(double health,unsigned maximum,int pips,double elapsed){
        health=std::clamp(health+2.*std::clamp(pips,-10,10)*elapsed/1000.,0.,double(maximum));
        const auto integer=std::round(health);if(std::abs(health-integer)<1e-7)health=integer;
        return health;
    }

public:
    void swap(State&other)noexcept{
        static_assert(noexcept(sources_.swap(other.sources_)));
        using std::swap;swap(owner_,other.owner_);swap(cursor_,other.cursor_);sources_.swap(other.sources_);
    }
    friend void swap(State&a,State&b)noexcept{a.swap(b);}
    bool matches(effects::Scope scope)const{return owns(scope);}
    effects::Time cursor()const{return cursor_;}
    std::optional<Source> lookup(effects::Scope scope,std::uint32_t effect)const{
        if(!owns(scope))return {};const auto found=sources_.find(effect);
        return found==sources_.end()?std::nullopt:std::optional(found->second);
    }
    int pips(effects::Time now)const {int result=0;for(const auto& [id,s]:sources_)if(now>=s.begins&&(!s.ends||now<*s.ends))result+=s.pips;return result;}
    std::vector<effects::Time> expiry_boundaries(effects::Time after,effects::Time before)const{
        std::vector<effects::Time> result;for(const auto& [id,s]:sources_)if(s.ends&&*s.ends>after&&*s.ends<before)result.push_back(*s.ends);return result;
    }
    State(effects::Scope owner,effects::Time now):owner_(owner),cursor_(now){}
    bool apply(effects::Scope scope,Source source,bool replace_same_skill=true){
        if(!owns(scope)||!source.effect||!source.cause||(source.skill==308&&source.pips!=1)||source.begins!=cursor_||(source.ends?*source.ends<=source.begins:(source.skill!=290&&source.skill!=308&&(source.skill!=925||source.pips!=0)))||(source.pips < -100 || source.pips > 100))return false;
        const auto* row=find_skill(source.skill);if(!row||!source.skill)return false;
        std::size_t replaced=0;for(const auto& [id,old]:sources_)if(id==source.effect||(replace_same_skill&&old.skill==source.skill))++replaced;
        if(sources_.size()-replaced>=128)return false;
        for(auto it=sources_.begin();it!=sources_.end();)if(it->first==source.effect||(replace_same_skill&&it->second.skill==source.skill))it=sources_.erase(it);else ++it;
        sources_[source.effect]=source;return true;
    }
    // Caller first advances to removal time; otherwise prior contribution
    // cannot be reconstructed after deleting its source.
    bool remove(effects::Scope scope,std::uint32_t effect){return owns(scope)&&sources_.erase(effect);}
    // Linked benefits may end before this actor's next resource tick. Preserve
    // their historical contribution until the cursor reaches the removal time;
    // never recursively advance another actor while resolving death/expiry.
    bool end(effects::Scope scope,std::uint32_t effect,effects::Time now){
        if(!owns(scope)||now<cursor_)return false;
        const auto found=sources_.find(effect);if(found==sources_.end())return false;
        if(now<=cursor_)sources_.erase(found);
        else found->second.ends=found->second.ends?std::min(*found->second.ends,now):now;
        return true;
    }
    // World can preflight on a value copy and use death_at to settle its own
    // death/reaction boundary before committing. This receipt is not a World
    // death event and never heals a dead actor back to life.
    std::optional<BandedAdvance> advance_external(effects::Scope scope,effects::Time now,double& health,
            unsigned maximum,std::span<const ExternalPips> external,int other_pips=0){
        constexpr effects::Time exact_time=9007199254740991ull;
        if(!owns(scope)||now<cursor_||now>exact_time||!maximum||!std::isfinite(health)||health<0||health>maximum||
           other_pips < -1000||other_pips > 1000||external.size()>65536)return {};
        double covered=double(cursor_);
        for(const auto& band:external){
            if(!std::isfinite(band.begins)||!std::isfinite(band.ends)||band.begins!=covered||
               band.ends<=band.begins||band.ends>double(now)||band.pips < -1000||band.pips > 1000)return {};
            covered=band.ends;
        }
        if(covered!=double(now))return {};
        std::vector<double> bounds{double(cursor_),double(now)};
        for(const auto& band:external){bounds.push_back(band.begins);bounds.push_back(band.ends);}
        for(const auto&[id,source]:sources_){
            if(source.begins>cursor_&&source.begins<now)bounds.push_back(double(source.begins));
            if(source.ends&&*source.ends>cursor_&&*source.ends<now)bounds.push_back(double(*source.ends));
        }
        std::sort(bounds.begin(),bounds.end());bounds.erase(std::unique(bounds.begin(),bounds.end()),bounds.end());
        double proposed=health;BandedAdvance result;std::size_t band=0;
        for(std::size_t i=1;i<bounds.size()&&proposed>0;++i){
            const double begins=bounds[i-1],ends=bounds[i];
            while(band<external.size()&&external[band].ends<=begins)++band;
            int net=other_pips+(band<external.size()?external[band].pips:0);
            for(const auto&[id,source]:sources_)if(double(source.begins)<=begins&&
                (!source.ends||double(*source.ends)>begins))net+=source.pips;
            const int capped=std::clamp(net,-10,10);
            if(capped<0){const double delay=proposed*1000./(-2.*capped);
                if(delay<=ends-begins){result.death_at=begins+delay;proposed=0;break;}}
            proposed=integrate(proposed,maximum,capped,ends-begins);
            if(proposed==0){result.death_at=ends;break;}
        }
        // No mutation occurs before the complete timeline has been validated.
        health=proposed;cursor_=now;
        for(auto it=sources_.begin();it!=sources_.end();)if(it->second.ends&&now>=*it->second.ends)it=sources_.erase(it);else ++it;
        return result;
    }
    bool advance(effects::Scope scope,effects::Time now,double& health,unsigned maximum,int other_pips=0){
        if(!owns(scope)||now<cursor_||!maximum||!std::isfinite(health)||health<0||health>maximum||(other_pips < -1000 || other_pips > 1000))return false;
        std::vector<effects::Time> bounds{cursor_,now};
        for(const auto& [id,s]:sources_){if(s.begins>cursor_&&s.begins<now)bounds.push_back(s.begins);if(s.ends&&*s.ends>cursor_&&*s.ends<now)bounds.push_back(*s.ends);}
        std::sort(bounds.begin(),bounds.end());bounds.erase(std::unique(bounds.begin(),bounds.end()),bounds.end());
        for(std::size_t i=1;i<bounds.size()&&health>0;++i){int net=other_pips;for(const auto& [id,s]:sources_)if(s.begins<=bounds[i-1]&&(!s.ends||*s.ends>bounds[i-1]))net+=s.pips;
            // Share the same integer-residue tolerance with banded external
            // sources, preserving legacy sparse/dense publication rounding.
            health=integrate(health,maximum,net,double(bounds[i]-bounds[i-1]));
        }
        cursor_=now;for(auto it=sources_.begin();it!=sources_.end();)if(it->second.ends&&now>=*it->second.ends)it=sources_.erase(it);else ++it;return true;
    }
};
}



