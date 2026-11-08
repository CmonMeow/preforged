#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <optional>
#include <string_view>
namespace fw::gw::world {
inline bool valid_travel_flags_2022(std::uint8_t flags){return flags<=1;}
inline std::optional<std::uint16_t> development_travel_map(std::u16string_view text){
    const auto prefix=text.starts_with(u"/travel ")?std::u16string_view(u"/travel "):
        text.starts_with(u"/tp ")?std::u16string_view(u"/tp "):std::u16string_view{};
    if(prefix.empty())return {};
    text.remove_prefix(prefix.size());
    if(text.empty()||text.size()>5)return {};
    std::uint32_t map=0;
    for(auto c:text){if(c<u'0'||c>u'9')return {};map=map*10+c-u'0';}
    if(!map||map>65535)return {};
    return static_cast<std::uint16_t>(map);
}
// Build37130 descriptor60dcfc; callback7cd610. Host is a 24-byte socket
// address, not a counted blob. Transport framing is added by send_world.
inline std::vector<std::uint8_t> transfer_2022(const std::array<std::uint8_t,24>& host,
        std::uint32_t world,std::uint32_t player,std::uint32_t map,
        std::int32_t region,bool explorable){
    if(!world||!player||player>65535||!map||map>65535||
       !((region>=0&&region<=4)||region==-2))
        throw std::invalid_argument("invalid world transfer destination");
    std::vector<std::uint8_t> packet{0xa4,0x01};
    packet.insert(packet.end(),host.begin(),host.end());
    auto append=[&](std::uint32_t value,unsigned width){
        for(unsigned i=0;i<width;++i)packet.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    };
    append(world,4);append(static_cast<std::uint32_t>(region),1);
    append(map,2);append(explorable,1);append(player,4);
    return packet;
}
}
