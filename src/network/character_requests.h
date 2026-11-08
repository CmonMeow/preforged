#pragma once
#include "gw_character_profile.h"
#include <algorithm>
#include <optional>
#include <string_view>

namespace gwserver {
struct Identity {
    std::array<std::uint8_t,16> bytes{};
    static Identity from_hex(std::string_view text){
        if(text.size()!=32)throw std::invalid_argument("identity requires 16 bytes");
        const auto digit=[](char c)->unsigned{
            if(c>='0'&&c<='9')return c-'0';
            if(c>='a'&&c<='f')return c-'a'+10;
            throw std::invalid_argument("identity requires lowercase hexadecimal");
        };
        Identity id;for(unsigned n=0;n<16;++n)id.bytes[n]=std::uint8_t(digit(text[n*2])*16+digit(text[n*2+1]));
        return id;
    }
};
struct CreateCharacter {Identity account;std::string name;std::optional<fw::gw::CharacterProfile> profile;};
struct DeleteCharacter {Identity account,character;};
struct CharacterRoster {Identity account;unsigned offset=0;};
struct SelectMap {Identity account,character;std::uint32_t expected_map,destination;};

// Temporary storage encoding stays at the application boundary, not in the
// retail packet handler or future custom client. These are not wire packets.
inline std::string storage_hex(std::span<const std::uint8_t> bytes){
    constexpr char hex[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2);
    for(auto byte:bytes){result+=hex[byte>>4];result+=hex[byte&15];}return result;
}
inline void validate_creation_name(std::string_view name){
    if(name.size()>76)throw std::invalid_argument("invalid character name");
    const auto native=fw::gw::character_name_to_utf16(name,19);
    if(!std::all_of(native.begin(),native.end(),[](char16_t c){
        return c>=0x80||(c>=u'A'&&c<=u'Z')||(c>=u'a'&&c<=u'z')||
            (c>=u'0'&&c<=u'9')||c==u' '||c==u'-'||c==u'\'';
    }))throw std::invalid_argument("invalid character name");
}
inline std::string storage_payload(const CreateCharacter& value){
    validate_creation_name(value.name);
    auto result=storage_hex(value.account.bytes)+"\t"+value.name;
    if(value.profile)result+="\t"+storage_hex(value.profile->encode());return result;
}
inline std::string storage_payload(const DeleteCharacter& value){
    return storage_hex(value.account.bytes)+"\t"+storage_hex(value.character.bytes);
}
inline std::string storage_payload(const CharacterRoster& value){
    if(value.offset>8||value.offset%3)throw std::invalid_argument("invalid roster page offset");
    return storage_hex(value.account.bytes)+"\t"+std::to_string(value.offset);
}
inline std::string storage_payload(const SelectMap& value){
    if(!value.expected_map||!value.destination||value.expected_map>65535||value.destination>65535||
       value.expected_map==value.destination)throw std::invalid_argument("invalid character map transition");
    return storage_hex(value.account.bytes)+"\t"+storage_hex(value.character.bytes)+"\t"+
        std::to_string(value.expected_map)+"\t"+std::to_string(value.destination);
}
}
