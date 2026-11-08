#pragma once
#include "gw_character_name.h"
#include <span>
#include <cstdint>
namespace fw::gw::auth {
struct CharacterChange2022 {
    enum class State { incomplete,invalid,complete };
    std::uint32_t transaction=0;
    std::u16string name;
    std::size_t consumed=0;
};
// Retail descriptor a1f6a4: opcode10, DWORD transaction, UTF16[20].
// Serializer encodes the string as a WORD count followed by that many units.
inline CharacterChange2022::State parse_character_change_2022(
    std::span<const std::uint8_t> bytes,CharacterChange2022& out,std::uint16_t opcode=10){
    using State=CharacterChange2022::State;
    if(bytes.size()<2)return State::incomplete;
    if((opcode!=7&&opcode!=10)||(bytes[0]|(unsigned(bytes[1]&0x7f)<<8))!=opcode)return State::invalid;
    if(bytes.size()<8)return State::incomplete;
    const unsigned count=bytes[6]|(unsigned(bytes[7])<<8);
    if(!count||count>=20)return State::invalid;
    if(bytes.size()<8+2*count)return State::incomplete;
    CharacterChange2022 result;
    for(unsigned i=0;i<4;++i)result.transaction|=std::uint32_t(bytes[2+i])<<(8*i);
    if(!result.transaction)return State::invalid;
    for(unsigned i=0;i<count;++i)result.name+=char16_t(bytes[8+i*2]|(unsigned(bytes[9+i*2])<<8));
    try{(void)character_name_to_utf8(result.name,19);}catch(const std::invalid_argument&){return State::invalid;}
    result.consumed=8+2*count;out=std::move(result);return State::complete;
}
}
