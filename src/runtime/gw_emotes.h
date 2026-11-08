#pragma once
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace fw::gw::emotes {
using Packet=std::vector<std::uint8_t>;
enum class Action { dance, sit, stand };
struct Request {Action action=Action::dance;bool synchronized=false;};
inline constexpr std::uint32_t sitting=0x2000;
// Pinned37130 avchar.cpp/77ba30: 43394f1d is the special dance sequence;
// without the CE flags it explicitly falls back to this ordinary dance UID.
inline constexpr std::uint32_t dance_sequence=0x31939cbb;
inline void player(std::uint32_t id){if(!id||id>65535)throw std::invalid_argument("invalid emote player");}
inline void word(Packet& packet,std::uint32_t value){for(unsigned n=0;n<4;++n)packet.push_back(static_cast<std::uint8_t>(value>>(8*n)));}
inline Packet dance_2022(std::uint32_t id){
    player(id);Packet result{158,0};word(result,28);word(result,id);word(result,dance_sequence);return result;
}
inline Packet effects_2022(std::uint32_t id,std::uint32_t effects){
    player(id);Packet result{240,0};word(result,id);word(result,effects);return result;
}
// 794810 kind22 ->77ba30. An unavailable sequence UID takes the native
// locomotion/neutral-pose fallback; zero does not request a guessed emote UID.
// Verify this reset against each supported retail model before widening the
// catalogue: model-specific sequence metadata lives in the DAT, not this exe.
inline Packet reset_animation_2022(std::uint32_t id){
    player(id);Packet result{158,0};word(result,22);word(result,id);word(result,0);return result;
}
struct Result {
    std::vector<Packet> packets;
    std::optional<std::uint32_t> effects;
    bool stop_movement=false;
    std::u16string announcement; // suffix only; empty for sit/stand, per wiki
};
// State is owned by one joined Player. It neither owns the complete effects
// mask nor retains pointers into command Context. Always supply the current
// authoritative mask, and commit Result.effects without discarding other bits.
class State {
    struct Scheduled {Action action;std::uint64_t due;};
    std::optional<Scheduled> pending_;
    bool dancing_=false;
    Result apply(std::uint32_t id,Action action,std::uint32_t effects){
        player(id);Result result;
        switch(action){
        case Action::dance:
            if(effects&sitting){effects&=~sitting;result.effects=effects;result.packets.push_back(effects_2022(id,effects));}
            result.packets.push_back(dance_2022(id));dancing_=true;result.stop_movement=true;
            result.announcement=u"starts dancing.";break;
        case Action::sit:
            if(dancing_){result.packets.push_back(reset_animation_2022(id));dancing_=false;}
            if(!(effects&sitting)){effects|=sitting;result.effects=effects;result.packets.push_back(effects_2022(id,effects));}
            result.stop_movement=true;break;
        case Action::stand:
            // /stand has no effect unless the character is sitting. It is not
            // a general animation-cancel command and must not stop a dance.
            if(effects&sitting){effects&=~sitting;result.effects=effects;result.packets.push_back(effects_2022(id,effects));}
            break;
        default:throw std::invalid_argument("unknown emote action");
        }
        return result;
    }
public:
    bool dancing()const{return dancing_;}
    bool scheduled()const{return pending_.has_value();}
    Result submit(std::uint32_t id,Request request,std::uint64_t now,std::uint32_t effects){
        player(id);
        if(request.action!=Action::dance&&request.action!=Action::sit&&request.action!=Action::stand)
            throw std::invalid_argument("unknown emote action");
        if(effects&0x10)throw std::invalid_argument("dead player cannot emote");
        if(request.synchronized&&request.action==Action::stand)
            throw std::invalid_argument("stand has no synchronized animation");
        const auto wait=request.synchronized?(5000-now%5000)%5000:0;
        if(wait){
            if(now>std::numeric_limits<std::uint64_t>::max()-wait)throw std::invalid_argument("emote clock overflow");
            pending_=Scheduled{request.action,now+wait};return {};
        }
        pending_.reset();return apply(id,request.action,effects);
    }
    Result tick(std::uint32_t id,std::uint64_t now,std::uint32_t effects){
        player(id);if(!pending_||now<pending_->due)return {};
        const auto action=pending_->action;pending_.reset();
        if(effects&0x10)return {}; // dying while waiting cancels the request
        return apply(id,action,effects);
    }
    Result cancel(std::uint32_t id,std::uint32_t effects){
        player(id);pending_.reset();Result result;
        if(dancing_){result.packets.push_back(reset_animation_2022(id));dancing_=false;}
        if(effects&sitting){effects&=~sitting;result.effects=effects;result.packets.push_back(effects_2022(id,effects));}
        return result;
    }
    std::vector<Packet> snapshot(std::uint32_t id,std::uint32_t effects)const{
        player(id);std::vector<Packet> result;
        if(effects&sitting)result.push_back(effects_2022(id,effects));
        if(dancing_&&!(effects&0x10))result.push_back(dance_2022(id));
        return result;
    }
};
}
