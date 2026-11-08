#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>
namespace fw::gw::recovery {
using Time=std::uint64_t;
// The mirrored wiki proves seven automatic pips, two health/second per pip,
// and combat reset semantics. It does NOT specify exact ramp timing. The caller
// must supply an explicit schedule from a capture or documented server policy.
struct Schedule {
    std::array<Time,7> thresholds;
    bool valid()const {return thresholds[0]>0&&std::adjacent_find(thresholds.begin(),thresholds.end(),std::greater_equal<Time>())==thresholds.end();}
};
// Explicit development policy, approximate; exact retail thresholds remain uncaptured.
inline constexpr Schedule development_schedule{{5000,8000,11000,14000,17000,20000,23000}};
enum class Event {health_loss,attack,target_enemy_spell_or_signet,targeted_by_enemy_spell_or_signet,self_heal,pbaoe,enemy_shout,zero_damage_received};
inline constexpr bool resets(Event event){return event==Event::health_loss||event==Event::attack||event==Event::target_enemy_spell_or_signet||event==Event::targeted_by_enemy_spell_or_signet;}
struct Segment {Time begin,end;unsigned pips;};
class State {
    Schedule schedule_;Time cursor_,quiet_since_;
public:
    State(Schedule schedule,Time now):schedule_(schedule),cursor_(now),quiet_since_(now){if(!schedule.valid())throw std::invalid_argument("invalid passive recovery schedule");}
    Time cursor()const{return cursor_;}
    unsigned pips(Time now)const {
        if(now<quiet_since_)return 0;
        const auto elapsed=now-quiet_since_;
        return static_cast<unsigned>(std::upper_bound(schedule_.thresholds.begin(),schedule_.thresholds.end(),elapsed)-schedule_.thresholds.begin());
    }
    // Resolve all previous regeneration up to event time BEFORE recording it.
    bool record(Event event,Time now){if(now!=cursor_)return false;if(resets(event))quiet_since_=now;return true;}
    // While external degeneration is negative, continuously reset the quiet
    // timer. Counterbalanced degeneration >=0 permits recovery (wiki example).
    // Call after advancing current interval; death/revival starts a new clock.
    bool inhibit(Time now){if(now!=cursor_)return false;quiet_since_=now;return true;}
    std::vector<Segment> segments(Time now)const {
        if(now<cursor_)throw std::invalid_argument("passive recovery clock reversed");
        std::vector<Time> bounds{cursor_,now};
        for(auto threshold:schedule_.thresholds){
            if(threshold>UINT64_MAX-quiet_since_)continue;
            auto at=quiet_since_+threshold;if(at>cursor_&&at<now)bounds.push_back(at);
        }
        std::sort(bounds.begin(),bounds.end());bounds.erase(std::unique(bounds.begin(),bounds.end()),bounds.end());
        std::vector<Segment> result;for(std::size_t i=1;i<bounds.size();++i)result.push_back({bounds[i-1],bounds[i],pips(bounds[i-1])});return result;
    }
    bool commit(Time now){if(now<cursor_)return false;cursor_=now;return true;}
};
}


