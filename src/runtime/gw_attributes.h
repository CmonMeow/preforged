#pragma once
#include "gw_outgoing_frame.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace fw::gw::attributes {
using Packet=std::vector<std::uint8_t>;
using Ranks=std::array<std::uint8_t,51>;
using Bonuses=std::array<std::int16_t,51>;
// Exact 2022 constants VA980de0, 51 records of 20 bytes; bit 0 is primary-only.
// Profession11 rows are engine/nonplayer attributes, never player allocations.
struct Info {std::uint8_t profession;bool primary_only;};
inline constexpr std::array<Info,51> catalog{{
    {5,true},{5,false},{5,false},{5,false},{4,false},{4,false},{4,true},{4,false},
    {6,false},{6,false},{6,false},{6,false},{6,true},{3,false},{3,false},{3,false},{3,true},
    {1,true},{1,false},{1,false},{1,false},{1,false},{2,false},{2,true},{2,false},{2,false},
    {11,false},{11,false},{11,false},{7,false},{7,false},{7,false},{8,false},{8,false},{8,false},
    {7,true},{8,true},{9,false},{9,false},{9,false},{9,true},{10,false},{10,false},{10,false},{10,true},
    {11,false},{11,false},{11,false},{11,false},{11,false},{11,false}
}};
// Pinned ConstAttrib cost table a0b4c4 and local Attribute_point.html agree.
inline constexpr std::array<unsigned,13> cumulative_cost{0,1,3,6,10,15,21,28,37,48,61,77,97};
inline constexpr std::array<unsigned,20> level_point_table{0,5,10,15,20,25,30,35,40,45,55,65,75,85,95,110,125,140,155,170};
inline unsigned level_points(unsigned level){
    if(!level||level>20)throw std::invalid_argument("attribute level outside 1..20");
    return level_point_table[level-1];
}
inline unsigned used_points(const Ranks& ranks){
    unsigned used=0;
    for(unsigned id=0;id<ranks.size();++id){
        if(ranks[id]>12||(ranks[id]&&catalog[id].profession>10))
            throw std::invalid_argument("invalid base attribute ranks");
        used+=cumulative_cost[ranks[id]];
    }
    return used;
}
enum class PointsSource:std::uint8_t {level_baseline=0,minimum_inferred=1,captured_exact=2,development=3};
struct Profile {
    Ranks ranks{};
    std::uint8_t quest_bonus=0;
    PointsSource source=PointsSource::level_baseline;
    static constexpr std::size_t storage_size=54;
    unsigned total_points(unsigned level)const{return level_points(level)+quest_bonus;}
    std::array<std::uint8_t,storage_size> encode()const{
        if(quest_bonus>30||quest_bonus%15||unsigned(source)>3)
            throw std::invalid_argument("invalid attribute point provenance");
        (void)used_points(ranks);
        std::array<std::uint8_t,storage_size> data{1,static_cast<std::uint8_t>(source),quest_bonus};
        std::copy(ranks.begin(),ranks.end(),data.begin()+3);return data;
    }
    static Profile decode(std::span<const std::uint8_t> data){
        if(data.size()!=storage_size||data[0]!=1)throw std::invalid_argument("invalid attribute profile version/size");
        Profile profile;profile.source=static_cast<PointsSource>(data[1]);profile.quest_bonus=data[2];
        std::copy(data.begin()+3,data.end(),profile.ranks.begin());(void)profile.encode();return profile;
    }
    // Actual captured base allocations prove a minimum quest award only. This
    // never grants unused unobserved quest points. At level 20, spent 200 proves 30.
    static Profile from_captured(Ranks ranks,unsigned level){
        Profile p;p.ranks=ranks;const auto used=used_points(ranks),baseline=level_points(level);
        if(used>baseline+30)throw std::invalid_argument("captured attributes exceed possible earned points");
        p.quest_bonus=used>baseline+15?30:used>baseline?15:0;
        if(p.quest_bonus)p.source=PointsSource::minimum_inferred;
        return p;
    }
};
struct Policy {
    unsigned primary=0,secondary=0,total_points=0;
    bool can_increase=false,can_decrease=false;
};
struct Mode {bool can_increase=false,can_decrease=false;};
// Pinned MapTypeInstanceInfo VAa0dd80: request-instance types, not RegionType.
// Town/outpost and guild-hall staging instances permit refunds. Active PvE
// permits spending unused points; active PvP, preview and unknown modes deny.
inline constexpr Mode mode_2022(unsigned request_instance_type){
    switch(request_instance_type){
    case 0:case 2:case 3:case 5:case 7:case 15:return {true,true};
    case 1:case 4:return {true,false};
    default:return {};
    }
}
inline constexpr bool is_outpost_type(unsigned type){return mode_2022(type).can_decrease;}
inline constexpr bool can_increase_type(unsigned type){return mode_2022(type).can_increase;}
inline bool valid_policy(const Policy& p){return p.primary>=1&&p.primary<=10&&p.secondary<=10&&p.secondary!=p.primary&&p.total_points<=200;}
inline bool available(unsigned id,const Policy& p){
    return id<catalog.size()&&catalog[id].profession<=10&&
        (catalog[id].profession==p.primary||(catalog[id].profession==p.secondary&&!catalog[id].primary_only));
}
enum class Reason {accepted,owner,policy,attribute,rank,duplicate,points,increase_mode,decrease_mode};
inline const char* reason_text(Reason reason){
    switch(reason){
    case Reason::accepted:return "accepted";
    case Reason::owner:return "attribute actor is not owned";
    case Reason::policy:return "invalid attribute profession or point entitlement";
    case Reason::attribute:return "attribute is unavailable to current professions";
    case Reason::rank:return "base attribute rank is outside 0..12";
    case Reason::duplicate:return "attribute appears twice in allocation";
    case Reason::points:return "allocated ranks exceed earned attribute points";
    case Reason::increase_mode:return "attribute increases are not allowed in this instance";
    case Reason::decrease_mode:return "attribute refunds are not allowed in this instance";
    }return "invalid attribute request";
}
inline Reason validate(const Ranks& ranks,const Policy& p){
    if(!valid_policy(p))return Reason::policy;
    unsigned spent=0;
    for(unsigned id=0;id<ranks.size();++id){
        if(ranks[id]>12)return Reason::rank;
        if(ranks[id]&&!available(id,p))return Reason::attribute;
        spent+=cumulative_cost[ranks[id]];
    }
    return spent<=p.total_points?Reason::accepted:Reason::points;
}
inline void word(Packet& packet,unsigned value){packet.push_back(static_cast<std::uint8_t>(value));packet.push_back(static_cast<std::uint8_t>(value>>8));}
inline void dword(Packet& packet,std::uint32_t value){for(unsigned n=0;n<4;++n)packet.push_back(static_cast<std::uint8_t>(value>>(8*n)));}
inline Packet initial_packet_2022(std::uint32_t actor,unsigned available_points,unsigned total){
    if(!actor||total>200||available_points>total)throw std::invalid_argument("invalid initial attribute points");
    Packet p;word(p,55);dword(p,actor);p.push_back(static_cast<std::uint8_t>(available_points));p.push_back(static_cast<std::uint8_t>(total));return p;
}
inline Packet points_packet_2022(std::uint32_t actor,unsigned available_points,unsigned total,bool available_value){
    if(!actor||total>200||available_points>total)throw std::invalid_argument("invalid attribute points update");
    Packet p;word(p,available_value?56:57);dword(p,actor);p.push_back(static_cast<std::uint8_t>(available_value?available_points:total));return p;
}
inline Packet snapshot_packet_2022(std::uint32_t actor,const Ranks& ranks,const Policy& policy,const Bonuses& bonuses={}){
    if(!actor||validate(ranks,policy)!=Reason::accepted)throw std::invalid_argument("invalid attribute snapshot");
    std::vector<unsigned> ids;for(unsigned id=0;id<ranks.size();++id)if(available(id,policy))ids.push_back(id);
    if(ids.size()>16)throw std::invalid_argument("attribute snapshot exceeds pinned count");
    Packet p;word(p,58);dword(p,actor);word(p,static_cast<unsigned>(3*ids.size()));
    for(auto id:ids)dword(p,id);
    for(auto id:ids)dword(p,ranks[id]);
    for(auto id:ids){const auto effective=std::max(0,int(ranks[id])+int(bonuses[id]));
        dword(p,static_cast<unsigned>(effective));}
    return p;
}
inline std::vector<Packet> packets_2022(std::uint32_t actor,const Ranks& ranks,const Policy& policy,const Bonuses& bonuses={}){
    if(validate(ranks,policy)!=Reason::accepted)throw std::invalid_argument("invalid current attribute state");
    const auto available_points=policy.total_points-used_points(ranks);
    return {snapshot_packet_2022(actor,ranks,policy,bonuses),
        points_packet_2022(actor,available_points,policy.total_points,true),
        points_packet_2022(actor,available_points,policy.total_points,false)};
}
enum class Change {decrease,increase,load};
struct Allocation {std::uint32_t id=0,rank=0;};
struct Request {
    Change change=Change::load;
    std::uint32_t actor=0,token=0,attribute=0;
    std::array<Allocation,16> allocations{};
    unsigned count=0;
    std::size_t consumed=0;
};
inline world::FrameState parse_request(std::span<const std::uint8_t> bytes,Request& result){
    if(bytes.size()<2)return world::FrameState::incomplete;
    const auto opcode=(bytes[0]|(unsigned(bytes[1])<<8))&0x7fff;
    if(opcode!=12&&opcode!=13&&opcode!=14)return world::FrameState::unknown_opcode;
    const auto frame=world::outgoing_frame_2022(bytes);if(frame.state!=world::FrameState::complete)return frame.state;
    auto read=[&](unsigned at){return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|
        (std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);};
    Request request;request.actor=read(2);request.consumed=frame.size;
    if(opcode==12||opcode==13){request.change=opcode==12?Change::decrease:Change::increase;request.token=read(6);request.attribute=read(10);}
    else {
        request.count=bytes[6]|(unsigned(bytes[7])<<8);
        const auto at=8+4*request.count;const auto ranks_count=bytes[at]|(unsigned(bytes[at+1])<<8);
        if(ranks_count!=request.count)return world::FrameState::invalid;
        for(unsigned n=0;n<request.count;++n)request.allocations[n]={read(8+4*n),read(at+2+4*n)};
    }
    result=request;return world::FrameState::complete;
}
class AttributeState {
    std::uint32_t actor_;Ranks ranks_{};
public:
    explicit AttributeState(std::uint32_t actor):actor_(actor){if(!actor)throw std::invalid_argument("attribute actor must be nonzero");}
    const Ranks& ranks()const{return ranks_;}
    Reason restore(const Ranks& ranks,const Policy& policy){const auto reason=validate(ranks,policy);if(reason==Reason::accepted)ranks_=ranks;return reason;}
    struct Update {
        Reason status;Ranks proposed;
        std::vector<Packet> authoritative;
        std::uint32_t actor=0;Ranks previous{};
        bool accepted()const{return status==Reason::accepted;}
    };
    // Proposal is read-only. The caller saves proposed ranks with owned-character
    // CAS first, then commits and publishes; a failed save cannot mutate state.
    Update propose(const Request& request,const Policy& policy,const Bonuses& bonuses={})const{
        auto finish=[&](Reason reason,const Ranks& proposal){return Update{reason,proposal,
            packets_2022(actor_,reason==Reason::accepted?proposal:ranks_,policy,bonuses),actor_,ranks_};};
        if(!valid_policy(policy))throw std::invalid_argument("invalid current attribute policy");
        if(request.actor!=actor_)return finish(Reason::owner,ranks_);
        auto proposed=ranks_;
        if(request.change==Change::load){
            if(request.count>16)return finish(Reason::attribute,ranks_);
            proposed={};std::array<bool,51> seen{};
            for(unsigned n=0;n<request.count;++n){const auto [id,rank]=request.allocations[n];
                if(!available(id,policy))return finish(Reason::attribute,ranks_);
                if(rank>12)return finish(Reason::rank,ranks_);
                if(seen[id])return finish(Reason::duplicate,ranks_);seen[id]=true;proposed[id]=static_cast<std::uint8_t>(rank);
            }
        }else {
            const auto id=request.attribute;if(!available(id,policy))return finish(Reason::attribute,ranks_);
            if(request.change==Change::increase){if(proposed[id]==12)return finish(Reason::rank,ranks_);++proposed[id];}
            else {if(!proposed[id])return finish(Reason::rank,ranks_);--proposed[id];}
        }
        for(unsigned id=0;id<ranks_.size();++id){
            if(proposed[id]>ranks_[id]&&!policy.can_increase)return finish(Reason::increase_mode,ranks_);
            if(proposed[id]<ranks_[id]&&!policy.can_decrease)return finish(Reason::decrease_mode,ranks_);
        }
        const auto reason=validate(proposed,policy);return finish(reason,proposed);
    }
    void commit(const Update& update,const Policy& policy){
        if(!update.accepted()||update.actor!=actor_||update.previous!=ranks_||
                validate(update.proposed,policy)!=Reason::accepted)
            throw std::invalid_argument("cannot commit denied, foreign or stale attribute update");
        ranks_=update.proposed;
    }
};
}
