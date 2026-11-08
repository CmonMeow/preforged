#pragma once
#include "gw_item_wire.h"
namespace fw::gw::equipment {
struct VisibilityChange {unsigned value,mask;};
inline VisibilityChange parse_visibility_2022(std::span<const std::uint8_t> bytes){
 if(bytes.size()!=10||(bytes[0]|unsigned(bytes[1])<<8)!=0x8055)throw std::invalid_argument("visibility frame");
 auto read=[&](unsigned at){unsigned result=0;for(unsigned i=0;i<4;++i)result|=unsigned(bytes[at+i])<<(8*i);return result;};
 const auto value=read(2),mask=read(6);
 if((mask!=3&&mask!=12&&mask!=48&&mask!=192)||(value&~mask))throw std::invalid_argument("visibility mask/value");
 return {value,mask};
}
inline items::Packet visibility_2022(unsigned value,unsigned mask=255){
 if(value>255||mask>255||!mask||(value&~mask))throw std::invalid_argument("visibility response");
 items::Packet packet;items::put(packet,238,2);items::put(packet,value,4);items::put(packet,mask,4);return packet;
}
}
