#pragma once
#include "gw_outgoing_frame.h"
#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>
namespace fw::gw::creation {
// Build37130 native873340 tableA0B468: fields0..7. Catalog group4 is
// independent from the account roster's origin-campaign label.
inline std::uint32_t appearance_field(std::uint32_t appearance,unsigned field){
    constexpr std::array<unsigned,8> shifts{15,10,24,20,30,1,0,5};
    constexpr std::array<unsigned,8> widths{5,5,6,4,2,4,1,5};
    if(field>=shifts.size())throw std::invalid_argument("appearance field");
    return (appearance>>shifts[field])&((1u<<widths[field])-1);
}
// Native8770d0 sends campaign then profession as BYTE fields. Matching GWCA
// Campaign enum: Core0, Prophecies1, Factions2, Nightfall3; expansions are not
// character-creation campaigns. Profession remains a selected class1..10.
inline bool valid_preview_selection(unsigned campaign,unsigned profession){return campaign<=3&&profession>=1&&profession<=10;}
inline std::uint32_t preview_appearance(std::uint32_t appearance,unsigned profession,unsigned campaign){
    if(profession<1||profession>10||campaign>3)throw std::invalid_argument("preview appearance selection");
    appearance=(appearance&~(15u<<20))|(profession<<20);
    // Native crcontext4cca50 stores campaign-1 as catalog group; the previous
    // selected character's campaign must not leak into this preview response.
    if(campaign)appearance=(appearance&~(3u<<30))|((campaign-1u)<<30);
    return appearance;
}
// Campaign start maps from the pinned GWCA MapID enum: pre-searing Ascalon,
// Monastery Overlook and Island of Shehkah. These are creation destinations,
// not ordinary travel arrivals.
inline unsigned starting_map(unsigned campaign){
    switch(campaign){case 1:return 148;case 2:return 212;case 3:return 490;default:return 0;}
}
inline bool authorized_preview(unsigned campaign,unsigned profession,unsigned owned){
    if(!starting_map(campaign)||!(owned&(1u<<campaign)))return false;
    return profession>=1&&profession<=10&&
        ((campaign==1&&profession<=6)||(campaign==2&&profession<=8)||
         (campaign==3&&(profession<=6||profession>=9)));
}
struct Request {
    std::u16string name;
    std::array<std::uint8_t,8> appearance{};
    std::size_t consumed=0;
};
inline bool authorized_request(const Request& request,unsigned campaign,unsigned profession,unsigned owned){
    std::uint32_t appearance=0,reserved=0;
    for(unsigned i=0;i<4;++i){appearance|=std::uint32_t(request.appearance[i])<<(8*i);reserved|=std::uint32_t(request.appearance[i+4])<<(8*i);}
    return authorized_preview(campaign,profession,owned)&&((appearance>>20)&15)==profession&&!reserved;
}
// Build 37130 sender 7d07f0: CMSG 137, UTF16[20], opaque BYTE[8].
// Account identity comes from the authenticated session, never these fields.
inline world::FrameState parse_request(std::span<const std::uint8_t> bytes,Request& out){
    using State=world::FrameState;
    if(bytes.size()<2)return State::incomplete;
    if(((bytes[0]|(unsigned(bytes[1])<<8))&0x7fff)!=137)return State::invalid;
    const auto frame=world::outgoing_frame_2022(bytes);
    if(frame.state!=State::complete)return frame.state;
    const unsigned count=bytes[2]|(unsigned(bytes[3])<<8);
    if(!count)return State::invalid;
    Request result;
    for(unsigned i=0;i<count;++i){
        const auto c=char16_t(bytes[4+i*2]|(unsigned(bytes[5+i*2])<<8));
        if(!c)return State::invalid;
        result.name+=c;
    }
    std::copy_n(bytes.begin()+4+count*2,8,result.appearance.begin());
    result.consumed=frame.size;out=std::move(result);return State::complete;
}
}

