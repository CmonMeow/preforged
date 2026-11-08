#pragma once
#include "gw_guild_cape.h"
#include "gw_npc_dialogue.h"
namespace fw::gw::guild::trim {
// Native color IDs corroborated by the observer source's GuildBannerComposer.
// Private-server policy includes the requested gold purchase option.
struct Offer {std::uint8_t id;unsigned price;std::u16string_view name;};
inline constexpr Offer offers[]={{1,7500,u"Silver"},{2,15000,u"Gold"},{4,2500,u"Red"},{5,2500,u"Blue"},{6,2500,u"Green"},{7,2500,u"Purple"},{8,2500,u"Orange"},{9,15000,u"Obsidian"},{11,2500,u"Pink"}};
inline const Offer* find(unsigned id){for(const auto& offer:offers)if(offer.id==id)return &offer;return nullptr;}
inline std::u16string decimal(unsigned value){const auto s=std::to_string(value);return {s.begin(),s.end()};}
inline constexpr unsigned choose=0x700000,confirm=0x710000,cancel=0x720000;
inline dialogue::Batch menu(std::uint32_t actor,unsigned balance){
    std::vector<dialogue::Button> buttons;
    for(const auto& offer:offers)buttons.push_back({std::u16string(offer.name)+u" trim - "+decimal(offer.price)+u" Tournament Reward Points",choose+offer.id});
    buttons.push_back({u"Goodbye",cancel});
    return dialogue::open(actor,u"Guild leaders can replace their cape trim using Tournament Reward Points. Your balance: "+decimal(balance)+u".",buttons);
}
inline dialogue::Batch confirmation(std::uint32_t actor,const Offer& offer){
    return dialogue::open(actor,u"Replace your guild's current trim permanently with "+std::u16string(offer.name)+u" trim for "+decimal(offer.price)+u" Tournament Reward Points?",{{u"Yes, replace the trim",confirm+offer.id},{u"Wait, I need to think",cancel}});
}
}
