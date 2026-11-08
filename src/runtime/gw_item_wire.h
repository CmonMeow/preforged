#pragma once
#include "gw_item_modifiers.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace fw::gw::items {
using Packet=std::vector<std::uint8_t>;
// Build 37130 descriptor RVA 6278e0, callback VA 7c5300. Field labels
// follow the read-only GWCA StoC ItemGeneral reference where understood.
struct Definition {
    std::uint32_t id=0,file=0;
    std::uint8_t type=0,unknown1=0;
    std::uint16_t dye=0,materials=0;
    std::uint8_t unknown2=0;
    std::uint32_t interaction=0,price=0,model=0,quantity=1;
    std::u16string encoded_name;
    std::vector<std::uint32_t> modifiers;
};
inline void put(std::vector<std::uint8_t>& out,std::uint32_t value,unsigned bytes){
    for(unsigned i=0;i<bytes;++i)out.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
inline Packet inventory_2022(std::uint16_t inventory){
    if(!inventory)throw std::invalid_argument("inventory identity required");
    Packet p;put(p,323,2);put(p,inventory,2);put(p,0,1);return p;
}
// Pinned descriptor RVA60d82c [336,WORD,DWORD], callback7c4d70.
// Definitions must precede nonzero assignment: an unresolved native ID clears
// the held pointer. This encoder grants no ownership or pickup authority.
inline Packet held_bundle_2022(std::uint16_t inventory,std::uint32_t item){
    if(!inventory||item>65535)throw std::invalid_argument("invalid held bundle identity");
    Packet packet;put(packet,336,2);put(packet,inventory,2);put(packet,item,4);return packet;
}
inline Packet bag_2022(std::uint16_t inventory,std::uint8_t type,std::uint8_t index,
                      std::uint16_t bag,std::uint8_t capacity,std::uint32_t container=0){
    if(!inventory||!bag||!capacity||capacity>64||index>21||type<1||type>5)
        throw std::invalid_argument("invalid bag definition");
    Packet p;put(p,318,2);put(p,inventory,2);put(p,type,1);put(p,index,1);put(p,bag,2);put(p,capacity,1);put(p,container,4);return p;
}
inline Packet location_2022(std::uint16_t inventory,std::uint32_t item,std::uint16_t bag,std::uint8_t slot,bool moving=false){
    if(!inventory||!item||!bag||slot>=64)throw std::invalid_argument("invalid item location");
    Packet p;put(p,moving?330:317,2);put(p,inventory,2);put(p,item,4);put(p,bag,2);put(p,slot,1);return p;
}
inline Packet quantity_2022(std::uint32_t item,std::uint32_t count){
    if(!item||!count||count>65535)throw std::invalid_argument("invalid item quantity");
    Packet p;put(p,312,2);put(p,item,4);put(p,count,4);return p;
}
// 332 removes location, not the immutable item definition. 7c4bd0->7c92e0
// requires currently placed item; it is also usable before a safe slot swap.
inline Packet detach_2022(std::uint16_t inventory,std::uint32_t item){
    if(!inventory||!item)throw std::invalid_argument("invalid item removal");
    Packet p;put(p,332,2);put(p,inventory,2);put(p,item,4);return p;
}
// Native348 with replacement0 deletes the detached definition/table entry.
// Detach332 alone removes its placement but retains the item in the client.
inline Packet delete_2022(std::uint32_t item){
    if(!item)throw std::invalid_argument("invalid item deletion");
    Packet p;put(p,348,2);put(p,0,4);put(p,item,4);return p;
}
// Adapted from read-only framework/src/runtime/gw_item_wire.h. Pinned 37130
// callbacks 7c45e0/7c4ac0: initialize inventory, then equipped bag, then items.
inline std::vector<std::uint8_t> equipment_bag_2022(std::uint16_t inventory,std::uint16_t bag){
    if(!inventory||!bag)throw std::invalid_argument("invalid equipment bag identity");
    std::vector<std::uint8_t> out;put(out,318,2);put(out,inventory,2);
    put(out,2,1);put(out,21,1);put(out,bag,2);put(out,9,1);put(out,0,4);return out;
}
inline std::vector<std::uint8_t> equip_2022(std::uint16_t inventory,std::uint32_t item,std::uint16_t bag,std::uint8_t slot){
    if(!inventory||!item||!bag||slot>=9)throw std::invalid_argument("invalid equipment placement");
    std::vector<std::uint8_t> out;put(out,330,2);put(out,inventory,2);
    put(out,item,4);put(out,bag,2);put(out,slot,1);return out;
}
inline std::vector<std::uint8_t> definition_2022(const Definition& item){
    (void)modifiers::decode_wire(item.modifiers);
    // File zero asserts in the retail callback. Quantity is narrowed there.
    if(!item.id||!item.file||!item.quantity||item.quantity>65535||
       item.encoded_name.size()>63||item.encoded_name.find(u'\0')!=std::u16string::npos||item.modifiers.size()>64||
       (!item.modifiers.empty()&&item.modifiers.back()==0xc0000000u))
        throw std::invalid_argument("invalid 2022 item definition");
    std::vector<std::uint8_t> out;put(out,352,2);
    put(out,item.id,4);put(out,item.file,4);put(out,item.type,1);put(out,item.unknown1,1);
    put(out,item.dye,2);put(out,item.materials,2);put(out,item.unknown2,1);
    for(auto v:{item.interaction,item.price,item.model,item.quantity})put(out,v,4);
    put(out,static_cast<std::uint32_t>(item.encoded_name.size()),2);
    for(auto c:item.encoded_name)put(out,c,2);
    // The retail serializer uses a BYTE count for its bounded structure array.
    put(out,static_cast<std::uint32_t>(item.modifiers.size()),1);
    for(auto v:item.modifiers)put(out,v,4);
    return out;
}
// Emit only after the new item's definition exists. Callback VA 7c5230
// publishes creation UI item events and removes the old item when different.
inline std::vector<std::uint8_t> equipment_change_2022(std::uint32_t new_id,std::uint32_t old_id){
    if(!new_id&&!old_id)throw std::invalid_argument("empty equipment change");
    std::vector<std::uint8_t> out;put(out,348,2);put(out,new_id,4);put(out,old_id,4);return out;
}
}
