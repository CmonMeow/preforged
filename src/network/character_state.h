#pragma once
#include "skill_state.h"
#include "gw_character_name.h"
namespace gwserver {
template<class Value> struct CharacterStateResult {std::uint32_t status;std::string message;std::optional<Value> value;};
using CharacterNumberResult=CharacterStateResult<std::uint32_t>;
using CharacterNameResult=CharacterStateResult<std::string>;
struct SaveEquipmentVisibility {OwnedCharacter owner;unsigned expected,replacement;};
struct RenameCharacter {OwnedCharacter owner;std::string expected_name,replacement;};
struct DevelopmentUnlockResult {std::uint32_t status;std::string message;std::uint64_t skill_revision=0,attribute_revision=0;std::uint32_t learned=0;};
inline std::string storage_payload(const RenameCharacter& value){
    (void)fw::gw::character_name_to_utf16(value.expected_name);(void)fw::gw::character_name_to_utf16(value.replacement);
    return storage_payload(value.owner)+"\t"+value.expected_name+"\t"+value.replacement;
}
inline DevelopmentUnlockResult decode_development_unlock(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload};
    try{
        const auto fields=fw::split(result.payload);if(fields.size()!=3)throw std::invalid_argument("unlock framing");
        std::uint32_t learned=0;const auto parsed=std::from_chars(fields[2].data(),fields[2].data()+fields[2].size(),learned);
        if(fields[2].empty()||parsed.ec!=std::errc{}||parsed.ptr!=fields[2].data()+fields[2].size()||learned>3433)throw std::invalid_argument("unlock learned count");
        return {FW_OK,{},read_revision(fields[0]),read_revision(fields[1]),learned};
    }catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid development unlock response"};}
}
inline CharacterNumberResult decode_character_number(const fw::AccountResult& result,bool map){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    if(!map&&result.payload=="-")return {FW_OK,{},{}};
    std::uint32_t value=0;const auto parsed=std::from_chars(result.payload.data(),result.payload.data()+result.payload.size(),value);
    if(result.payload.empty()||parsed.ec!=std::errc{}||parsed.ptr!=result.payload.data()+result.payload.size()||(map&&(!value||value>65535)))
        return {FW_INTERNAL,"invalid stored character number",{}};
    return {FW_OK,{},value};
}
inline CharacterNameResult decode_character_name(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    try{(void)fw::gw::character_name_to_utf16(result.payload);return {FW_OK,{},result.payload};}
    catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid stored character name",{}};}
}
}
