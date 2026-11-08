#pragma once
#include "character_requests.h"
#include "accounts.h"
#include "framework/module.h"
#include <charconv>
#include <set>

namespace gwserver {
struct RosterEntry {
    Identity character;
    std::string name;
    std::uint16_t map;
    std::optional<fw::gw::CharacterProfile> profile;
};
struct RosterResult {
    std::uint32_t status;
    std::string message;
    unsigned total=0;
    std::vector<RosterEntry> entries;
};
struct CreatedCharacter {Identity character;std::string name;std::uint16_t map;};
struct CreationResult {
    std::uint32_t status;
    std::string message;
    std::optional<CreatedCharacter> created;
};
struct DeletionResult {std::uint32_t status;std::string message;};
struct MapTransitionResult {std::uint32_t status;std::string message;};
// Decode the internal storage envelope once. Protocol handlers consume records,
// never its delimiter layout. A malformed page must not expose partial records.
inline RosterResult decode_roster(const fw::AccountResult& result,unsigned offset) {
    if(result.status!=FW_OK)return {result.status,result.payload};
    try {
        const auto number=[](std::string_view text,unsigned maximum){
            unsigned value=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
            if(text.empty()||parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||value>maximum)
                throw std::invalid_argument("invalid roster number");
            return value;
        };
        std::vector<std::string_view> fields;std::string_view remaining=result.payload;
        for(;;){auto at=remaining.find('\t');fields.push_back(remaining.substr(0,at));
            if(at==remaining.npos)break;remaining.remove_prefix(at+1);}
        if(fields.size()<2||offset>8||offset%3)throw std::invalid_argument("invalid roster page");
        RosterResult page{FW_OK,{}};page.total=number(fields[0],8);
        const auto count=number(fields[1],3);
        const auto expected=offset<page.total?std::min(3u,page.total-offset):0u;
        if(count!=expected||fields.size()!=2+count*4)throw std::invalid_argument("invalid roster framing");
        std::set<std::array<std::uint8_t,16>> identities;
        for(unsigned i=0;i<count;++i){
            const auto at=2+4*i;
            RosterEntry entry{Identity::from_hex(fields[at]),std::string(fields[at+1]),
                static_cast<std::uint16_t>(number(fields[at+2],65535)),{}};
            if(!entry.map||!identities.insert(entry.character.bytes).second)
                throw std::invalid_argument("invalid roster identity or map");
            fw::gw::character_name_to_utf16(entry.name,19);
            if(fields[at+3]!="-"){
                const auto hex=fields[at+3];
                std::array<std::uint8_t,fw::gw::CharacterProfile::storage_size> bytes{};
                if(hex.size()!=bytes.size()*2)throw std::invalid_argument("invalid roster profile length");
                for(std::size_t n=0;n<bytes.size();++n){
                    unsigned value=0;const auto parsed=std::from_chars(hex.data()+2*n,hex.data()+2*n+2,value,16);
                    if(parsed.ec!=std::errc{}||parsed.ptr!=hex.data()+2*n+2)
                        throw std::invalid_argument("invalid roster profile hex");
                    bytes[n]=static_cast<std::uint8_t>(value);
                }
                entry.profile=fw::gw::CharacterProfile::decode(bytes);
                if(entry.profile->last_map_id!=entry.map)throw std::invalid_argument("roster profile map mismatch");
            }
            page.entries.push_back(std::move(entry));
        }
        return page;
    }catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid character roster storage response"};}
}
inline CreationResult decode_creation(const fw::AccountResult& result,const CreateCharacter& request) {
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    // The creation record has the same identity/name/map fields as a roster
    // entry. Reuse validation without exposing storage framing to callers.
    const auto page=decode_roster({FW_OK,"1\t1\t"+result.payload+"\t-"},0);
    if(page.status!=FW_OK||page.entries.size()!=1)
        return {FW_INTERNAL,"invalid character creation storage response",{}};
    const auto& entry=page.entries.front();
    if(entry.name!=request.name||entry.map!=(request.profile?request.profile->last_map_id:148))
        return {FW_INTERNAL,"character creation response does not match request",{}};
    return {FW_OK,{},CreatedCharacter{entry.character,entry.name,entry.map}};
}
inline DeletionResult decode_deletion(const fw::AccountResult& result) {
    if(result.status!=FW_OK)return {result.status,result.payload};
    if(result.payload!="deleted")return {FW_INTERNAL,"invalid character deletion storage response"};
    return {FW_OK,{}};
}
inline MapTransitionResult decode_map_transition(const fw::AccountResult& result) {
    if(result.status!=FW_OK)return {result.status,result.payload};
    if(result.payload!="transitioned")return {FW_INTERNAL,"invalid map transition storage response"};
    return {FW_OK,{}};
}
}
