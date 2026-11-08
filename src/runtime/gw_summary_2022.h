#pragma once
#include "gw_character_profile.h"
#include <algorithm>

namespace fw::gw {
// Registry 60bc58 -> callback 874570 -> 7937e0 writes WorldContext+688.
// The native roster writer later reads this value through 797c70.
inline std::vector<std::uint8_t> origin_campaign_packet_2022(std::uint8_t campaign) {
    if(campaign>15)throw std::invalid_argument("campaign summary nibble overflow");
    return {123,0,campaign,0,0,0};
}
// Version-6 layout verified against retail writer 87c2f0 and decoder 87c3e0.
// Unidentified fields remain explicitly opaque; layout does not establish
// their gameplay meaning or appropriate values for a new character.
struct Summary2022 {
    std::uint32_t appearance=0,opaque_metadata=0;
    std::array<std::uint32_t,4> opaque_words{};
    std::uint16_t map=0;
    // 49be60 -> 7eb6b0: bit9 comes from Player.flags bit11 (PvP).
    // Bits10..13 come from 7638f0's second profession result.
    // Low nibble is origin campaign: native roster label verified with1/3.
    std::uint8_t level=0,low_nibble=0,is_pvp=0,secondary_profession=0,flag14=0;
    std::vector<CharacterProfile::EquipmentAppearance> equipment;
    std::vector<std::uint8_t> encode()const{
        if(!map||level>31||low_nibble>15||is_pvp>1||secondary_profession>10||flag14>1||equipment.size()>5)
            throw std::invalid_argument("2022 character summary bounds");
        std::vector<std::uint8_t> out(37+equipment.size()*5,0);
        auto put=[&](std::size_t at,std::uint32_t value,unsigned count){
            for(unsigned i=0;i<count;++i)out[at+i]=static_cast<std::uint8_t>(value>>(8*i));
        };
        put(0,6,2);put(2,map,2);put(4,opaque_metadata,4);put(8,appearance,4);
        for(unsigned i=0;i<4;++i)put(12+4*i,opaque_words[i],4);
        put(28,low_nibble|(std::uint32_t(level)<<4)|(std::uint32_t(is_pvp)<<9)
            |(std::uint32_t(secondary_profession)<<10)|(std::uint32_t(flag14)<<14),4);
        out[32]=static_cast<std::uint8_t>(equipment.size());
        for(std::size_t i=0;i<equipment.size();++i){
            put(37+5*i,equipment[i].composite,2);put(39+5*i,equipment[i].color,2);
            put(41+5*i,equipment[i].parameter,1);
        }
        return out;
    }
    CharacterProfile profile(std::uint32_t envelope_unknown=0)const{
        auto summary=encode();std::vector<std::uint8_t> storage(CharacterProfile::storage_size,0);
        for(unsigned i=0;i<4;++i)storage[i]=static_cast<std::uint8_t>(envelope_unknown>>(8*i));
        storage[4]=static_cast<std::uint8_t>(summary.size());
        std::copy(summary.begin(),summary.end(),storage.begin()+6);
        return CharacterProfile::decode(storage);
    }
};
}

