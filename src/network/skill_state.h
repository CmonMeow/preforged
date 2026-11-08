#pragma once
#include "character_requests.h"
#include "accounts.h"
#include "common.h"
#include "gw_character_skills.h"
#include "gw_attributes.h"
#include <charconv>
namespace gwserver {
struct OwnedCharacter {Identity account,character;};
struct SaveSkillbar {OwnedCharacter owner;std::uint64_t expected_revision;fw::gw::skills::Slots bar;};
struct SaveAttributes {OwnedCharacter owner;std::uint64_t expected_revision;fw::gw::attributes::Profile profile;};
struct SaveSecondaryProfession {
    OwnedCharacter owner;std::uint64_t skill_revision,attribute_revision;
    fw::gw::skills::Profile skills;fw::gw::attributes::Profile attributes;
};
template<class Profile> struct VersionedProfile {std::uint64_t revision;Profile profile;};
template<class Profile> struct ProfileResult {std::uint32_t status;std::string message;std::optional<VersionedProfile<Profile>> record;};
using SkillsResult=ProfileResult<fw::gw::skills::Profile>;
using AttributesResult=ProfileResult<fw::gw::attributes::Profile>;
struct RevisionResult {std::uint32_t status;std::string message;std::uint64_t revision=0;};
struct ProfessionSaveResult {std::uint32_t status;std::string message;std::uint64_t skill_revision=0,attribute_revision=0;};
inline std::string storage_payload(const OwnedCharacter& value){return storage_hex(value.account.bytes)+"\t"+storage_hex(value.character.bytes);}
inline void check_revision(std::uint64_t value){if(value>=INT64_MAX)throw std::invalid_argument("revision outside storage range");}
inline std::string storage_payload(const SaveSkillbar& value){
    check_revision(value.expected_revision);std::vector<std::uint8_t> bytes;
    for(const auto& slot:value.bar){fw::gw::skills::dword(bytes,slot.skill);fw::gw::skills::dword(bytes,slot.copy);}
    return storage_payload(value.owner)+"\t"+std::to_string(value.expected_revision)+"\t"+storage_hex(bytes);
}
inline std::string storage_payload(const SaveAttributes& value){
    check_revision(value.expected_revision);return storage_payload(value.owner)+"\t"+std::to_string(value.expected_revision)+"\t"+storage_hex(value.profile.encode());
}
inline std::string storage_payload(const SaveSecondaryProfession& value){
    check_revision(value.skill_revision);check_revision(value.attribute_revision);
    return storage_payload(value.owner)+"\t"+std::to_string(value.skill_revision)+"\t"+std::to_string(value.attribute_revision)+"\t"+storage_hex(fw::gw::skills::encode_profile(value.skills))+"\t"+storage_hex(value.attributes.encode());
}
inline std::uint64_t read_revision(const std::string& text){
    std::uint64_t value=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    if(text.empty()||parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!value||value>INT64_MAX)throw std::invalid_argument("invalid stored revision");return value;
}
inline std::vector<std::uint8_t> profile_bytes(const std::string& text,std::size_t max){
    if(text.empty()||text.size()%2||text.size()>max*2)throw std::invalid_argument("invalid stored profile length");
    std::vector<std::uint8_t> bytes;bytes.reserve(text.size()/2);
    const auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::invalid_argument("invalid stored profile hex");};
    for(std::size_t i=0;i<text.size();i+=2)bytes.push_back(static_cast<std::uint8_t>(digit(text[i])*16+digit(text[i+1])));return bytes;
}
inline SkillsResult decode_skills(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    if(result.payload=="0")return {FW_OK,{},{}};
    try{const auto fields=fw::split(result.payload);if(fields.size()!=2)throw std::invalid_argument("profile framing");
        return {FW_OK,{},VersionedProfile<fw::gw::skills::Profile>{read_revision(fields[0]),fw::gw::skills::decode_profile(profile_bytes(fields[1],13804))}};
    }catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid skill profile storage response",{}};}
}
inline AttributesResult decode_attributes(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    if(result.payload=="0")return {FW_OK,{},{}};
    try{const auto fields=fw::split(result.payload);if(fields.size()!=2)throw std::invalid_argument("profile framing");
        return {FW_OK,{},VersionedProfile<fw::gw::attributes::Profile>{read_revision(fields[0]),fw::gw::attributes::Profile::decode(profile_bytes(fields[1],54))}};
    }catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid attribute profile storage response",{}};}
}
inline RevisionResult decode_revision(const fw::AccountResult& result,std::uint64_t expected){
    if(result.status!=FW_OK)return {result.status,result.payload};
    try{check_revision(expected);const auto revision=read_revision(result.payload);if(revision!=expected+1)throw std::invalid_argument("revision mismatch");return {FW_OK,{},revision};
    }catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid save revision storage response"};}
}
inline ProfessionSaveResult decode_profession_save(const fw::AccountResult& result,const SaveSecondaryProfession& request){
    if(result.status!=FW_OK)return {result.status,result.payload};
    const auto fields=fw::split(result.payload);if(fields.size()!=2)return {FW_INTERNAL,"invalid profession save storage response"};
    const auto skills=decode_revision({FW_OK,fields[0]},request.skill_revision),attributes=decode_revision({FW_OK,fields[1]},request.attribute_revision);
    if(skills.status!=FW_OK||attributes.status!=FW_OK)return {FW_INTERNAL,"invalid profession save storage response"};
    return {FW_OK,{},skills.revision,attributes.revision};
}
}
