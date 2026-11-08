#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>
#include "gw_character_name.h"

namespace fw::gw {
// Opaque CharacterInfo fields captured from the client's AccountInfo roster.
// These values are preserved as data; the framework does not interpret them.
struct CharacterProfile {
    struct EquipmentAppearance {
        std::uint16_t composite=0,color=0;
        std::uint8_t parameter=0;
        std::uint32_t packed_composite() const {
            return std::uint32_t(composite)|(std::uint32_t(parameter)<<16);
        }
    };
    static constexpr std::size_t storage_size=70;
    std::uint32_t unknown0=0;
    std::uint16_t extension_count=0, unknown1=0, last_map_id=148;
    std::array<std::uint16_t,30> extended{};

    std::uint8_t origin_campaign_2022() const {
        if((unknown1!=6&&unknown1!=8)||extension_count<37)
            throw std::invalid_argument("unsupported campaign summary layout");
        return static_cast<std::uint8_t>(extended[12]&15);
    }

    std::vector<EquipmentAppearance> equipment_2022() const {
        if(unknown1!=6 && unknown1!=8)
            throw std::invalid_argument("unsupported equipment summary version");
        const auto bytes=encode();
        const auto count=bytes[6+32];
        if(count>5 || extension_count!=37+5*count)
            throw std::invalid_argument("unsupported equipment summary shape");
        std::vector<EquipmentAppearance> result;result.reserve(count);
        for(unsigned index=0;index<count;++index){
            const auto at=6+37+5*index;
            result.push_back({static_cast<std::uint16_t>(bytes[at]|(std::uint16_t(bytes[at+1])<<8)),
                static_cast<std::uint16_t>(bytes[at+2]|(std::uint16_t(bytes[at+3])<<8)),bytes[at+4]});
        }
        return result;
    }

    CharacterProfile with_equipment_2022(std::span<const EquipmentAppearance> equipment) const {
        (void)equipment_2022(); // Validate the supported layout before editing it.
        if(equipment.size()>5)throw std::invalid_argument("equipment summary count");
        auto bytes=encode();
        bytes[4]=static_cast<std::uint8_t>(37+5*equipment.size());bytes[5]=0;
        bytes[38]=static_cast<std::uint8_t>(equipment.size());
        for(std::size_t at=43;at<bytes.size();++at)bytes[at]=0;
        for(std::size_t index=0;index<equipment.size();++index){
            const auto at=43+5*index;const auto& item=equipment[index];
            bytes[at]=static_cast<std::uint8_t>(item.composite);bytes[at+1]=static_cast<std::uint8_t>(item.composite>>8);
            bytes[at+2]=static_cast<std::uint8_t>(item.color);bytes[at+3]=static_cast<std::uint8_t>(item.color>>8);
            bytes[at+4]=item.parameter;
        }
        return decode(bytes);
    }

    CharacterProfile for_client(std::uint32_t build) const {
        auto result=*this;
        if(build!=37130 || unknown1!=8)return result;
        // Pinned decoder probes: Reforged RVA 527e50 and 2022 RVA 47c3e0
        // share every legacy field and five-byte equipment record. Version 8
        // adds flag interpretations, but 2022 rejects it before reading them.
        const auto bytes=encode();
        const auto equipment_count=bytes[6+32];
        if(extension_count<37 || equipment_count>5 || extension_count!=37+5*equipment_count)
            throw std::invalid_argument("unsupported version 8 character summary shape");
        result.unknown1=6;
        return result;
    }

    std::vector<std::uint8_t> encode() const {
        if(extension_count>62 || !last_map_id)throw std::invalid_argument("GW character profile bounds");
        std::vector<std::uint8_t> out;out.reserve(storage_size);
        auto put=[&](std::uint32_t value,unsigned bytes){for(unsigned i=0;i<bytes;++i)out.push_back(static_cast<std::uint8_t>(value>>(8*i)));};
        put(unknown0,4);put(extension_count,2);put(unknown1,2);put(last_map_id,2);
        for(auto value:extended)put(value,2);
        if(out.size()!=storage_size)throw std::logic_error("GW character profile size mismatch");
        return out;
    }
    static CharacterProfile decode(std::span<const std::uint8_t> bytes) {
        if(bytes.size()!=storage_size)throw std::invalid_argument("GW character profile size");
        std::size_t at=0;auto get=[&](unsigned n){std::uint32_t value=0;for(unsigned i=0;i<n;++i)value|=std::uint32_t(bytes[at++])<<(8*i);return value;};
        CharacterProfile result;result.unknown0=get(4);result.extension_count=static_cast<std::uint16_t>(get(2));
        result.unknown1=static_cast<std::uint16_t>(get(2));result.last_map_id=static_cast<std::uint16_t>(get(2));
        for(auto& value:result.extended)value=static_cast<std::uint16_t>(get(2));
        if(result.extension_count>62 || !result.last_map_id)throw std::invalid_argument("GW character profile bounds");
        return result;
    }
};
}
