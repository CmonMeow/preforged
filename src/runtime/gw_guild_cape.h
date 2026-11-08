#pragma once
#include "gw_guild_wire.h"
#include "gw_outgoing_frame.h"
namespace fw::gw::guild::cape {
inline constexpr unsigned price=2000;
struct Design {std::uint8_t background=0,detail_color=0,emblem_color=0,shape=0,detail=0,trim=0;std::uint16_t emblem=0;};
inline void validate(const Design& d){if(d.shape>8||d.detail>31||d.emblem>173||d.trim>13)throw std::invalid_argument("invalid cape design");}
inline Packet encode(const Design& d){validate(d);return {d.background,d.detail_color,d.emblem_color,d.shape,d.detail,std::uint8_t(d.emblem),std::uint8_t(d.emblem>>8),d.trim};}
inline Design decode(std::span<const std::uint8_t> b){if(b.size()!=8)throw std::invalid_argument("invalid cape design size");Design d{b[0],b[1],b[2],b[3],b[4],b[7],std::uint16_t(b[5]|unsigned(b[6])<<8)};validate(d);return d;}
inline void apply(General& g,const Design& d){validate(d);g.background=d.background;g.detail_color=d.detail_color;g.emblem_color=d.emblem_color;g.shape=d.shape;g.detail=d.detail;g.emblem=d.emblem;g.trim=d.trim;g.features=std::max<std::uint8_t>(g.features,20);}
// Native194 ->795EB0 ->UI100000B3;579C43 asserts vendor type9.
// Native195 sets window owner before194. Cost is a server-supplied DWORD.
inline Batch open(std::uint32_t actor){if(!actor||actor>65535)throw std::invalid_argument("invalid cape vendor actor");Packet owner;items::put(owner,195,2);items::put(owner,actor,4);Packet window;items::put(window,194,2);items::put(window,9,1);items::put(window,price,4);return {owner,window};}
struct Purchase {unsigned quoted_price=0;Design design;};
// Native876400 receives cost + seven DWORD cape fields. Build37130's
// outgoing descriptor54 serializes costDWORD, fiveBYTEs, emblemWORD, trimBYTE.
inline Purchase parse(std::span<const std::uint8_t> b){if(b.size()!=14||((b[0]|unsigned(b[1])<<8)&0x7fff)!=54)throw std::invalid_argument("invalid cape purchase frame");unsigned cost=0;for(unsigned n=0;n<4;++n)cost|=unsigned(b[2+n])<<(8*n);return {cost,decode(b.subspan(6))};}
}
