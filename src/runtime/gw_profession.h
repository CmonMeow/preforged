#pragma once
#include "gw_attributes.h"
#include "gw_character_skills.h"
#include <iterator>

namespace fw::gw::professions {
using Packet=std::vector<std::uint8_t>;
struct Request {std::uint32_t actor=0;unsigned secondary=0;std::size_t consumed=0;};
inline world::FrameState parse_request(std::span<const std::uint8_t> bytes,Request& result){
    if(bytes.size()<2)return world::FrameState::incomplete;
    const auto opcode=(bytes[0]|(unsigned(bytes[1])<<8))&0x7fff;
    if(opcode!=63)return world::FrameState::unknown_opcode;
    const auto frame=world::outgoing_frame_2022(bytes);
    if(frame.state!=world::FrameState::complete)return frame.state;
    Request request;for(unsigned n=0;n<4;++n)request.actor|=std::uint32_t(bytes[2+n])<<(8*n);
    request.secondary=bytes[6];request.consumed=frame.size;result=request;
    return world::FrameState::complete;
}
enum class Reason {accepted,owner,mode,profession,locked};
inline const char* reason_text(Reason reason){
    switch(reason){
    case Reason::accepted:return "accepted";
    case Reason::owner:return "profession actor is not owned";
    case Reason::mode:return "profession changes require a staging instance";
    case Reason::profession:return "invalid secondary profession";
    case Reason::locked:return "secondary profession has not been unlocked";
    }return "invalid profession request";
}
struct Policy {
    std::uint32_t owned_actor=0;
    unsigned map_type=0;
    std::uint32_t campaigns=0;
    unsigned level=0;
    bool state_flag=false;
    bool trial_area=false;
};
inline attributes::Policy attribute_policy(const skills::Profile& skill_profile,
    const attributes::Profile& attribute_profile,const Policy& policy){
    const auto mode=attributes::mode_2022(policy.map_type);
    return {skill_profile.primary,skill_profile.secondary,attribute_profile.total_points(policy.level),
        mode.can_increase,mode.can_decrease};
}
// SMSG165 updates the visible agent's primary/secondary bytes independently of
// ProfessionState182. Primary is immutable for secondary-change requests.
inline Packet agent_packet_2022(std::uint32_t actor,unsigned primary,unsigned secondary){
    if(!actor||!primary||primary>10||secondary>10||secondary==primary)
        throw std::invalid_argument("invalid visible agent professions");
    Packet packet;skills::word(packet,165);skills::dword(packet,actor);
    packet.push_back(static_cast<std::uint8_t>(primary));
    packet.push_back(static_cast<std::uint8_t>(secondary));return packet;
}
inline std::vector<Packet> packets_2022(const skills::Profile& skill_profile,
    const attributes::Profile& attribute_profile,const Policy& policy,const attributes::Bonuses& bonuses={}){
    if(!policy.owned_actor)throw std::invalid_argument("profession owner must be nonzero");
    skills::validate_profile(skill_profile);
    const auto p=attribute_policy(skill_profile,attribute_profile,policy);
    if(attributes::validate(attribute_profile.ranks,p)!=attributes::Reason::accepted||
        skills::validate(skill_profile.bar,skills::policy(skill_profile,policy.campaigns,true,false,policy.trial_area))!=skills::Reason::accepted)
        throw std::invalid_argument("invalid current character allocation");
    std::vector<Packet> result{agent_packet_2022(policy.owned_actor,skill_profile.primary,skill_profile.secondary)};
    auto professions=skills::profession_packets_2022(policy.owned_actor,skill_profile.primary,
        skill_profile.secondary,skill_profile.profession_mask,policy.state_flag);
    result.insert(result.end(),std::make_move_iterator(professions.begin()),std::make_move_iterator(professions.end()));
    // 55 already exists. Never recreate it on profession edits. 182 must precede
    // 58: the latter's retail callback queries the initialized profession state.
    auto attribute_packets=attributes::packets_2022(policy.owned_actor,attribute_profile.ranks,p,bonuses);
    result.insert(result.end(),std::make_move_iterator(attribute_packets.begin()),std::make_move_iterator(attribute_packets.end()));
    result.push_back(skills::bar_packet_2022(policy.owned_actor,skill_profile.bar,false));return result;
}
struct Update {
    Reason status=Reason::profession;
    bool changed=false;
    skills::Profile skill_profile;
    attributes::Profile attribute_profile;
    std::vector<Packet> authoritative;
    bool accepted()const{return status==Reason::accepted;}
};
// Pure proposal. Caller must atomically CAS both owned-character profiles before
// restoring its in-memory state and publishing any changed packets. Denials
// reconcile the original profession, allocation and bar; no learning is granted.
inline Update propose(const Request& request,const skills::Profile& current_skills,
    const attributes::Profile& current_attributes,const Policy& policy,const attributes::Bonuses& bonuses={}){
    const auto unchanged=packets_2022(current_skills,current_attributes,policy,bonuses);
    auto deny=[&](Reason reason){return Update{reason,false,current_skills,current_attributes,unchanged};};
    if(request.actor!=policy.owned_actor)return deny(Reason::owner);
    if(!attributes::is_outpost_type(policy.map_type))return deny(Reason::mode);
    if(request.secondary>10||request.secondary==current_skills.primary||
        (!request.secondary&&current_skills.secondary))return deny(Reason::profession);
    // Exact native UI4f1780 allows the current secondary without its mask bit.
    // It excludes the primary independently, so primary's absent bit is valid.
    if(request.secondary!=current_skills.secondary&&
        !(current_skills.profession_mask&(1u<<request.secondary)))return deny(Reason::locked);
    if(request.secondary==current_skills.secondary)
        return {Reason::accepted,false,current_skills,current_attributes,unchanged};
    auto next_skills=current_skills;auto next_attributes=current_attributes;
    next_skills.secondary=static_cast<std::uint8_t>(request.secondary);
    next_skills.trial_skills.clear();
    for(auto& slot:next_skills.bar){if(!slot.skill)continue;const auto* skill=skills::find_skill(slot.skill);
        if(skill->profession&&skill->profession!=next_skills.primary&&skill->profession!=next_skills.secondary)slot={};}
    const auto p=attribute_policy(next_skills,next_attributes,policy);
    for(unsigned id=0;id<next_attributes.ranks.size();++id)
        if(!attributes::available(id,p))next_attributes.ranks[id]=0;
    return {Reason::accepted,true,next_skills,next_attributes,packets_2022(next_skills,next_attributes,policy,bonuses)};
}
}
