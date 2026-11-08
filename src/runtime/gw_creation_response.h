#pragma once
#include "gw_auth_responses.h"
#include "framework/module.h"
#include <algorithm>
#include <optional>
#include <string>
namespace fw::gw::creation {
// Expected application rejections stay in the creation wizard. Error29 is
// name unavailable;19 is the existing generic creation rejection response.
// Transport/internal failures remain distinct and are handled by the caller.
inline std::optional<std::uint32_t> persistence_rejection(std::uint32_t status){
    switch(status){
    case FW_CONFLICT:return 29;
    case FW_INVALID:case FW_OVERLOADED:case FW_NOT_FOUND:return 19;
    default:return std::nullopt;
    }
}
// Native394 dispatches UI10000095 with a DWORD error.
inline std::vector<std::uint8_t> failure_2022(std::uint32_t error){
    if(!error)throw std::invalid_argument("creation failure requires an error");
    std::vector<std::uint8_t> packet;
    auth::encoding::integer(packet,394,2);auth::encoding::integer(packet,error,4);
    return packet;
}
// Build 37130 descriptor 60dae0: four BYTE masks, not four DWORDs.
inline std::vector<std::uint8_t> campaigns_2022(std::uint8_t owned_pve,
    std::uint8_t owned_pvp=0,std::uint8_t extra_pve=0,std::uint8_t extra_pvp=0){
    return {0x7b,0x01,owned_pve,owned_pvp,extra_pve,extra_pvp};
}
// Build 37130 descriptor 60db70 -> 7ccd10 -> UI 10000094.
// Summary remains version-specific opaque data until its writer is verified.
struct Created {
    GameToken character{};
    std::u16string name;
    std::uint16_t unknown=0;
    std::vector<std::uint8_t> summary;
    std::vector<std::uint8_t> encode_2022()const{
        if(std::all_of(character.begin(),character.end(),[](auto b){return b==0;})||
           name.empty()||name.size()>=20||name.find(u'\0')!=name.npos||summary.size()>64)
            throw std::invalid_argument("invalid creation response fields");
        using namespace auth::encoding;
        std::vector<std::uint8_t> out;
        integer(out,391,2);raw(out,character);integer(out,static_cast<std::uint32_t>(name.size()),2);
        for(auto c:name)integer(out,c,2);
        // Wire descriptor capacity is 1024; charcreate.cpp:0x426 asserts
        // above NET_CHARACTER_SUMMARY_MAX (64) before copying into its UI.
        integer(out,unknown,2);array(out,summary,1024);return out;
    }
};
}
