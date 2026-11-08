#pragma once
#include "gw_effect_registry.h"
#include <cmath>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>

namespace fw::gw::combat::ai::aggro {
using Scope=skills::effects::Scope;
inline bool valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
struct Observation {
    Scope target;
    bool alive=false,published=false,same_plane=false;
    // An unavailable authoritative pose must not erase established aggro.
    std::optional<double> distance;
};
struct Selection {std::optional<Scope> target;bool engaged=false;};
// Per-NPC encounter memory. Only observed acquisition-radius entrants become
// new foes; remembered foes may remain eligible through the larger follow
// radius. Radius values and nearest-first choice remain explicit host policy.
class Memory {
    Scope source_;
    std::set<Scope> foes_;
public:
    explicit Memory(Scope source):source_(source){if(!valid(source))throw std::invalid_argument("invalid NPC aggro source");}
    void clear(){foes_.clear();}
    bool remembers(Scope target)const{return foes_.contains(target);}
    std::size_t size()const{return foes_.size();}
    // Explicit committed hostility, distinct from proximity acquisition. The
    // World caller validates damage, life, plane and authored follow/leash
    // limits. Subsequent snapshots still retire dead or out-of-follow foes.
    bool provoke(Scope target){
        if(!valid(target)||target.instance!=source_.instance||target==source_)return false;
        foes_.insert(target);return true;
    }
    // The snapshot must enumerate all current player lifetimes. Missing scopes
    // are retired, while present observations with unknown distance are kept.
    // Validate everything before mutating the encounter.
    std::optional<Selection> observe(std::span<const Observation> snapshot,double acquire,double follow,bool acquire_new=true){
        if(!std::isfinite(acquire)||acquire<=0||!std::isfinite(follow)||follow<acquire)return {};
        std::set<Scope> seen;
        for(const auto& o:snapshot){
            if(!valid(o.target)||o.target.instance!=source_.instance||o.target==source_||!seen.insert(o.target).second||
               (o.distance&&(!std::isfinite(*o.distance)||*o.distance<0)))return {};
        }
        auto next=foes_;std::erase_if(next,[&](Scope target){return !seen.contains(target);});
        Selection result;double nearest=follow;
        for(const auto& o:snapshot){
            if(!o.alive||!o.published||!o.same_plane){next.erase(o.target);continue;}
            if(!o.distance)continue;
            if(*o.distance>follow){next.erase(o.target);continue;}
            if(acquire_new&&*o.distance<=acquire)next.insert(o.target);
            if(next.contains(o.target)&&(!result.target||*o.distance<nearest||(*o.distance==nearest&&o.target<*result.target))){
                result.target=o.target;nearest=*o.distance;
            }
        }
        result.engaged=!next.empty();foes_=std::move(next);return result;
    }
};
}
