#pragma once
#include "gw_token.h"
#include "gw_character_profile.h"
#include <span>
#include <vector>
#include <stdexcept>
namespace fw::gw::auth {
namespace encoding {
inline void integer(std::vector<std::uint8_t>& out, std::uint32_t value, unsigned size) {
    for (unsigned i=0;i<size;++i) out.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
inline void raw(std::vector<std::uint8_t>& out, std::span<const std::uint8_t> bytes) { out.insert(out.end(),bytes.begin(),bytes.end()); }
inline void array(std::vector<std::uint8_t>& out, std::span<const std::uint8_t> bytes, unsigned limit) {
    if (bytes.size()>limit) throw std::invalid_argument("GW response array bound");
    integer(out,static_cast<std::uint32_t>(bytes.size()),2); raw(out,bytes);
}
}
// Pinned Gw.exe build 38888 descriptor at RVA 0x7f905c is 1,404,404:
// opcode followed by server salt and one undocumented 32-bit field (10 bytes).
struct SessionInfo {
    std::uint32_t server_salt=0, unknown=0, legacy_unknown2=0, legacy_unknown3=0;
    std::vector<std::uint8_t> encode(std::uint32_t build=38888) const {
        std::vector<std::uint8_t> out;
        encoding::integer(out,0x0001,2); encoding::integer(out,server_salt,4);
        encoding::integer(out,unknown,4);
        // 2022 descriptor RVA 0x61f85c: opcode 1 and four DWORDs.
        if(build==37130){encoding::integer(out,legacy_unknown2,4);encoding::integer(out,legacy_unknown3,4);}
        return out;
    }
};
// Unknown fields remain explicit: their meaning must not be guessed from layout.
struct AccountInfo {
    std::uint32_t transaction=0, territory=0, unknown0=0;
    std::array<std::uint8_t,8> unknown1{},unknown2{};
    GameToken account{},character{};
    std::uint32_t unknown3=0, attribute_count=0;
    std::array<std::uint8_t,200> attributes{};
    // The in-memory AccountInfo descriptor is 275 bytes. On wire, ARRAY_8 uses
    // a uint16 length followed by that many bytes before the agreement version.
    // It carries the accepted agreement version, not a boolean.
    std::uint8_t agreement_version=0;
    std::int32_t unknown4=0;
    std::vector<std::uint8_t> encode() const {
        using namespace encoding; std::vector<std::uint8_t> out;
        if (attribute_count > attributes.size()) throw std::invalid_argument("GW account attribute bound");
        integer(out,0x0011,2); integer(out,transaction,4); integer(out,territory,4); integer(out,unknown0,4);
        raw(out,unknown1);raw(out,unknown2);raw(out,account);raw(out,character);
        integer(out,unknown3,4);integer(out,attribute_count,2);
        raw(out,std::span(attributes).first(attribute_count));integer(out,agreement_version,1);
        integer(out,static_cast<std::uint32_t>(unknown4),4);
        if (out.size()!=73+attribute_count) throw std::logic_error("GW AccountInfo wire layout mismatch");
        return out;
    }
};
struct AccountInfoProfile {
    std::uint32_t territory=0, unknown0=0, unknown3=0;
    std::array<std::uint8_t,8> unknown1{},unknown2{};
    std::uint32_t attribute_count=0;
    std::array<std::uint8_t,200> attributes{};
    std::uint8_t agreement_version=0;
    std::int32_t unknown4=0;
    static AccountInfoProfile decode(std::span<const std::uint8_t> bytes) {
        if(bytes.size()<39 || bytes.size()>239 || bytes[0]!='F' || bytes[1]!='W' || bytes[2]!='A' || bytes[3]!='P')
            throw std::invalid_argument("invalid GW account profile header or size");
        std::size_t offset=4;
        auto read=[&](unsigned count) { std::uint32_t value=0;
            if(offset+count>bytes.size())throw std::invalid_argument("truncated GW account profile");
            for(unsigned i=0;i<count;++i)value|=std::uint32_t(bytes[offset++])<<(i*8);
            return value; };
        AccountInfoProfile profile;
        profile.territory=read(4);profile.unknown0=read(4);
        for(auto& byte:profile.unknown1)byte=static_cast<std::uint8_t>(read(1));
        for(auto& byte:profile.unknown2)byte=static_cast<std::uint8_t>(read(1));
        profile.unknown3=read(4);profile.attribute_count=read(2);
        if(profile.attribute_count>profile.attributes.size() || bytes.size()!=39+profile.attribute_count)
            throw std::invalid_argument("invalid GW account profile attributes");
        for(std::size_t i=0;i<profile.attribute_count;++i)profile.attributes[i]=static_cast<std::uint8_t>(read(1));
        profile.agreement_version=static_cast<std::uint8_t>(read(1));profile.unknown4=static_cast<std::int32_t>(read(4));
        return profile;
    }
    void apply(AccountInfo& value) const {
        value.territory=territory; value.unknown0=unknown0;
        value.unknown1=unknown1; value.unknown2=unknown2;
        value.unknown3=unknown3; value.attribute_count=attribute_count;
        value.attributes=attributes; value.agreement_version=agreement_version; value.unknown4=unknown4;
    }
};
struct CharacterInfo {
    std::uint32_t transaction=0;
    GameToken character{};
    std::uint32_t unknown=0;
    std::u16string name;
    std::uint32_t extended_count=0;
    std::uint16_t unknown1=0, last_map_id=1;
    std::array<std::uint16_t,30> extended{};
    void apply(const fw::gw::CharacterProfile& profile) {
        unknown=profile.unknown0;extended_count=profile.extension_count;
        unknown1=profile.unknown1;last_map_id=profile.last_map_id;extended=profile.extended;
    }
    std::vector<std::uint8_t> encode() const {
        using namespace encoding;
        if (name.empty() || name.size()>=20) throw std::invalid_argument("GW character name bound");
        for (std::size_t i=0;i<name.size();++i) {
            auto c=name[i];
            if (!c || (c>=0xdc00 && c<=0xdfff)) throw std::invalid_argument("GW character name encoding");
            if (c>=0xd800 && c<=0xdbff)
                if (++i>=name.size() || name[i]<0xdc00 || name[i]>0xdfff) throw std::invalid_argument("GW character name encoding");
        }
        if (extended_count > 62) throw std::invalid_argument("GW character extension bound");
        std::vector<std::uint8_t> out;
        integer(out,0x0007,2);integer(out,transaction,4);raw(out,character);integer(out,unknown,4);
        // The in-memory descriptor is fixed-size, but TYPE_STRING_16 and
        // TYPE_ARRAY_8 are length-prefixed on wire by the donor packer.
        integer(out,static_cast<std::uint32_t>(name.size()),2);
        for(auto c:name) integer(out,c,2);
        integer(out,extended_count,2);
        std::vector<std::uint8_t> extension;
        integer(extension,unknown1,2);integer(extension,last_map_id,2);
        for(auto value:extended) integer(extension,value,2);
        raw(out,std::span(extension).first(extended_count));
        return out;
    }
};
struct ErrorMessage {
    std::uint32_t transaction=0, code=0;
    std::vector<std::uint8_t> encode() const {
        std::vector<std::uint8_t> out;
        encoding::integer(out,0x0003,2); encoding::integer(out,transaction,4); encoding::integer(out,code,4);
        return out;
    }
};
// Current Gw.exe's server descriptor at RVA 0x7fc0c4 and donor
// AUTH_SERVER_0027 both define opcode 0x1b as four DWORDs after the opcode.
// The callback treats the first as a transaction and consumes three values;
// their meanings are not established, so this encoder is not wired into login.
struct AuthDataResponse {
    std::uint32_t transaction=0, field0=0, field1=0, field2=0;
    std::vector<std::uint8_t> encode() const {
        std::vector<std::uint8_t> out;
        encoding::integer(out,0x001b,2); encoding::integer(out,transaction,4);
        encoding::integer(out,field0,4); encoding::integer(out,field1,4);
        encoding::integer(out,field2,4);
        return out;
    }
};
// AUTH_SERVER_0022 (opcode 0x16) completes GcApi auth-data lookups. The
// callback consumes the first DWORD of the 16-bit-length-prefixed byte array
// as status and the remaining bytes as the opaque result payload.
struct AuthDataLookupResult {
    std::uint32_t transaction=0, status=0;
    std::vector<std::uint8_t> data;
    std::vector<std::uint8_t> encode(bool status_prefix=true) const {
        std::vector<std::uint8_t> body;
        if(status_prefix)encoding::integer(body,status,4);
        encoding::raw(body,data);
        std::vector<std::uint8_t> out;
        encoding::integer(out,0x0016,2);
        encoding::integer(out,transaction,4);
        encoding::array(out,body,2048);
        return out;
    }
};
struct ServerResponse {
    std::uint32_t transaction=0, code=0;
    std::vector<std::uint8_t> encode() const {
        std::vector<std::uint8_t> out;
        encoding::integer(out,0x0026,2); encoding::integer(out,transaction,4); encoding::integer(out,code,4);
        return out;
    }
};
struct GameServerInfo {
    std::uint32_t transaction=0, world_id=0, map_id=0, player_id=0;
    std::array<std::uint8_t,24> host{};
    std::vector<std::uint8_t> encode() const {
        std::vector<std::uint8_t> out;
        encoding::integer(out,0x0009,2);
        encoding::integer(out,transaction,4);
        encoding::integer(out,world_id,4);
        encoding::integer(out,map_id,4);
        encoding::raw(out,host);
        encoding::integer(out,player_id,4);
        return out;
    }
};
}
