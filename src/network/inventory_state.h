#pragma once
#include "skill_state.h"
#include "gw_inventory.h"
#include "gw_storage.h"
namespace gwserver {
using InventoryResult=ProfileResult<fw::gw::inventory::Profile>;
struct SaveInventory {OwnedCharacter owner;std::uint64_t expected_revision;fw::gw::inventory::Profile profile;};
struct TransferStorage {
    OwnedCharacter owner;std::uint64_t inventory_revision,storage_revision;
    fw::gw::inventory::Profile character,bank;
};
struct StorageTransferResult {std::uint32_t status;std::string message;std::uint64_t inventory_revision=0,storage_revision=0;};
inline std::string storage_payload(const SaveInventory& value){
    check_revision(value.expected_revision);fw::gw::inventory::validate_character(value.profile);
    return storage_payload(value.owner)+"\t"+std::to_string(value.expected_revision)+"\t"+storage_hex(fw::gw::inventory::encode(value.profile));
}
inline std::string storage_payload(const TransferStorage& value){
    check_revision(value.inventory_revision);check_revision(value.storage_revision);
    fw::gw::inventory::validate_character(value.character);fw::gw::storage::validate(value.bank);
    return storage_payload(value.owner)+"\t"+std::to_string(value.inventory_revision)+"\t"+std::to_string(value.storage_revision)+
        "\t"+storage_hex(fw::gw::inventory::encode(value.character))+"\t"+storage_hex(fw::gw::inventory::encode(value.bank));
}
inline StorageTransferResult decode_storage_transfer(const fw::AccountResult& result,const TransferStorage& value){
    if(result.status!=FW_OK)return {result.status,result.payload};
    const auto fields=fw::split(result.payload);
    if(fields.size()!=2)return {FW_INTERNAL,"invalid storage transfer response"};
    const auto character=decode_revision({FW_OK,fields[0]},value.inventory_revision),bank=decode_revision({FW_OK,fields[1]},value.storage_revision);
    if(character.status!=FW_OK||bank.status!=FW_OK)return {FW_INTERNAL,"invalid storage transfer revisions"};
    return {FW_OK,{},character.revision,bank.revision};
}
inline InventoryResult decode_inventory(const fw::AccountResult& result,bool bank){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    if(result.payload=="0")return {FW_OK,{},{}};
    try{
        const auto fields=fw::split(result.payload);
        if(fields.size()!=2)throw std::invalid_argument("inventory framing");
        auto profile=fw::gw::inventory::decode(profile_bytes(fields[1],fw::gw::inventory::maximum_blob));
        if(bank)fw::gw::storage::validate(profile);else fw::gw::inventory::validate_character(profile);
        return {FW_OK,{},VersionedProfile<fw::gw::inventory::Profile>{read_revision(fields[0]),std::move(profile)}};
    }catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid inventory storage response",{}};}
}
}
