#pragma once
#include "inventory_state.h"
#include "gw_trade_exchange.h"
namespace gwserver {
struct TradeParticipant {OwnedCharacter owner;std::uint64_t expected_revision;fw::gw::trade::exchange::Offer offer;};
struct ExchangeTrade {TradeParticipant left,right;};
struct TradeResult {
    std::uint32_t status;std::string message;
    std::optional<VersionedProfile<fw::gw::inventory::Profile>> left,right;
};
inline std::string storage_payload(const TradeParticipant& value){
    check_revision(value.expected_revision);
    if(value.offer.gold>100000||value.offer.items.size()>7)throw std::invalid_argument("trade offer bound");
    std::set<std::uint32_t> seen;std::string items;
    for(auto item:value.offer.items){
        if(!item.item||!item.quantity||!seen.insert(item.item).second)throw std::invalid_argument("duplicate or empty trade offer");
        if(!items.empty())items+=',';
        items+=std::to_string(item.item)+":"+std::to_string(item.quantity);
    }
    return storage_payload(value.owner)+"\t"+std::to_string(value.expected_revision)+"\t"+std::to_string(value.offer.gold)+"\t"+items;
}
inline std::string storage_payload(const ExchangeTrade& value){
    if(value.left.owner.account.bytes==value.right.owner.account.bytes||value.left.owner.character.bytes==value.right.owner.character.bytes)
        throw std::invalid_argument("trade requires distinct accounts and characters");
    return storage_payload(value.left)+"\t"+storage_payload(value.right);
}
inline TradeResult decode_trade(const fw::AccountResult& result,const ExchangeTrade& value){
    if(result.status!=FW_OK)return {result.status,result.payload,{},{}};
    const auto fields=fw::split(result.payload);
    if(fields.size()!=4)return {FW_INTERNAL,"invalid trade response framing",{},{}};
    const auto left_revision=decode_revision({FW_OK,fields[0]},value.left.expected_revision);
    const auto right_revision=decode_revision({FW_OK,fields[1]},value.right.expected_revision);
    const auto left=decode_inventory({FW_OK,fields[0]+"\t"+fields[2]},false);
    const auto right=decode_inventory({FW_OK,fields[1]+"\t"+fields[3]},false);
    if(left_revision.status!=FW_OK||right_revision.status!=FW_OK||left.status!=FW_OK||right.status!=FW_OK||!left.record||!right.record)
        return {FW_INTERNAL,"invalid trade revisions or profiles",{},{}};
    return {FW_OK,{},left.record,right.record};
}
}
