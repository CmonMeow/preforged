#pragma once
#include "gw_skill_catalog_generated.h"
#include "gw_outgoing_frame.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <vector>

namespace fw::gw::skills {
using Packet=std::vector<std::uint8_t>;
struct Slot {
    std::uint32_t skill=0, copy=0;
    friend bool operator==(const Slot&,const Slot&)=default;
};
using Slots=std::array<Slot,8>;
// Explicit character learning/copy counts. Campaign access is not learning and
// account unlocks are a different state. This module grants neither of them.
using Learned=std::map<std::uint32_t,std::uint32_t>;
struct Policy {
    std::uint8_t primary=0, secondary=0;
    std::uint32_t campaigns=0; // bitN is owned campaignN; Core0 needs no bit.
    bool can_change=false; // Caller derives this from authoritative instance mode.
    Learned learned;
    bool pvp_match=false; // Authoritative instance rule; does not erase equipped PvE skills.
    std::set<std::uint32_t> trial_skills; // Explicit map-scoped grants, never a profession unlock.
};
enum class Reason {
    accepted, owner, mode, slot, unknown_skill, not_learned, copy,
    profession, campaign, multiple_elites, multiple_pve_skills, missing_slot, occupied_slot
};
inline const char* reason_text(Reason reason){
    switch(reason){
    case Reason::accepted:return "accepted";
    case Reason::owner:return "actor is not owned";
    case Reason::mode:return "skill changes are not allowed in this instance";
    case Reason::slot:return "skill slot is outside the bar";
    case Reason::unknown_skill:return "skill is outside the pinned2022 catalogue";
    case Reason::not_learned:return "character has not learned this skill";
    case Reason::copy:return "skill copy is unavailable or repeated";
    case Reason::profession:return "skill does not match primary or secondary profession";
    case Reason::campaign:return "skill campaign is unavailable";
    case Reason::multiple_elites:return "bar contains more than one elite";
    case Reason::multiple_pve_skills:return "bar contains more than three PvE-only skills";
    case Reason::missing_slot:return "requested skill copy is not in the bar";
    case Reason::occupied_slot:return "destination slot is occupied";
    }
    return "invalid skillbar state";
}
inline Reason validate(const Slots& bar,const Policy& policy){
    std::set<std::pair<std::uint32_t,std::uint32_t>> copies;
    unsigned elites=0,pve_slots=0;
    for(const auto& slot:bar){
        if(!slot.skill){if(slot.copy)return Reason::copy;continue;}
        const auto* skill=find_skill(slot.skill);
        if(!skill)return Reason::unknown_skill;
        const auto known=policy.learned.find(slot.skill);
        if(known==policy.learned.end()||!known->second)return Reason::not_learned;
        if(slot.copy>=known->second||!copies.emplace(slot.skill,slot.copy).second)return Reason::copy;
        if(skill->profession&&skill->profession!=policy.primary&&skill->profession!=policy.secondary&&
            !policy.trial_skills.contains(slot.skill))
            return Reason::profession;
        if(skill->campaign&&!(policy.campaigns&(1u<<skill->campaign)))return Reason::campaign;
        if(skill->elite()&&++elites>1)return Reason::multiple_elites;
        // Pinned2022 special bit matches the reviewed PvE-only skill list.
        // Each equipped copy counts, including Signet of Capture (skill 3).
        if((skill->special&0x80000u)&&++pve_slots>3)return Reason::multiple_pve_skills;
    }
    return Reason::accepted;
}

inline void word(Packet& p,std::uint16_t value){p.push_back(value&255);p.push_back(value>>8);}
inline void dword(Packet& p,std::uint32_t value){for(unsigned n=0;n<4;++n)p.push_back(value>>(8*n));}
inline Packet slot_packet_2022(std::uint32_t actor,unsigned index,Slot slot){
    if(!actor||index>=8||!find_skill(slot.skill)||(!slot.skill&&slot.copy))
        throw std::invalid_argument("invalid skillbar slot packet");
    Packet packet;word(packet,216);dword(packet,actor);packet.push_back(index);
    word(packet,static_cast<std::uint16_t>(slot.skill));dword(packet,slot.copy);return packet;
}
inline Packet bar_packet_2022(std::uint32_t actor,const Slots& bar,bool create=false){
    if(!actor)throw std::invalid_argument("invalid skillbar actor");
    Packet packet;word(packet,217);dword(packet,actor);word(packet,8);
    for(const auto& slot:bar){
        if(!find_skill(slot.skill)||(!slot.skill&&slot.copy))
            throw std::invalid_argument("invalid skillbar snapshot");
        dword(packet,slot.skill);
    }
    word(packet,8);for(const auto& slot:bar)dword(packet,slot.copy);
    packet.push_back(create);return packet;
}
inline Packet learned_packet_2022(const Learned& learned){
    std::array<std::uint32_t,108> words{};std::size_t count=0;
    for(const auto& [id,copies]:learned){
        if(!id||!find_skill(id))throw std::invalid_argument("invalid learned skill ID");
        if(!copies)continue;
        words[id/32]|=1u<<(id%32);count=std::max(count,std::size_t(id/32+1));
    }
    Packet packet;word(packet,218);word(packet,static_cast<std::uint16_t>(count));
    for(std::size_t index=0;index<count;++index)dword(packet,words[index]);
    return packet;
}

// 219 notifies genuine new learning and displays the client's new-skill UI.
// Its retail handler asserts count != 0. Count is total available copies,
// not the zero-based copy index carried in skillbar slots.
inline Packet learning_packet_2022(std::uint32_t skill,std::uint32_t count){
    if(!skill||!find_skill(skill)||!count)
        throw std::invalid_argument("invalid new learned skill count");
    Packet packet;word(packet,219);word(packet,static_cast<std::uint16_t>(skill));
    dword(packet,count);return packet;
}
// 220 silently restores/updates learning: 0 removes learning and extra copies,
// 1 keeps a single learned copy, >1 sets explicit total copies. No eight-copy
// cap applies: stored Signet of Capture counts may exceed eight equipped slots.
inline Packet learned_count_packet_2022(std::uint32_t skill,std::uint32_t count){
    if(!skill||!find_skill(skill))
        throw std::invalid_argument("invalid restored learned skill count");
    Packet packet;word(packet,220);word(packet,static_cast<std::uint16_t>(skill));
    dword(packet,count);return packet;
}
// 182 must precede 181: it creates profession state; an unlock-mask update for
// a missing state is ignored. The final182 BYTE is an independently verified
// boolean at ProfessionState+0x10, not the unlocked-mask DWORD at +0x0c.
// Its broader meaning is unproved, so retain an explicit, unlabelled state flag.
inline std::array<Packet,2> profession_packets_2022(std::uint32_t actor,
    unsigned primary,unsigned secondary,std::uint32_t unlocked_mask,bool state_flag=false){
    if(!actor||primary>10||secondary>10)
        throw std::invalid_argument("invalid character profession state");
    std::array<Packet,2> packets;
    word(packets[0],182);dword(packets[0],actor);
    packets[0].push_back(static_cast<std::uint8_t>(primary));
    packets[0].push_back(static_cast<std::uint8_t>(secondary));
    packets[0].push_back(state_flag);
    word(packets[1],181);dword(packets[1],actor);dword(packets[1],unlocked_mask);
    return packets;
}

enum class Change { set, load, swap, move };
struct Request {
    Change change=Change::load;
    std::uint32_t actor=0, slot=0;
    Slot first, second;
    std::array<std::uint32_t,8> ids{};
    unsigned count=0;
    std::size_t consumed=0;
};
// Framing is shared with all other2022 CMSG branches. On partial/invalid input
// leave the caller's request untouched; coalesced next packets are not consumed.
inline world::FrameState parse_request(std::span<const std::uint8_t> bytes,Request& result){
    if(bytes.size()<2)return world::FrameState::incomplete;
    const auto opcode=(bytes[0]|(unsigned(bytes[1])<<8))&0x7fff;
    if(opcode<90||opcode>93)return world::FrameState::unknown_opcode;
    const auto frame=world::outgoing_frame_2022(bytes);
    if(frame.state!=world::FrameState::complete)return frame.state;
    auto read=[&](std::size_t at){return std::uint32_t(bytes[at])|
        (std::uint32_t(bytes[at+1])<<8)|(std::uint32_t(bytes[at+2])<<16)|
        (std::uint32_t(bytes[at+3])<<24);};
    Request request;request.actor=read(2);request.consumed=frame.size;
    if(opcode==90){request.change=Change::set;request.slot=read(6);request.first={read(10),read(14)};}
    else if(opcode==91){
        request.change=Change::load;request.count=bytes[6]|(unsigned(bytes[7])<<8);
        for(unsigned n=0;n<request.count;++n)request.ids[n]=read(8+4*n);
    }
    else if(opcode==92){request.change=Change::swap;request.first={read(6),read(10)};
        request.second={read(14),read(18)};}
    else {request.change=Change::move;request.first={read(6),read(10)};request.slot=read(14);}
    result=request;return world::FrameState::complete;
}

class SkillbarState {
    std::uint32_t actor_;
    Slots bar_{};
public:
    explicit SkillbarState(std::uint32_t actor):actor_(actor){
        if(!actor_)throw std::invalid_argument("skillbar actor must be nonzero");
    }
    const Slots& slots()const{return bar_;}
    std::uint32_t actor()const{return actor_;}
    Packet snapshot(bool create=false)const{return bar_packet_2022(actor_,bar_,create);}
    // Restore explicit persistent data using the same constraints as a new bar.
    // No default learned skills, imported secondary, or fabricated bar is inferred.
    Reason restore(const Slots& slots,const Policy& policy){
        const auto status=validate(slots,policy);
        if(status==Reason::accepted)bar_=slots;
        return status;
    }
    struct Result {Reason status;Packet authoritative;bool accepted()const{return status==Reason::accepted;}};
    Result apply(const Request& request,const Policy& policy){
        auto finish=[&](Reason reason){return Result{reason,snapshot()};};
        if(request.actor!=actor_)return finish(Reason::owner);
        if(!policy.can_change)return finish(Reason::mode);
        auto proposed=bar_;
        auto locate=[&](Slot value){return std::find(proposed.begin(),proposed.end(),value);};
        switch(request.change){
        case Change::set:
            if(request.slot>=8)return finish(Reason::slot);
            proposed[request.slot]=request.first;break;
        case Change::load:
            if(request.count>8)return finish(Reason::slot);
            proposed={};
            for(unsigned n=0;n<request.count;++n){
                std::uint32_t copy=0;
                if(request.ids[n])for(unsigned previous=0;previous<n;++previous)
                    if(request.ids[previous]==request.ids[n])++copy;
                proposed[n]={request.ids[n],copy};
            }
            break;
        case Change::swap:{
            const auto first=locate(request.first),second=locate(request.second);
            if(!request.first.skill||!request.second.skill||first==proposed.end()||second==proposed.end())
                return finish(Reason::missing_slot);
            std::iter_swap(first,second);break;
        }
        case Change::move:{
            if(request.slot>=8)return finish(Reason::slot);
            const auto first=locate(request.first);
            if(!request.first.skill||first==proposed.end())return finish(Reason::missing_slot);
            if(proposed[request.slot].skill&&proposed[request.slot]!=request.first)
                return finish(Reason::occupied_slot);
            std::iter_swap(first,proposed.begin()+request.slot);break;
        }
        }
        const auto status=validate(proposed,policy);
        if(status==Reason::accepted)bar_=proposed;
        // Both success and denial reconcile the client's optimistic skill edits.
        return finish(status);
    }
};
}
