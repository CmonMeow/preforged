#pragma once
#include "gw_item_wire.h"
#include "gw_outgoing_frame.h"
#include <span>

namespace fw::gw::inventory::gold {
using Batch=std::vector<items::Packet>;
// Local2021 Gold.html and original2022 Xunlai UI850521/850535 agree.
inline constexpr std::uint32_t carried_cap=100000,stored_cap=1000000;
struct Balances {
    std::uint32_t carried=0,stored=0;
    bool operator==(const Balances&)const=default;
};
inline bool valid(Balances balance){return balance.carried<=carried_cap&&balance.stored<=stored_cap;}
inline std::uint64_t total(Balances balance){return std::uint64_t(balance.carried)+balance.stored;}
struct Request {Balances balances;std::size_t consumed=0;};
// Builder7cab70, descriptor60d688 [122,404,404]. Both fields are requested
// FINAL balances, not amounts to add. Direction bit belongs to transport.
inline world::FrameState parse(std::span<const std::uint8_t> bytes,Request& request){
    if(bytes.size()<2)return world::FrameState::incomplete;
    if((bytes[0]|unsigned(bytes[1]&0x7f)<<8)!=122)return world::FrameState::unknown_opcode;
    const auto frame=world::outgoing_frame_2022(bytes);
    if(frame.state!=world::FrameState::complete)return frame.state;
    if(frame.size!=10)return world::FrameState::invalid;
    const auto read=[&](unsigned offset){std::uint32_t result=0;for(unsigned n=0;n<4;++n)result|=std::uint32_t(bytes[offset+n])<<(8*n);return result;};
    request={{read(2),read(6)},frame.size};return world::FrameState::complete;
}
// Original callbacks319/320 add the DWORD;334/335 negate it before adding.
// Inventory WORD must already exist. These are DELTAS, never absolute setters.
inline items::Packet delta_2022(unsigned opcode,std::uint16_t inventory,std::uint32_t amount){
    if(!inventory||!amount||(opcode!=319&&opcode!=320&&opcode!=334&&opcode!=335)||
       amount>((opcode==319||opcode==334)?carried_cap:stored_cap))
        throw std::invalid_argument("invalid native gold delta");
    items::Packet packet;items::put(packet,opcode,2);items::put(packet,inventory,2);items::put(packet,amount,4);return packet;
}
// known_client is the last successfully published balance in this fresh native
// inventory. Constructor7c85f2/7c85fc initializes0; after initial restoration,
// do not replay additions on each chest opening or denied/stale transaction.
inline Batch updates(Balances known_client,Balances desired,std::uint16_t inventory=1){
    if(!inventory||!valid(known_client)||!valid(desired))throw std::invalid_argument("invalid native gold balance");
    Batch result;
    // Debit before credit avoids transiently displaying newly created wealth.
    if(desired.carried<known_client.carried)result.push_back(delta_2022(334,inventory,known_client.carried-desired.carried));
    if(desired.stored<known_client.stored)result.push_back(delta_2022(335,inventory,known_client.stored-desired.stored));
    if(desired.carried>known_client.carried)result.push_back(delta_2022(319,inventory,desired.carried-known_client.carried));
    if(desired.stored>known_client.stored)result.push_back(delta_2022(320,inventory,desired.stored-known_client.stored));
    return result;
}
struct Policy {bool chest_open=false,can_deposit=false,can_withdraw=false;};
enum class Reason {accepted,mode,limit,conservation,direction};
inline const char* reason_text(Reason reason){switch(reason){case Reason::accepted:return "accepted";case Reason::mode:return "authorized_xunlai_required";
    case Reason::limit:return "gold_capacity";case Reason::conservation:return "gold_total_changed";case Reason::direction:return "gold_transfer_direction_unavailable";}return "invalid";}
struct Proposal {
    Reason status;Balances balances;Batch packets;std::uint32_t amount=0;bool deposit=false;
    bool accepted()const{return status==Reason::accepted;}
};
// Prepare only. Caller verifies current balances/revisions against both owned
// durable records and commits them together in one CAS transaction before any
// deltas are published. An absent imported balance defaults0, not fake wealth.
inline Proposal propose(Balances current,Balances requested,const Policy& policy,std::uint16_t inventory=1){
    if(!valid(current)||!inventory)throw std::invalid_argument("invalid authoritative gold balances");
    const auto reject=[&](Reason reason){return Proposal{reason,current,{},0,false};};
    if(!policy.chest_open)return reject(Reason::mode);
    if(!valid(requested))return reject(Reason::limit);
    if(total(current)!=total(requested))return reject(Reason::conservation);
    const bool deposit=requested.carried<current.carried;
    const auto amount=deposit?current.carried-requested.carried:requested.carried-current.carried;
    if(amount&&!(deposit?policy.can_deposit:policy.can_withdraw))return reject(Reason::direction);
    return {Reason::accepted,requested,updates(current,requested,inventory),amount,deposit};
}
}
