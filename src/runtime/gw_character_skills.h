#pragma once
#include "gw_skillbar.h"
#include "gw_presearing_skills.h"
#include <string>

namespace fw::gw::skills {
// Versioned durable character state; absence is represented by no database row.
struct Profile {
    std::uint8_t primary=0,secondary=0;
    std::uint32_t profession_mask=0;
    Learned learned;
    Slots bar{};
    std::set<std::uint32_t> trial_skills;
};
// A session view, never durable character state. Only reciprocal native links
// with an explicitly flagged PvP counterpart qualify; sentinel3433 is no skill.
inline Profile match_profile(const Profile& saved,bool pvp_match){
    if(!pvp_match)return saved;
    Profile effective=saved;
    const auto variant=[](std::uint32_t id){
        const auto* base=find_skill(id);
        if(!base||(base->special&0x400000u))return id;
        const auto* other=find_skill(base->pvp_skill);
        return other&&(other->special&0x400000u)&&other->pvp_skill==id
            ?std::uint32_t(other->id):id;
    };
    // Aliases inherit only an already-owned copy count. They do not create
    // learning from the catalogue, or combine counts to grant additional copies.
    for(auto [id,count]:saved.learned){
        const auto next=variant(id);
        if(next!=id)effective.learned[next]=std::max(effective.learned[next],count);
    }
    for(auto& slot:effective.bar)slot.skill=variant(slot.skill);
    return effective;
}
inline Policy policy(const Profile& p,std::uint32_t campaigns,bool staging,bool pvp_match=false,bool trial_area=false){
    return {p.primary,p.secondary,campaigns,staging,p.learned,pvp_match,
        trial_area?p.trial_skills:std::set<std::uint32_t>{}};
}
// Session-only filtering retains the saved build, but cannot publish or cast
// trial skills outside pre-Searing. Returning to pre restores the durable bar.
inline Profile trial_view(const Profile& saved,bool authorized_area){
    if(authorized_area||saved.trial_skills.empty())return saved;
    auto effective=saved;
    for(auto& slot:effective.bar){const auto* row=find_skill(slot.skill);
        if(row&&row->profession&&row->profession!=saved.primary&&row->profession!=saved.secondary&&
            saved.trial_skills.contains(slot.skill))slot={};}
    effective.trial_skills.clear();return effective;
}
inline void validate_profile(const Profile& p){
    if(!p.primary||p.primary>10||p.secondary>10||(p.profession_mask&~0x7feu)||p.learned.size()>3432)
        throw std::invalid_argument("invalid character skill professions");
    for(auto [id,count]:p.learned)
        if(!id||!find_skill(id)||!count||count>65535)
            throw std::invalid_argument("invalid character skill learning");
    if(p.trial_skills.size()>44)throw std::invalid_argument("too many trial skills");
    if(!p.trial_skills.empty()&&(p.primary>6||p.secondary))
        throw std::invalid_argument("trial requires unchosen core secondary");
    for(auto id:p.trial_skills)if(!p.learned.contains(id)||!presearing::obtainable_skill(id))
        throw std::invalid_argument("trial skill must be learned");
    if(validate(p.bar,policy(p,0x1e,true,false,true))!=Reason::accepted)
        throw std::invalid_argument("invalid character skill bar");
}
inline Packet encode_profile(const Profile& p){
    validate_profile(p);Packet b{'F','W','S',static_cast<std::uint8_t>(p.trial_skills.empty()?1:2),p.primary,p.secondary};
    dword(b,p.profession_mask);word(b,static_cast<std::uint16_t>(p.learned.size()));
    for(auto [id,count]:p.learned){word(b,static_cast<std::uint16_t>(id));word(b,static_cast<std::uint16_t>(count));}
    for(auto slot:p.bar){dword(b,slot.skill);dword(b,slot.copy);}
    if(!p.trial_skills.empty()){word(b,static_cast<std::uint16_t>(p.trial_skills.size()));
        for(auto id:p.trial_skills)word(b,static_cast<std::uint16_t>(id));}
    return b;
}
inline Profile decode_profile(std::span<const std::uint8_t> b){
    if(b.size()<76||b.size()>13894||b[0]!='F'||b[1]!='W'||b[2]!='S'||(b[3]!=1&&b[3]!=2))
        throw std::invalid_argument("invalid character skill profile format");
    std::size_t at=6;auto read=[&](unsigned count){
        if(at+count>b.size())throw std::invalid_argument("truncated character skill profile");
        std::uint32_t n=0;for(unsigned i=0;i<count;++i)n|=std::uint32_t(b[at++])<<(i*8);return n;};
    Profile p;p.primary=b[4];p.secondary=b[5];p.profession_mask=read(4);
    const auto count=read(2);if(count>3432||(b[3]==1?b.size()!=76+count*4:b.size()<78+count*4))
        throw std::invalid_argument("invalid character skill profile length");
    std::uint32_t previous=0;
    for(unsigned i=0;i<count;++i){const auto id=read(2),copies=read(2);
        if(id<=previous)throw std::invalid_argument("character skill IDs must be strictly ordered");
        p.learned.emplace(id,copies);previous=id;}
    for(auto& slot:p.bar){slot.skill=read(4);slot.copy=read(4);}
    if(b[3]==2){const auto trials=read(2);if(!trials||trials>44||b.size()!=78+count*4+trials*2)
            throw std::invalid_argument("invalid trial skill length");
        previous=0;for(unsigned i=0;i<trials;++i){const auto id=read(2);
            if(id<=previous)throw std::invalid_argument("trial skill IDs must be strictly ordered");
            p.trial_skills.insert(id);previous=id;}}
    validate_profile(p);return p;
}
// These packets are owner-only. Caller may supply previously restored, proven
// learned IDs for clearing across character changes. Never sweep internal
// catalogue rows: wire shape alone does not prove those IDs safe to learn/remove.
inline std::vector<Packet> restore_packets(std::uint32_t actor,const Profile& p,
    std::span<const std::uint32_t> previous_learning={}){
    validate_profile(p);std::vector<Packet> result;
    auto professions=profession_packets_2022(actor,p.primary,p.secondary,p.profession_mask);
    result.insert(result.end(),professions.begin(),professions.end());
    for(auto id:previous_learning)if(!p.learned.contains(id))result.push_back(learned_count_packet_2022(id,0));
    result.push_back(learned_packet_2022(p.learned));
    for(auto [id,count]:p.learned)result.push_back(learned_count_packet_2022(id,count));
    result.push_back(bar_packet_2022(actor,p.bar,true));return result;
}
// Native226 acknowledges one pending skill/copy reference. It does not
// stop the character's cast action; that is the separate generic kind59.
// Successful casts also need this acknowledgment before228 recharge.
inline Packet cancellation_packet(std::uint32_t actor,Slot slot){
    if(!actor||slot.skill>3432)throw std::invalid_argument("invalid skill cancellation");
    Packet p;word(p,226);dword(p,actor);word(p,static_cast<std::uint16_t>(slot.skill));dword(p,slot.copy);return p;
}
}
