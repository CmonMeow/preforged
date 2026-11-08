#pragma once
#include "gw_item_wire.h"
#include <array>
#include <functional>
#include <optional>

namespace fw::gw::weapon_sets {
struct Set {std::uint32_t weapon=0,offhand=0;bool operator==(const Set&)const=default;};
struct State {std::array<Set,4> sets{};std::uint8_t active=0;bool operator==(const State&)const=default;};
// Lookup must return only an authoritative, owned carrying/equipped item.
// A preview appearance or unknown model is never an owned weapon definition.
struct Item {unsigned type=0;std::uint32_t quantity=1,interaction=0;};
using Lookup=std::function<std::optional<Item>(std::uint32_t)>;
enum class Hand {weapon,offhand};
inline bool valid_type(unsigned type,Hand hand){
    if(hand==Hand::offhand)return type==12||type==24;
    switch(type){case 2:case 5:case 15:case 22:case 26:case 27:case 32:case 35:case 36:return true;default:return false;}
}
inline void validate(const State& state,const Lookup& owned){
    if(state.active>=4)throw std::invalid_argument("invalid active weapon set");
    for(const auto& set:state.sets){
        if(set.weapon&&set.weapon==set.offhand)throw std::invalid_argument("same item in both weapon-set hands");
        for(auto hand:{Hand::weapon,Hand::offhand}){
            const auto id=hand==Hand::weapon?set.weapon:set.offhand;
            if(!id)continue;
            const auto item=owned?owned(id):std::nullopt;
            if(!item||item->quantity!=1||!valid_type(item->type,hand))
                throw std::invalid_argument("weapon set references unowned or ineligible item");
        }
    }
}
// Native7c8f10 preserves the stored offhand but hides it when mainhand's
// reviewed interaction bit200000 marks two-handed equipment. Do not discard
// that possession/reference merely because it is currently ineffective.
inline Set effective(const State& state,const Lookup& owned){
    validate(state,owned);auto result=state.sets[state.active];
    if(result.weapon&&(owned(result.weapon)->interaction&0x200000u))result.offhand=0;
    return result;
}
// Build37130 registryRVA60d7c0 [326,204,104,404,404], callback7c4960.
inline items::Packet set_2022(std::uint16_t inventory,unsigned index,Set set){
    if(!inventory||index>=4||(set.weapon&&set.weapon==set.offhand))
        throw std::invalid_argument("invalid weapon-set publication");
    items::Packet packet;items::put(packet,326,2);items::put(packet,inventory,2);
    items::put(packet,index,1);items::put(packet,set.weapon,4);items::put(packet,set.offhand,4);return packet;
}
// Build37130 registryRVA60d7cc [327,204,104], callback7c49f0.
inline items::Packet active_2022(std::uint16_t inventory,unsigned active){
    if(!inventory||active>=4)throw std::invalid_argument("invalid active weapon-set publication");
    items::Packet packet;items::put(packet,327,2);items::put(packet,inventory,2);items::put(packet,active,1);return packet;
}
// Native315 action3 ->7c44e0 ->UIe8. Actual gmweaponbar521500 ignores both
// payload fields, clears pending and redraws its current set. Value0 is a
// reviewed neutral cancellation here, not an invented game error mapping.
inline items::Packet cancel_2022(){
    items::Packet packet;items::put(packet,315,2);items::put(packet,3,1);items::put(packet,0,4);return packet;
}
// Publish AFTER all definition/metadata and bag-placement/removal updates:
// those native callbacks themselves mutate membership/current-set references.
inline std::vector<items::Packet> restore_2022(std::uint16_t inventory,const State& state,const Lookup& owned){
    validate(state,owned);std::vector<items::Packet> packets;
    for(unsigned index=0;index<4;++index)packets.push_back(set_2022(inventory,index,state.sets[index]));
    packets.push_back(active_2022(inventory,state.active));return packets;
}
inline State switched(State state,unsigned active){
    if(state.active>=4||active>=4)throw std::invalid_argument("invalid weapon-set switch");
    state.active=static_cast<std::uint8_t>(active);return state;
}
// Ordinary set editing is equip46 into the ACTIVE set; no invented edit packet.
inline State equipped(State state,Hand hand,std::uint32_t item){
    if(state.active>=4||!item)throw std::invalid_argument("invalid weapon-set equip");
    auto& set=state.sets[state.active];(hand==Hand::weapon?set.weapon:set.offhand)=item;return state;
}
// A verified active-hand item moved out of equipment clears only that hand;
// other stored references survive. Physical equipment and active references
// are independent, so never clear an unrelated active item by slot alone.
inline State unequipped(State state,Hand hand,std::uint32_t item){
    if(state.active>=4||!item)throw std::invalid_argument("invalid weapon-set unequip");
    auto& set=state.sets[state.active];auto& current=hand==Hand::weapon?set.weapon:set.offhand;
    if(current==item)current=0;return state;
}
inline State erased(State state,std::uint32_t item){
    if(state.active>=4||!item)throw std::invalid_argument("invalid weapon-set erasure");
    for(auto& set:state.sets){if(set.weapon==item)set.weapon=0;if(set.offhand==item)set.offhand=0;}return state;
}
// Exact CMSG48 descriptorRVA60b748 [48,104], sender876250 from
// gmweaponbar00520510/F1-F4. Direction bit is transport framing, not a field.
inline std::optional<unsigned> switch_request_2022(const items::Packet& packet){
    if(packet.size()!=3||(unsigned(packet[0])|(unsigned(packet[1]&0x7fu)<<8))!=48||packet[2]>=4)return {};
    return packet[2];
}
}
