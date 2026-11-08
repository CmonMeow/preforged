#pragma once
#include "gw_party_wire.h"
#include "gw_guild_wire.h"
#include <string>
namespace fw::gw::observer::party_ui {
struct Team {std::uint16_t party_id=0;std::uint32_t token=0;std::uint8_t flags=0,color_key=0,unknown=0;std::u16string encoded_name;std::vector<std::uint16_t> player_numbers;};
// Pinned37130 registry A0E2B4: {A0E104,1,7D5000}; descriptor {468}.
// Callback 7D5000 -> 7D7FC0 sets party flags bit8 and emits UI10000129.
// That event creates the observed-party HUD child and aggregates live team
// tokens. Send after player identities, mission teams/rosters and opcode34's
// game-view initialization; sending before the view exists loses the event.
inline party::Packet activate_hud(){return {0xd4,0x01};}
// Native429 descriptorA281D0 and callback7CD960->87CFC0. The corresponding
// mission428 must already have flags2; its guild decoration is separate from
// guild288 and individual actor affiliation. Send before HUD initialization.
inline party::Packet mission_guild(std::uint16_t party_id,const guild::General& g){
 if(!party_id||g.name.empty()||g.tag.size()>4)throw std::invalid_argument("invalid observer mission guild");
 party::Packet p;party::word(p,429);party::word(p,party_id);
 for(auto v:{g.background,g.detail_color,g.emblem_color,g.shape,g.detail})p.push_back(v);
 party::word(p,g.emblem);p.push_back(g.trim);items::put(p,g.rank,4);items::put(p,g.rating,4);
 guild::text(p,g.name,32);guild::text(p,g.tag,6);return p;
}
inline void dword(party::Packet& p,std::uint32_t v){for(unsigned n=0;n<4;++n)p.push_back(static_cast<std::uint8_t>(v>>(8*n)));}
// Pinned37130 descriptor6281a0, callback7cd920->87d150.
// 428 creates mission-team token->partyID mapping used by0054b360 observerHUD.
// Do not repeat this packet for an already created party in the same map.
inline party::Packet mission_team(const Team& team){
 if(!team.party_id||!team.token||team.encoded_name.size()>20||team.encoded_name.find(u'\0')!=team.encoded_name.npos)throw std::invalid_argument("invalid observer mission team");
 party::Packet p;party::word(p,428);party::word(p,team.party_id);dword(p,team.token);p.push_back(team.flags);p.push_back(team.color_key);p.push_back(team.unknown);party::word(p,static_cast<std::uint16_t>(team.encoded_name.size()));for(auto c:team.encoded_name)party::word(p,c);return p;
}
// Captured PLAYER_JOIN player_number is the native party member identity;
// the corresponding agentID belongs to the agent/player table, not this list.
// This publishes remote teams without falsely selecting either as spectator's
// own party. Caller sends once after map-loaded UI initialization and actor
// identity publication, before normal spectator team aggregation begins.
inline std::vector<party::Packet> publish(std::span<const Team> teams){
 if(teams.empty()||teams.size()>3)throw std::invalid_argument("invalid observer team count");
 std::set<std::uint16_t> party_ids,players;std::set<std::uint32_t> tokens;std::vector<party::Packet> p;
 for(const auto& team:teams){if(!party_ids.insert(team.party_id).second||!tokens.insert(team.token).second)throw std::invalid_argument("duplicate observer team identity");if(team.player_numbers.empty()||team.player_numbers.size()>12)throw std::invalid_argument("invalid observer team roster");for(auto id:team.player_numbers)if(!id||!players.insert(id).second)throw std::invalid_argument("duplicate or invalid captured player number");
  p.push_back(mission_team(team));party::Packet begin;party::word(begin,465);party::word(begin,team.party_id);p.push_back(std::move(begin));for(auto id:team.player_numbers){party::Packet member;party::word(member,458);party::word(member,team.party_id);party::word(member,id);member.push_back(1);p.push_back(std::move(member));}party::Packet end;party::word(end,466);party::word(end,team.party_id);p.push_back(std::move(end));
 }
 return p;
}
}
