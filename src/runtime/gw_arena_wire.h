#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fw::gw::arena {
using Packet=std::vector<std::uint8_t>;
using Batch=std::vector<Packet>;
struct Request {
    std::uint16_t opcode=0;
    std::uint8_t selector=0;
    bool enter()const{return opcode==163;}
    friend bool operator==(const Request&,const Request&)=default;
};
// Original37130 outgoing descriptors60e3e0/60e3f8:163 BYTE selector;161 no
// fields. This selector is neither a mapID nor a partyID. Preserve all BYTE
// values here; admission belongs to the authenticated, authoritative caller.
inline std::optional<Request> parse_request(std::span<const std::uint8_t> bytes){
    if(bytes.size()<2)return {};
    const auto opcode=std::uint16_t((bytes[0]|unsigned(bytes[1])<<8)&0x7fff);
    if(opcode==163&&bytes.size()==3)return Request{163,bytes[2]};
    if(opcode==161&&bytes.size()==2)return Request{161,0};
    return {};
}
inline void put(Packet& out,std::uint32_t n,unsigned width){
    for(unsigned i=0;i<width;++i)out.push_back(std::uint8_t(n>>(8*i)));
}
inline void label(Packet& out,std::u16string_view text){
    if(text.size()>121||text.find(u'\0')!=std::u16string_view::npos)
        throw std::invalid_argument("invalid arena clock label");
    put(out,std::uint32_t(text.size()),2);
    for(auto c:text)put(out,c,2);
}
// Original383→7cdf90 starts a clock;7cded0 computes duration-elapsed, clamped
// at0, from the client's monotonic clock. Supply elapsed from ONE server window
// when a second participant joins. The native label has122UTF16 cells including
// its terminator. Bits0/1 are UI booleans. Bit2 is the running bit which the
// original constructor ORs into the stored flags; captured quest clocks use4.
inline Packet countdown_2022(std::u16string_view text,std::uint32_t elapsed_ms,
        std::uint32_t duration_ms,std::uint8_t flags=0){
    if(!duration_ms||elapsed_ms>duration_ms||flags>7)
        throw std::invalid_argument("invalid arena countdown");
    Packet p;put(p,383,2);label(p,text);put(p,flags,1);
    put(p,elapsed_ms,4);put(p,duration_ms,4);return p;
}
// The native timer renders coded game text, not a plain UTF16 string. Wrap
// developer labels in the client's literal-string encoding before publication.
inline Packet countdown_plain_2022(std::u16string_view text,std::uint32_t elapsed_ms,std::uint32_t duration_ms){
    if(text.size()>118)throw std::invalid_argument("arena literal label too long");
    for(auto unit:text)if(unit<32)throw std::invalid_argument("arena literal label control character");
    std::u16string encoded=u"\x108\x107";encoded+=text;encoded+=u'\x1';
    return countdown_2022(encoded,elapsed_ms,duration_ms);
}
// Original384→7ce060 freezes an EXISTING clock at this elapsed value; it does
// not create/start a countdown. Exposed for exact protocol coverage only.
inline Packet freeze_clock_2022(std::uint32_t elapsed_ms){
    Packet p;put(p,384,2);put(p,elapsed_ms,4);return p;
}
// Original382→7cdf20 corrects the elapsed value of an existing clock. It keeps
// the running flag; the native callback only accepts a later clock origin.
inline Packet correct_clock_2022(std::uint32_t elapsed_ms){
    Packet p;put(p,382,2);put(p,elapsed_ms,4);return p;
}
// Original381→7cde80 frees/removes the clock and emits UI10000091.
inline Packet clear_clock_2022(){Packet p;put(p,381,2);return p;}
// Original472→7d8220: nonzero marker/waiting clears local request bits2/4 and
// locks party membership; zero/zero releases this reference and restores ready
// request state. Marker2 is private-server policy, not a recovered retail team
// identity. Repeated pending(true) with the same marker is reference-balanced.
inline Packet pending_2022(bool value){
    Packet p;put(p,472,2);put(p,value?2:0,1);put(p,value?1:0,1);label(p,u"");return p;
}
inline Batch cancel_2022(){return {clear_clock_2022(),pending_2022(false)};}
}
