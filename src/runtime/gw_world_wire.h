#pragma once
#include "gw_auth_wire.h"
#include "gw_client_profile.h"

namespace fw::gw::world {
// Supplied client descriptor RVA 0x804a48; gw_current_world_info_wire_probe.py.
inline std::vector<std::uint8_t> instance_info(std::uint32_t agent, std::uint16_t map,
        std::uint8_t explorable, std::uint32_t district, std::uint8_t language, std::uint8_t observer, const ClientProfile& profile=client_reforged) {
    std::vector<std::uint8_t> bytes;
    const auto append=[&](std::uint32_t value,unsigned width){
        for(unsigned i=0;i<width;++i)bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    };
    append(profile.instance_info,2);append(agent,4);append(map,2);append(explorable,1);
    append(district,4);append(language,1);append(observer,1);
    return bytes;
}
struct ItemsRequest {
    std::span<const std::uint8_t> entries{};
    std::size_t consumed{};
};
struct MapRequest {
    bool alternate{};
    std::array<std::uint8_t,2> options{};
    std::size_t consumed{};
};
// Current ordinary sender 0x452e30 and alternate sender 0x452c30.
// Options are opaque client settings; they never authorize a different map.
inline auth::Parse parse_map_request(std::span<const std::uint8_t> bytes,MapRequest& output) {
    if(bytes.empty())return auth::Parse::incomplete;
    if(bytes[0]!=0x89 && bytes[0]!=0x91)return auth::Parse::invalid;
    if(bytes.size()<2)return auth::Parse::incomplete;
    if((bytes[1]&0x7f)!=0)return auth::Parse::invalid;
    const auto size=bytes[0]==0x89?2u:4u;
    if(bytes.size()<size)return auth::Parse::incomplete;
    MapRequest value;value.alternate=bytes[0]==0x89;value.consumed=size;
    if(!value.alternate)value.options={bytes[2],bytes[3]};
    output=value;return auth::Parse::complete;
}
// Current-client outgoing descriptor RVA 0x804754; sender RVA 0x452e80.
// Output borrows the input buffer and is unchanged until a complete frame.
inline auth::Parse parse_items_request(std::span<const std::uint8_t> bytes, ItemsRequest& output,
                                     std::uint8_t expected_opcode=0x92) {
    if (bytes.empty()) return auth::Parse::incomplete;
    if (bytes[0]!=expected_opcode) return auth::Parse::invalid;
    if (bytes.size()<2) return auth::Parse::incomplete;
    if ((bytes[1]&0x7f)!=0) return auth::Parse::invalid;
    if (bytes.size()<4) return auth::Parse::incomplete;
    const std::size_t count=std::size_t(bytes[2]) | (std::size_t(bytes[3])<<8);
    // 2022 FUN_007d09d0 sends a mission mask, with MISSION_MASK_BYTES=0x70.
    // The current client's descriptor permits 116 bytes; keep builds distinct.
    if (count>(expected_opcode==0x90 ? 112u : 116u)) return auth::Parse::invalid;
    if (bytes.size()<4+count) return auth::Parse::incomplete;
    output={bytes.subspan(4,count),4+count};
    return auth::Parse::complete;
}
struct Preface {
    std::uint32_t build{}, platform{}, world{}, map{}, player{};
    GameToken account{}, character{};
    std::array<std::uint32_t, 2> tail{};
    std::size_t consumed{};
};
// Current-client constructor RVA 0x90b00; see gw_current_world_preface_probe.py.
// Decode structure here; build compatibility and transfer authorization belong
// to the session handler. The two trailing DWORD meanings remain unverified.
inline auth::Parse parse_preface(std::span<const std::uint8_t> bytes, Preface& output) {
    constexpr std::array<std::uint8_t, 4> header{0, 5, 12, 0};
    const auto count = std::min(bytes.size(), header.size());
    if (!std::equal(bytes.begin(), bytes.begin()+count, header.begin())) return auth::Parse::invalid;
    if (bytes.size() < 64) return auth::Parse::incomplete;
    auto read = [&](std::size_t at) {
        return std::uint32_t(bytes[at]) | (std::uint32_t(bytes[at+1])<<8) |
            (std::uint32_t(bytes[at+2])<<16) | (std::uint32_t(bytes[at+3])<<24);
    };
    Preface value;
    value.build=read(4); value.platform=read(8); value.world=read(12);
    value.map=read(16); value.player=read(20);
    std::copy_n(bytes.begin()+24,16,value.account.begin());
    std::copy_n(bytes.begin()+40,16,value.character.begin());
    value.tail={read(56),read(60)}; value.consumed=64;
    output=value;
    return auth::Parse::complete;
}
}
