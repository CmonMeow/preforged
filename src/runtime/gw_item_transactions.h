#pragma once
#include "gw_item_wire.h"
#include <array>
#include <optional>
#include <span>

namespace fw::gw::item_transactions {
enum class Kind {dye,identify,salvage_start,salvage_cancel,salvage_ack,
                 salvage_materials,salvage_component,upgrade_select,
                 upgrade_query,upgrade_cancel,upgrade_confirm};
struct Request {
    Kind kind{};
    std::uint16_t inventory=0;
    std::uint32_t source=0,target=0;
    std::array<std::uint32_t,4> dyes{};
    std::uint8_t dye_count=0,selection=0;
};
// Exact build37130 descriptors/send wrappers are independently executed in
// tools/protocol/item_transactions_2022.py. Syntax checks do not establish
// item ownership, charges, recipe eligibility, preview identity, or outcomes.
inline std::optional<Request> request_2022(const items::Packet& packet){
    if(packet.size()<2)return {};
    const auto read=[&](std::size_t at,unsigned width){std::uint32_t result=0;
        for(unsigned byte=0;byte<width;++byte)result|=std::uint32_t(packet[at+byte])<<(8*byte);return result;};
    const auto opcode=unsigned(packet[0])|(unsigned(packet[1]&0x7fu)<<8);
    Request request;
    switch(opcode){
    case 104:{ // [104,404,42b,6]: count WORD followed by count DWORD IDs.
        if(packet.size()<8)return {};
        const auto count=read(6,2);
        if(count<1||count>4||packet.size()!=8+count*4)return {};
        request.kind=Kind::dye;request.target=read(2,4);if(!request.target)return {};
        request.dye_count=static_cast<std::uint8_t>(count);
        for(unsigned index=0;index<count;++index){request.dyes[index]=read(8+index*4,4);if(!request.dyes[index])return {};}
        return request;
    }
    case 106:
        if(packet.size()!=10)return {};
        request.kind=Kind::identify;request.source=read(2,4);request.target=read(6,4);
        if(!request.source||!request.target)return {};return request;
    case 117: // 7c5f20 asserts inventoryTable.Get(targetInventoryId).
        if(packet.size()!=12)return {};
        request.kind=Kind::salvage_start;request.inventory=static_cast<std::uint16_t>(read(2,2));
        request.source=read(4,4);request.target=read(8,4);
        if(!request.inventory||!request.source||!request.target)return {};return request;
    case 121:
        if(packet.size()!=3||packet[2]>=3)return {};
        request.kind=Kind::salvage_component;request.selection=packet[2];return request;
    case 125:
        if(packet.size()!=7||packet[2]>=4)return {};
        request.kind=Kind::upgrade_select;request.selection=packet[2];request.source=read(3,4);
        if(!request.source)return {};return request;
    case 126:
        if(packet.size()!=8)return {};
        request.kind=Kind::upgrade_query;request.inventory=static_cast<std::uint16_t>(read(2,2));request.target=read(4,4);
        if(!request.inventory||!request.target)return {};return request;
    case 118:request.kind=Kind::salvage_cancel;break;
    case 119:request.kind=Kind::salvage_ack;break;
    case 120:request.kind=Kind::salvage_materials;break;
    case 127:request.kind=Kind::upgrade_cancel;break;
    case 128:request.kind=Kind::upgrade_confirm;break;
    default:return {};
    }
    if(packet.size()!=2)return {};return request;
}
// Native callbacks copy all THREE native option slots to the UI. This bounded
// array serializer/deserializer pads native fields; retain option positions,
// including zero gaps. Nonzero IDs require real definitions before publication.
inline items::Packet choices_2022(unsigned opcode,std::uint16_t inventory,
                                 std::uint32_t item,std::span<const std::uint32_t> choices){
    if((opcode!=354&&opcode!=361)||!inventory||!item||choices.size()>3)
        throw std::invalid_argument("invalid item transaction choices");
    items::Packet packet;items::put(packet,opcode,2);items::put(packet,inventory,2);items::put(packet,item,4);
    items::put(packet,static_cast<std::uint32_t>(choices.size()),2);for(auto id:choices)items::put(packet,id,4);return packet;
}
inline items::Packet salvage_choices_2022(std::uint16_t inventory,std::uint32_t target,
                                         std::span<const std::uint32_t> choices){
    return choices_2022(354,inventory,target,choices);
}
// The second field becomes the confirmation preview item in gmview. It must
// be a reviewed preview/target definition, never a made-up replacement item.
inline items::Packet upgrade_choices_2022(std::uint16_t inventory,std::uint32_t preview,
                                         std::span<const std::uint32_t> choices){
    return choices_2022(361,inventory,preview,choices);
}
inline items::Packet terminal_2022(unsigned opcode){
    if(opcode!=355&&opcode!=356&&opcode!=357&&opcode!=362&&opcode!=363)
        throw std::invalid_argument("invalid item transaction terminal opcode");
    items::Packet packet;items::put(packet,opcode,2);return packet;
}
inline items::Packet salvage_cancel_2022(){return terminal_2022(355);}
inline items::Packet salvage_done_2022(){return terminal_2022(356);}
// Native success357 and kept358 cause an automatic CMSG119 acknowledgment.
inline items::Packet salvage_success_2022(){return terminal_2022(357);}
inline items::Packet salvage_kept_2022(std::uint8_t value){
    items::Packet packet;items::put(packet,358,2);items::put(packet,value,1);return packet;
}
// Exact fields proven; their semantic meaning is deliberately not inferred
// from old GWCA labels. gmview opens the saved confirmation on this event.
inline items::Packet upgrade_ready_2022(std::uint8_t value,std::uint32_t item){
    items::Packet packet;items::put(packet,360,2);items::put(packet,value,1);items::put(packet,item,4);return packet;
}
inline items::Packet upgrade_cancel_2022(){return terminal_2022(362);}
inline items::Packet upgrade_done_2022(){return terminal_2022(363);}
}
