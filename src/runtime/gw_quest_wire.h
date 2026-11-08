#pragma once
#include "gw_item_wire.h"
#include <bit>
#include <cmath>
#include <string_view>
namespace fw::gw::quests {
// Build37130 QuestAdd: original startup462c20 fills descriptor RVA60ba00.
// Encoded strings are native string-code units, not plain localized titles.
struct Entry {
    std::uint32_t id=0,state=0;
    float x=0,y=0;
    std::uint16_t plane=0,map_to=0,map_from=0;
    std::u16string location,name,npc;
};
// Native790144 compares both coordinates with +infinity at89ac68 and
// requests the marker when both match. NaN is not this sentinel.
inline bool valid_marker(float x,float y){
    return (std::isfinite(x)&&std::isfinite(y))||
        (std::bit_cast<std::uint32_t>(x)==0x7f800000u&&std::bit_cast<std::uint32_t>(y)==0x7f800000u);
}
inline void encoded_text(items::Packet& out,std::u16string_view text,unsigned maximum){
    // Reserve termination in the caller's decoded native buffer (8 or128
    // units); never accept an unterminated full array or embedded NUL.
    if(text.size()>maximum||text.find(u'\0')!=text.npos)throw std::invalid_argument("quest encoded text exceeds native terminated buffer");
    items::put(out,static_cast<std::uint32_t>(text.size()),2);
    for(auto unit:text)items::put(out,unit,2);
}
inline void encoded_name(items::Packet& out,std::u16string_view text){encoded_text(out,text,7);}
inline items::Packet add_2022(const Entry& quest){
    if(!quest.id||!valid_marker(quest.x,quest.y))throw std::invalid_argument("invalid quest identity or marker");
    items::Packet out;items::put(out,72,2);items::put(out,quest.id,4);
    items::put(out,std::bit_cast<std::uint32_t>(quest.x),4);items::put(out,std::bit_cast<std::uint32_t>(quest.y),4);
    items::put(out,quest.plane,2);items::put(out,quest.map_to,2);items::put(out,quest.state,4);
    encoded_name(out,quest.location);encoded_name(out,quest.name);encoded_name(out,quest.npc);
    items::put(out,quest.map_from,2);return out;
}
// Callback873ab0 writes description at Quest+2c and objectives at Quest+30.
// Quest must already exist: the original callback asserts on missing identity.
inline items::Packet details_2022(std::uint32_t id,std::u16string_view description,std::u16string_view objectives){
    if(!id)throw std::invalid_argument("quest identity required");
    items::Packet out;items::put(out,75,2);items::put(out,id,4);
    encoded_text(out,description,127);encoded_text(out,objectives,127);return out;
}
inline items::Packet remove_2022(std::uint32_t id){
    if(!id)throw std::invalid_argument("quest identity required");
    items::Packet out;items::put(out,81,2);items::put(out,id,4);return out;
}
// Retail873c10 ->791af0 updates Quest+30 only when details are cached and
// emits UI1000014a (QuestUpdated). General79 emits QuestAdded instead.
inline items::Packet objectives_2022(std::uint32_t id,std::u16string_view objectives){
    if(!id)throw std::invalid_argument("quest identity required");
    items::Packet out;items::put(out,83,2);items::put(out,id,4);
    encoded_text(out,objectives,127);return out;
}
// Native873b20 ->7915d0 updates an existing quest's state and short text.
// Details sets cache bit1 separately; sending that bit here trips a native assert.
inline items::Packet update_2022(const Entry& quest){
    if(!quest.id||(quest.state&1))throw std::invalid_argument("invalid quest update identity or loaded-details flag");
    items::Packet out;items::put(out,79,2);items::put(out,quest.id,4);items::put(out,quest.state,4);
    encoded_name(out,quest.location);encoded_name(out,quest.name);encoded_name(out,quest.npc);
    items::put(out,quest.map_from,2);return out;
}
inline items::Packet marker_2022(const Entry& quest){
    if(!quest.id||!valid_marker(quest.x,quest.y))throw std::invalid_argument("invalid quest marker");
    items::Packet out;items::put(out,76,2);items::put(out,quest.id,4);
    items::put(out,std::bit_cast<std::uint32_t>(quest.x),4);items::put(out,std::bit_cast<std::uint32_t>(quest.y),4);
    items::put(out,quest.plane,2);items::put(out,quest.map_to,2);return out;
}
}
