#pragma once
#include "progression_state.h"
#include "gw_guild_cape.h"
#include "gw_guild_trim.h"
namespace gwserver {
using CapeResult=CharacterStateResult<fw::gw::guild::cape::Design>;
struct PurchaseGuildCape {OwnedCharacter owner;std::uint64_t inventory_revision;unsigned quoted_price;fw::gw::guild::cape::Design design;};
struct PurchaseGuildTrim {OwnedCharacter owner;unsigned trim;fw::gw::guild::cape::Design expected_cape;};
struct CapePurchaseResult {std::uint32_t status;std::string message;std::uint64_t inventory_revision=0;std::uint32_t gold=0;};
inline std::string storage_payload(const PurchaseGuildCape& r){
    check_revision(r.inventory_revision);if(r.quoted_price!=fw::gw::guild::cape::price)throw std::invalid_argument("cape quote mismatch");
    return storage_payload(r.owner)+"\t"+std::to_string(r.inventory_revision)+"\t"+std::to_string(r.quoted_price)+"\t"+storage_hex(fw::gw::guild::cape::encode(r.design));
}
inline std::string storage_payload(const PurchaseGuildTrim& r){
    if(!fw::gw::guild::trim::find(r.trim))throw std::invalid_argument("unknown trim offer");
    return storage_payload(r.owner)+"\t"+std::to_string(r.trim)+"\t"+storage_hex(fw::gw::guild::cape::encode(r.expected_cape));
}
inline CapeResult decode_cape(const fw::AccountResult& r){
    if(r.status!=FW_OK)return {r.status,r.payload,{}};
    if(r.payload=="0")return {FW_OK,{},{}};
    try{return {FW_OK,{},fw::gw::guild::cape::decode(profile_bytes(r.payload,8))};}
    catch(const std::exception&){return {FW_INTERNAL,"invalid cape storage response",{}};}
}
inline CapePurchaseResult decode_cape_purchase(const fw::AccountResult& r,const PurchaseGuildCape& request){
    if(r.status!=FW_OK)return {r.status,r.payload};
    try{const auto f=fw::split(r.payload);if(f.size()!=2)throw std::invalid_argument("cape purchase framing");
        const auto revision=decode_revision({FW_OK,f[0]},request.inventory_revision);const auto gold=progression_number(f[1]);
        if(revision.status!=FW_OK||gold>100000)throw std::invalid_argument("cape purchase values");
        return {FW_OK,{},revision.revision,gold};
    }catch(const std::exception&){return {FW_INTERNAL,"invalid cape purchase response"};}
}
}
