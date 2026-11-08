#pragma once
#include "gw_effect_registry.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace fw::gw::overcast {
// Owned ceiling and timed regeneration modifiers, not another Energy pool. Current Energy/debt stay
// in the resource authority. Rate is explicit: native capture presentation
// supports33 units/ms (0.33 Energy/s), not an implicit one-point/3s phase.
class CeilingCurve {
public:
    static constexpr std::uint64_t units_per_energy=100000;
private:
    skills::effects::Scope owner_;
    std::uint64_t cursor_=0,burden_=0,last_cost_=0;
    std::uint32_t recovery_units_per_ms_;
    struct EnergyModifier {unsigned skill=0;int pips=0;std::uint64_t applied=0,expires=0;};
    std::map<unsigned,EnergyModifier> energy_modifiers_;
    unsigned maintained_upkeep_=0;
    bool owns(skills::effects::Scope owner)const{
        return owner==owner_&&owner.instance&&owner.actor&&owner.actor<=65535&&owner.generation;
    }
public:
    CeilingCurve(skills::effects::Scope owner,std::uint64_t at,std::uint32_t rate):owner_(owner),cursor_(at),recovery_units_per_ms_(rate){
        if(!owns(owner)||!rate)throw std::invalid_argument("invalid Overcast curve owner/rate");
    }
    bool advance(skills::effects::Scope owner,std::uint64_t at){
        if(!owns(owner)||at<cursor_)return false;
        const auto elapsed=at-cursor_;
        // Compare before multiplying so even UINT64_MAX elapsed is safe.
        const auto until_clear=burden_/recovery_units_per_ms_+(burden_%recovery_units_per_ms_!=0);
        burden_=elapsed>=until_clear?0:burden_-elapsed*recovery_units_per_ms_;
        cursor_=at;return true;
    }
    bool apply(skills::effects::Scope owner,std::uint64_t amount,std::uint64_t event,std::uint64_t at){
        if(!owns(owner)||!amount||!event||event<=last_cost_||at<cursor_||amount>UINT64_MAX/units_per_energy)return false;
        auto proposed=*this;if(!proposed.advance(owner,at))return false;
        const auto added=amount*units_per_energy;if(added>UINT64_MAX-proposed.burden_)return false;
        proposed.burden_+=added;proposed.last_cost_=event;*this=proposed;return true;
    }
    // Mutations require the resource cursor already settled at the event time.
    // This prevents a replacement/removal from rewriting an elapsed interval.
    bool apply_energy_modifier(skills::effects::Scope owner,unsigned effect,unsigned skill,std::int64_t pips,
                               std::uint64_t duration,std::uint64_t at){
        if(!owns(owner)||!effect||!skill||!skills::find_skill(skill)||pips< -10||pips>10||!duration||duration>43200000||
           at!=cursor_||duration>UINT64_MAX-at)return false;
        const auto collision=energy_modifiers_.find(effect);
        if(collision!=energy_modifiers_.end()&&collision->second.skill!=skill)return false;
        auto next=energy_modifiers_;
        std::erase_if(next,[&](const auto& entry){return entry.second.expires<=at||entry.second.skill==skill;});
        if(next.size()>=64)return false; // Defensive storage bound,not a retail slot limit.
        next.emplace(effect,EnergyModifier{skill,static_cast<int>(pips),at,at+duration});energy_modifiers_=std::move(next);return true;
    }
    // Maintained Succor has no timed expiry; its scoped owner removes it.
    bool apply_succor_modifier(skills::effects::Scope owner,unsigned effect,std::uint64_t at){
        if(!owns(owner)||!effect||at!=cursor_||!skills::find_skill(308))return false;
        auto next=energy_modifiers_;
        if(const auto found=next.find(effect);found!=next.end()&&found->second.skill!=308)return false;
        std::erase_if(next,[](const auto& item){return item.second.skill==308;});
        if(next.size()>=64)return false;
        next.emplace(effect,EnergyModifier{308,1,at,UINT64_MAX});energy_modifiers_=std::move(next);return true;
    }
    bool remove_energy_modifier(skills::effects::Scope owner,unsigned effect,std::uint64_t at){
        if(!owns(owner)||at!=cursor_)return false;return energy_modifiers_.erase(effect)!=0;
    }
    bool clear_energy_modifiers(skills::effects::Scope owner,std::uint64_t at){
        if(!owns(owner)||at!=cursor_)return false;energy_modifiers_.clear();return true;
    }
    bool prune_energy_modifiers(skills::effects::Scope owner,std::uint64_t at){
        if(!owns(owner)||at!=cursor_)return false;
        std::erase_if(energy_modifiers_,[&](const auto& entry){return entry.second.expires<=at;});return true;
    }
    bool has_energy_modifiers()const{return !energy_modifiers_.empty();}
    bool has_energy_modifier(skills::effects::Scope owner,unsigned effect)const{
        return owns(owner)&&energy_modifiers_.contains(effect);
    }
    bool matches_energy_modifier(skills::effects::Scope owner,unsigned effect,unsigned skill,int pips,
            std::uint64_t applied,std::uint64_t expires)const{
        if(!owns(owner))return false;const auto found=energy_modifiers_.find(effect);
        return found!=energy_modifiers_.end()&&found->second.skill==skill&&found->second.pips==pips&&
            found->second.applied==applied&&found->second.expires==expires;
    }
    int external_energy_pips(int base,std::uint64_t at)const{
        if(base< -10||base>10)throw std::invalid_argument("invalid base Energy regeneration");
        int total=base;for(const auto& [id,entry]:energy_modifiers_)if(at>=entry.applied&&at<entry.expires)total+=entry.pips;return total;
    }
    unsigned maintained_upkeep()const{return maintained_upkeep_;}
    bool set_maintained_upkeep(skills::effects::Scope owner,unsigned count,std::uint64_t at){
        if(!owns(owner)||at!=cursor_||count>4096)return false;maintained_upkeep_=count;return true;
    }
    int uncapped_energy_pips(int base,std::uint64_t at)const{return external_energy_pips(base,at)-int(maintained_upkeep_);}
    int effective_energy_pips(int base,std::uint64_t at)const{
        if(base< -10||base>10)throw std::invalid_argument("invalid base Energy regeneration");
        return std::clamp(uncapped_energy_pips(base,at),-10,10);
    }
    std::optional<std::uint64_t> next_energy_boundary(std::uint64_t at)const{
        std::optional<std::uint64_t> result;
        for(const auto& [id,entry]:energy_modifiers_)for(const auto edge:{entry.applied,entry.expires})
            if(edge>at&&(!result||edge<*result))result=edge;
        return result;
    }
    bool active()const{return burden_!=0;}
    std::uint64_t burden_units()const{return burden_;}
    std::uint64_t cursor()const{return cursor_;}
    // World must split regeneration at changing integer ceilings instead of
    // applying the final ceiling retroactively over an entire elapsed span.
    std::optional<std::uint64_t> next_integer_change()const{
        if(!burden_)return {};
        const auto tail=burden_%units_per_energy;
        const auto needed=tail?tail:units_per_energy;
        const auto elapsed=needed/recovery_units_per_ms_+(needed%recovery_units_per_ms_!=0);
        if(elapsed>UINT64_MAX-cursor_)return {};
        return cursor_+elapsed;
    }
    double recovery_per_second()const{return double(recovery_units_per_ms_)*1000/units_per_energy;}
    double ceiling(unsigned nominal)const{
        if(!nominal||nominal>1000000)throw std::invalid_argument("invalid nominal Energy capacity");
        return double(nominal)-double(burden_)/units_per_energy;
    }
    std::int64_t integer_ceiling(unsigned nominal)const{
        if(!nominal||nominal>1000000)throw std::invalid_argument("invalid nominal Energy capacity");
        const auto removed=burden_/units_per_energy+(burden_%units_per_energy!=0);
        return std::int64_t(nominal)-static_cast<std::int64_t>(removed);
    }
    bool retire(skills::effects::Scope owner){if(!owns(owner))return false;owner_={};burden_=0;energy_modifiers_.clear();return true;}
};
}
