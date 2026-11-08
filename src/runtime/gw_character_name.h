#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fw::gw {
inline std::u16string character_name_to_utf16(std::string_view text,std::size_t max_units=19) {
    std::u16string out;out.reserve(text.size());
    for(std::size_t i=0;i<text.size();) {
        const auto lead=static_cast<std::uint8_t>(text[i++]);std::uint32_t cp=0;unsigned extra=0;
        if(lead<0x80)cp=lead;
        else if((lead&0xe0)==0xc0){cp=lead&0x1f;extra=1;if(cp<2)throw std::invalid_argument("invalid GW character name UTF-8");}
        else if((lead&0xf0)==0xe0){cp=lead&0x0f;extra=2;}
        else if((lead&0xf8)==0xf0){cp=lead&0x07;extra=3;}
        else throw std::invalid_argument("invalid GW character name UTF-8");
        if(i+extra>text.size())throw std::invalid_argument("truncated GW character name UTF-8");
        for(unsigned n=0;n<extra;++n){auto next=static_cast<std::uint8_t>(text[i++]);if((next&0xc0)!=0x80)throw std::invalid_argument("invalid GW character name UTF-8");cp=(cp<<6)|(next&0x3f);}
        if((extra==2&&cp<0x800)||(extra==3&&cp<0x10000)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff)||cp<0x20||cp==0x7f)
            throw std::invalid_argument("invalid GW character name scalar");
        if(cp<=0xffff)out.push_back(static_cast<char16_t>(cp));
        else {cp-=0x10000;out.push_back(static_cast<char16_t>(0xd800+(cp>>10)));out.push_back(static_cast<char16_t>(0xdc00+(cp&0x3ff)));}
        if(out.size()>max_units)throw std::invalid_argument("GW character name exceeds UTF-16 bound");
    }
    if(out.empty())throw std::invalid_argument("empty GW character name");
    return out;
}
inline std::string character_name_to_utf8(std::u16string_view text,std::size_t max_units=19) {
    if(text.empty()||text.size()>max_units)throw std::invalid_argument("GW character name exceeds UTF-16 bound");
    std::string out;out.reserve(text.size()*3);
    auto append=[&](std::uint32_t cp){
        if(cp<0x20||cp==0x7f)throw std::invalid_argument("invalid GW character name scalar");
        if(cp<0x80)out.push_back(static_cast<char>(cp));
        else if(cp<0x800){out.push_back(static_cast<char>(0xc0|(cp>>6)));out.push_back(static_cast<char>(0x80|(cp&0x3f)));}
        else if(cp<0x10000){out.push_back(static_cast<char>(0xe0|(cp>>12)));out.push_back(static_cast<char>(0x80|((cp>>6)&0x3f)));out.push_back(static_cast<char>(0x80|(cp&0x3f)));}
        else {out.push_back(static_cast<char>(0xf0|(cp>>18)));out.push_back(static_cast<char>(0x80|((cp>>12)&0x3f)));out.push_back(static_cast<char>(0x80|((cp>>6)&0x3f)));out.push_back(static_cast<char>(0x80|(cp&0x3f)));}
    };
    for(std::size_t i=0;i<text.size();++i){std::uint32_t cp=text[i];if(cp>=0xd800&&cp<=0xdbff){if(++i>=text.size()||text[i]<0xdc00||text[i]>0xdfff)throw std::invalid_argument("invalid GW character name surrogate");cp=0x10000+((cp-0xd800)<<10)+(text[i]-0xdc00);}else if(cp>=0xdc00&&cp<=0xdfff)throw std::invalid_argument("invalid GW character name surrogate");append(cp);}
    return out;
}
// Count UTF-16 code units, not UTF-8 bytes, for the map-load name frame.
inline std::vector<std::uint8_t> character_name_packet(std::uint16_t opcode,std::string_view name,bool creation_preview=false) {
    const auto wide=creation_preview&&name.empty()?std::u16string{}:character_name_to_utf16(name);
    std::vector<std::uint8_t> packet;
    packet.reserve(4+wide.size()*2);
    const auto word=[&](std::uint16_t value){packet.push_back(static_cast<std::uint8_t>(value));packet.push_back(static_cast<std::uint8_t>(value>>8));};
    word(opcode);word(static_cast<std::uint16_t>(wide.size()));
    for(const auto unit:wide)word(unit);
    return packet;
}
}
