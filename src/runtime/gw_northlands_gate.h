#pragma once
#include "gw_effect_registry.h"
#include "gw_agent_wire.h"
#include "gw_navigation.h"
#include <array>
#include <map>
#include <optional>
#include <limits>
namespace fw::gw::northlands {
using Scope=skills::effects::Scope;
inline constexpr std::uint32_t lever_model=1279,door_model=1280,scene_object=50593;
inline constexpr std::uint64_t open_interval_ms=30000;
inline constexpr navigation::Point lever_position{-5508,12787};
inline constexpr unsigned lever_plane=36;
enum class Scene { south146,north147 };
struct Lease {std::uint64_t instance=0,generation=0;auto operator<=>(const Lease&)const=default;};
struct Policy {
    // Authored interaction/animation policy. The captured animation16start to
    // animation3start interval is30000ms; exact collision/animation settle
    // alignment is not recovered. Collision opens/closes at animation start.
    float interaction_radius=160;
    std::uint64_t animation_settle_ms=1000;
};
enum class Record { animation,state };
struct Event {
    Lease owner;std::uint64_t revision=0,at=0;Record record=Record::state;
    std::uint32_t object=scene_object,value=0,flags=2;
    bool changes_collision=false,collision_open=false;
};
enum class Status { advanced,opened,closed,duplicate,invalid_actor,wrong_target,unreachable,
                    not_interactive,clock_regressed,time_overflow };
struct Result {Status status=Status::advanced;std::vector<Event> events;};
struct Pull {
    // These are trusted World facts, never client coordinates/reachability.
    // Operation is minted by the server's accepted action boundary; a later
    // genuine pull has a new operation, while replay cannot toggle twice.
    Scope selected_actor,current_actor,selected_lever;
    navigation::Point position{};std::uint64_t operation=0;
    bool loaded=false,alive=false,server_reachable=false;
};
class Controller {
    Scene scene_;Lease owner_;Scope lever_;Policy policy_;
    bool open_=false;std::uint64_t revision_=1;
    std::optional<std::uint64_t> clock_,settle_at_,close_at_;
    std::map<Scope,std::uint64_t> operations_;
    static bool valid(Scope scope){return scope.instance&&scope.actor&&scope.generation;}
    Event event(std::uint64_t at,Record record,unsigned value,unsigned flags,bool collision=false)const{
        return {owner_,revision_,at,record,scene_object,value,flags,collision,open_};
    }
    void change(bool open,std::uint64_t at,std::vector<Event>& events){
        open_=open;++revision_;settle_at_=at+policy_.animation_settle_ms;
        close_at_=open?std::optional{at+open_interval_ms}:std::nullopt;
        events.push_back(event(at,Record::animation,open?16:3,2,true));
    }
    void advance_into(std::uint64_t now,std::vector<Event>& events){
        while(settle_at_||close_at_){
            if(settle_at_&&(!close_at_||*settle_at_<=*close_at_)){
                if(*settle_at_>now)break;
                events.push_back(event(*settle_at_,Record::state,open_?0:2,2));settle_at_.reset();
            }else{
                if(*close_at_>now)break;
                change(false,*close_at_,events);
            }
        }
        clock_=now;
    }
public:
    Controller(Scene scene,Lease owner,Scope lever={},Policy policy={}):scene_(scene),owner_(owner),lever_(lever),policy_(policy){
        if((scene!=Scene::south146&&scene!=Scene::north147)||!owner.instance||!owner.generation||
            !std::isfinite(policy.interaction_radius)||policy.interaction_radius<=0||policy.interaction_radius>1024||
            !policy.animation_settle_ms||policy.animation_settle_ms>=open_interval_ms||
            (scene==Scene::south146&&(!valid(lever)||lever.instance!=owner.instance))||
            (scene==Scene::north147&&lever!=Scope{}))throw std::invalid_argument("invalid scoped Northlands gate");
        open_=scene==Scene::north147;
    }
    Scene scene()const{return scene_;}Lease owner()const{return owner_;}Scope lever()const{return lever_;}
    bool collision_open()const{return open_;}std::uint64_t revision()const{return revision_;}
    std::optional<std::uint64_t> closes_at()const{return close_at_;}
    std::optional<std::uint64_t> next_boundary()const{
        if(!settle_at_)return close_at_;if(!close_at_)return settle_at_;return std::min(*settle_at_,*close_at_);
    }
    Result advance(std::uint64_t now){
        Result out;out.events.reserve(4);
        if(clock_&&now<*clock_){out.status=Status::clock_regressed;return out;}
        advance_into(now,out.events);return out;
    }
    Result pull(const Pull& use,std::uint64_t now){
        Result out;out.events.reserve(4);
        if(clock_&&now<*clock_){out.status=Status::clock_regressed;return out;}
        Status admission=Status::opened;std::map<Scope,std::uint64_t>::iterator operation=operations_.end();
        if(scene_!=Scene::south146)admission=Status::not_interactive;
        else if(!valid(use.current_actor)||use.selected_actor!=use.current_actor||use.current_actor.instance!=owner_.instance||
            use.current_actor.actor==lever_.actor||!use.loaded||!use.alive||!use.operation)admission=Status::invalid_actor;
        else if(use.selected_lever!=lever_)admission=Status::wrong_target;
        else if(!std::isfinite(use.position.x)||!std::isfinite(use.position.z)||!use.server_reachable||
            std::hypot(double(use.position.x)-lever_position.x,double(use.position.z)-lever_position.z)>policy_.interaction_radius)
            admission=Status::unreachable;
        else{
            // Allocate before mutating chronological state. Later writes and
            // the bounded four-event drain cannot allocate after commit.
            operation=operations_.try_emplace(use.current_actor,0).first;
            if(use.operation<=operation->second)admission=Status::duplicate;
        }
        advance_into(now,out.events);
        if(admission!=Status::opened){out.status=admission;return out;}
        if(!open_&&now>std::numeric_limits<std::uint64_t>::max()-open_interval_ms-policy_.animation_settle_ms){
            out.status=Status::time_overflow;return out;
        }
        operation->second=use.operation;
        change(!open_,now,out.events);out.status=open_?Status::opened:Status::closed;return out;
    }
    // Caller must deliver advance(now)'s shared events before taking this
    // current-state view. Late observers receive a settled snapshot, not a
    // restart of another observer's in-flight animation.
    std::array<Event,2> snapshot()const{
        const auto at=clock_.value_or(0);return {event(at,Record::state,open_?0:2,2),event(at,Record::animation,open_?16:3,3)};
    }
    // Local lifetime retirement only. The World still rejects departed actors
    // before constructing Pull; a reused actor must have a new generation.
    void forget(Scope actor){operations_.erase(actor);}
};
// Native37130 descriptors60D148/60D16C. Scene IDs are WORDs, animation BYTE,
// state value/mask DWORDs. Scene identity is distinct from actor/gadget IDs.
inline agent::Packet wire_2022(const Event& event){
    if(!event.owner.instance||!event.owner.generation||!event.revision||event.object!=scene_object||
        (event.record!=Record::animation&&event.record!=Record::state)||
        (event.record==Record::animation&&((event.value!=3&&event.value!=16)||(event.flags!=2&&event.flags!=3)))||
        (event.record==Record::state&&((event.value!=0&&event.value!=2)||event.flags!=2)))
        throw std::invalid_argument("invalid Northlands scene publication");
    agent::Packet out;const auto put=[&](std::uint32_t value,unsigned bytes){for(unsigned i=0;i<bytes;++i)out.push_back(std::uint8_t(value>>(8*i)));};
    put(event.record==Record::animation?269:272,2);put(event.object,2);
    put(event.value,event.record==Record::animation?1:4);put(event.flags,4);return out;
}
// Captured type2 lever record: sidequest packet538320. Preserve nonliving type,
// native model, flags/state and sentinel fields; only the private actor varies.
// This is not a living NPC model/tag or a claim of rendered verification.
namespace detail {
inline agent::Packet nonliving_spawn_2022(std::uint32_t actor,bool door,Scene scene=Scene::south146){
    if(!actor||actor>65535||(scene!=Scene::south146&&scene!=Scene::north147)||(!door&&scene!=Scene::south146))throw std::invalid_argument("invalid private lever actor");
    agent::Packet out;const auto put=[&](std::uint32_t value,unsigned bytes){for(unsigned i=0;i<bytes;++i)out.push_back(std::uint8_t(value>>(8*i)));};
    const auto real=[&](float value){put(std::bit_cast<std::uint32_t>(value),4);};
    put(32,2);put(actor,4);put(door?door_model:lever_model,4);put(door?3:2,1);put(door?0:1,1);
    real(door?(scene==Scene::south146?-5480.f:-11712.f):lever_position.x);real(door?(scene==Scene::south146?12212.f:-17471.f):lever_position.z);put(door?(scene==Scene::south146?37:0):lever_plane,2);real(1);real(0);put(1,1);
    real(0);real(1);put(door?0x34000000:0x41200000,4);for(unsigned n=0;n<6;++n)put(0,4);
    real(0);real(0);put(0x7f800000,4);put(0x7f800000,4);put(0,2);put(0,4);
    put(0x7f800000,4);put(0x7f800000,4);put(0,2);return out;
}
}
inline agent::Packet lever_spawn_2022(std::uint32_t actor){return detail::nonliving_spawn_2022(actor,false);}
// Captured door type3, model1280, packet535509. This is a separate actor
// presentation from both the lever and the scene-object animation/state ID.
inline agent::Packet door_spawn_2022(std::uint32_t actor,Scene scene=Scene::south146){return detail::nonliving_spawn_2022(actor,true,scene);}
}
