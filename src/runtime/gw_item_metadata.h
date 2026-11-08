#pragma once
#include "gw_item_wire.h"
#include <string_view>

namespace fw::gw::items {
// Build37130 descriptorRVA60d898 [345,404,104], callback7c5100.
// 352 initializes Item+4f to11 (unknown);345 supplies the actual BYTE.
// Sentinel11 is explicit unknown metadata, never permission to wear armor.
inline Packet profession_2022(std::uint32_t item,unsigned profession){
    if(!item||profession>11)throw std::invalid_argument("invalid item profession metadata");
    Packet packet;put(packet,345,2);put(packet,item,4);put(packet,profession,1);return packet;
}
// Build37130 descriptorRVA60d724 [313,404,2017], callback7c4400.
// This copies the owner string into Item+18 separately from352. Publish only
// after that owned item definition exists. Empty-string clearing is deliberately
// not modeled: its non-null allocation is not proven equivalent to no owner.
inline Packet customization_2022(std::uint32_t item,std::u16string_view name){
    if(!item||name.empty()||name.size()>31||name.find(u'\0')!=name.npos)
        throw std::invalid_argument("invalid item customization metadata");
    Packet packet;put(packet,313,2);put(packet,item,4);put(packet,static_cast<std::uint32_t>(name.size()),2);
    for(auto unit:name)put(packet,unit,2);return packet;
}
}
