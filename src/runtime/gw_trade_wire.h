#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace fw::gw::trade {
enum class Action : std::uint16_t { Open=0,Cancel=1,AddItem=2,Offer=3,ChangeOffer=4,RemoveItem=5,RevokeOffer=6,Accept=7 };
struct Request { Action action;std::uint32_t value=0,quantity=0; };
inline std::optional<Request> parse_2022(std::span<const std::uint8_t> bytes){
    if(bytes.size()<2)return {};
    unsigned op=(bytes[0]|unsigned(bytes[1])<<8)&0x7fff;
    if(op>7&&op!=71)return {};
    const unsigned length=(op==0||op==3||op==71)?6:(op==2||op==5)?10:2;
    if(bytes.size()!=length)return {};
    auto word=[&](unsigned at){std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(bytes[at+i])<<(8*i);return v;};
    Request r{op==71?Action::Open:static_cast<Action>(op)};
    if(length>=6)r.value=word(2);if(length==10)r.quantity=word(6);
    return r;
}
inline std::vector<std::uint8_t> partner_packet(std::uint16_t opcode,std::uint32_t player_number){
    if(!player_number)throw std::invalid_argument("invalid trade partner");
    std::vector<std::uint8_t> packet{static_cast<std::uint8_t>(opcode),0};
    for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(player_number>>(8*i)));
    return packet;
}
// Initiator's full trade window: UI10000160 payload[0,partner].
inline auto requested_2022(std::uint32_t player_number){return partner_packet(5,player_number);}
// Recipient View/Decline prompt: UI10000160 payload[1,partner]. Client sends
// CtoS0 acknowledgement automatically; it is not the user's View action.
inline auto initiated_2022(std::uint32_t player_number){return partner_packet(0,player_number);}
// Native9 calls7dab00, clears both offer arrays and emits close UI1000015f.
inline std::vector<std::uint8_t> cancelled_2022(){return {9,0};}
inline std::vector<std::uint8_t> partner_offer_gold_2022(std::uint32_t gold){
    if(gold>100000)throw std::invalid_argument("trade gold cap");
    std::vector<std::uint8_t> packet{3,0};for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(gold>>(8*i)));return packet;
}
inline std::vector<std::uint8_t> partner_offer_item_2022(std::uint32_t item,unsigned quantity){
    if(!item||!quantity||quantity>250)throw std::invalid_argument("trade item quantity");
    std::vector<std::uint8_t> packet{4,0};for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(item>>(8*i)));packet.push_back(static_cast<std::uint8_t>(quantity));return packet;
}
inline std::vector<std::uint8_t> offered_count_2022(unsigned count){
    if(count>7)throw std::invalid_argument("trade item count");
    std::vector<std::uint8_t> packet{2,0,0};for(unsigned i=0;i<4;++i)packet.push_back(static_cast<std::uint8_t>(count>>(8*i)));return packet;
}
inline std::vector<std::uint8_t> partner_accepted_2022(){return {6,0};}
inline std::vector<std::uint8_t> revoke_2022(){return {7,0};}
inline std::vector<std::uint8_t> clear_partner_offer_2022(){return {8,0};}
// Native1 status0 resolves the pending operation successfully,1 rejects it,
// 2 terminates the session,3 emits UI10000163. No item transfer is implied.
inline std::vector<std::uint8_t> operation_result_2022(unsigned status){
    if(status>3)throw std::invalid_argument("invalid trade status");
    return {1,0,static_cast<std::uint8_t>(status)};
}
}
