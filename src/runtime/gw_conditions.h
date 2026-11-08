#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

namespace fw::gw::combat::conditions {
enum class Kind:unsigned {bleeding,blind,burning,crippled,deep_wound,disease,poison,dazed,weakness,cracked_armor,count};
inline constexpr std::array<unsigned,10> skills{478,479,480,481,482,483,484,485,486,2077};
struct Entry {std::uint64_t expires=0;std::uint32_t cause=0;std::uint64_t applied=0,order=0;};
class State {
    std::array<std::optional<Entry>,10> entries_{};
    std::uint64_t next_order_=1;
    // Transient casting modifier, not a condition: cannot be cleansed and
    // contributes no condition icon/count. Deadline avoids late-loop overrun.
    std::uint64_t healing_signet_from_=0,healing_signet_until_=0;
public:
    bool healing_signet(std::uint64_t now,std::optional<std::uint64_t> until){
        if(until&&(*until<=now||*until-now>60000))return false;
        healing_signet_from_=now;healing_signet_until_=until.value_or(0);return true;
    }
    bool has(Kind kind,std::uint64_t now)const {const auto i=static_cast<unsigned>(kind);return i<10&&entries_[i]&&now>=entries_[i]->applied&&now<entries_[i]->expires;}
    // Wiki Condition notes: reapplication retains remaining duration unless
    // the new duration is longer. Reapplication never shortens an active timer.
    bool apply(Kind kind,std::uint32_t cause,std::uint64_t now,std::uint64_t duration_ms){
        const auto i=static_cast<unsigned>(kind);
        if(i>=10||!cause||!duration_ms||duration_ms>43200000||next_order_==std::numeric_limits<std::uint64_t>::max()||
            now>std::numeric_limits<std::uint64_t>::max()-duration_ms)return false;
        const auto expiry=now+duration_ms;
        if(!entries_[i]||expiry>entries_[i]->expires)entries_[i]=Entry{expiry,cause,now,next_order_};
        // A fresh application covers older different conditions, even when its
        // remaining duration is already longer. Preserve that original timer.
        entries_[i]->order=next_order_++;
        return true;
    }
    std::optional<Entry> lookup(Kind kind)const{const auto i=static_cast<unsigned>(kind);return i<10?entries_[i]:std::nullopt;}
    unsigned count(std::uint64_t now)const{
        unsigned result=0;for(unsigned i=0;i<10;++i)result+=has(static_cast<Kind>(i),now);return result;
    }
    std::optional<Kind> newest(std::uint64_t now)const{
        std::optional<Kind> result;std::uint64_t order=0;
        for(unsigned i=0;i<10;++i)if(has(static_cast<Kind>(i),now)&&entries_[i]->order>order){order=entries_[i]->order;result=static_cast<Kind>(i);}
        return result;
    }
    std::vector<std::uint64_t> expiry_boundaries(std::uint64_t after,std::uint64_t before)const{
        std::vector<std::uint64_t> result;for(const auto& entry:entries_)if(entry&&entry->expires>after&&entry->expires<before)result.push_back(entry->expires);
        std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());return result;
    }
    int degeneration_pips(std::uint64_t now)const{
        // Reviewed local Bleeding/Burning/Disease/Poison descriptions. Sum all
        // sources before the health integrator's joint regeneration cap.
        return (has(Kind::bleeding,now)?-3:0)+(has(Kind::burning,now)?-7:0)+
            (has(Kind::disease,now)?-4:0)+(has(Kind::poison,now)?-4:0);
    }
    bool remove(Kind kind){const auto i=static_cast<unsigned>(kind);if(i>=10||!entries_[i])return false;entries_[i].reset();return true;}
    void expire(std::uint64_t now){for(auto& e:entries_)if(e&&now>=e->expires)e.reset();}
    void clear(){for(auto& e:entries_)e.reset();healing_signet_until_=0;}
    std::uint32_t presentation(std::uint32_t other_effects,std::uint64_t now)const {
        // GWCA Agent.effects exposes specific bleeding/crippled/deepwound/
        // poison bits, but blind/weakness/cracked armor share Conditioned only.
        // Preserve death/enchantment/hex and all unrelated flags.
        auto result=other_effects&~std::uint32_t(0x6b);
        bool any=false;for(unsigned i=0;i<10;++i)any|=has(static_cast<Kind>(i),now);
        if(any)result|=2;
        if(has(Kind::bleeding,now))result|=1;
        if(has(Kind::crippled,now))result|=8;
        if(has(Kind::deep_wound,now))result|=0x20;
        if(has(Kind::poison,now))result|=0x40;
        return result;
    }
    int armor(int reviewed_rating,std::uint64_t now)const {
        // Cracked Armor lowers rating20, but never reduces ratings already<=60.
        const auto rating=has(Kind::cracked_armor,now)?std::min(reviewed_rating,std::max(60,reviewed_rating-20)):reviewed_rating;
        return rating-(now>=healing_signet_from_&&now<healing_signet_until_?40:0);
    }
    int earth_armor(int reviewed_rating,std::uint64_t now)const{return armor(reviewed_rating,now);}
};
}
