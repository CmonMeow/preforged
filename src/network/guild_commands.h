#pragma once
#include "skill_state.h"
namespace gwserver {
enum class GuildAction {create,join,leave,motd,invite,kick,promote,demote,leader};
inline constexpr std::string_view guild_actions[]={"create","join","leave","motd","invite","kick","promote","demote","leader"};
inline std::optional<GuildAction> guild_action(std::string_view text){for(unsigned i=0;i<std::size(guild_actions);++i)if(text==guild_actions[i])return static_cast<GuildAction>(i);return {};}
struct GuildCommand {OwnedCharacter owner;GuildAction action;std::string argument;};
struct GuildCommandResult {std::uint32_t status;std::string message;};
inline std::string storage_payload(const GuildCommand& r){
    const auto action=static_cast<unsigned>(r.action);if(action>=std::size(guild_actions))throw std::invalid_argument("unknown guild command");
    if(!r.argument.empty())fw::gw::character_name_to_utf16(r.argument,255);
    return storage_payload(r.owner)+"\t"+std::string(guild_actions[action])+"\t"+storage_hex(std::span(reinterpret_cast<const std::uint8_t*>(r.argument.data()),r.argument.size()));
}
inline GuildCommandResult decode_guild_command(const fw::AccountResult& r){
    try{fw::gw::character_name_to_utf16(r.payload,255);return {r.status,r.payload};}
    catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid guild command response"};}
}
}
