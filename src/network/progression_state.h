#pragma once
#include "skill_state.h"
#include "gw_progression.h"
#include "character_state.h"
namespace gwserver {
using FactionsResult=ProfileResult<fw::gw::progression::Factions>;
using ProgressionResult=ProfileResult<fw::gw::progression::Character>;
struct SaveFactions {Identity account;std::uint64_t expected_revision;fw::gw::progression::Factions profile;};
struct SaveProgression {OwnedCharacter owner;std::uint64_t expected_revision;fw::gw::progression::Character profile;};
struct AwardFaction {Identity account;std::uint64_t expected_revision;fw::gw::progression::Faction faction;std::uint32_t amount;};
struct ExchangeImperialFaction {Identity account;std::uint64_t expected_revision;fw::gw::progression::Faction destination;std::uint32_t amount;};
inline std::string storage_payload(const SaveFactions& r){check_revision(r.expected_revision);return storage_hex(r.account.bytes)+"\t"+std::to_string(r.expected_revision)+"\t"+storage_hex(fw::gw::progression::encode(r.profile));}
inline std::string storage_payload(const SaveProgression& r){check_revision(r.expected_revision);return storage_payload(r.owner)+"\t"+std::to_string(r.expected_revision)+"\t"+storage_hex(fw::gw::progression::encode(r.profile));}
inline std::string faction_mutation_payload(const Identity& account,std::uint64_t revision,unsigned faction,std::uint32_t amount,unsigned maximum){
    check_revision(revision);if(faction>maximum||!amount)throw std::invalid_argument("invalid faction mutation");
    return storage_hex(account.bytes)+"\t"+std::to_string(revision)+"\t"+std::to_string(faction)+"\t"+std::to_string(amount);
}
inline std::string storage_payload(const AwardFaction& r){return faction_mutation_payload(r.account,r.expected_revision,static_cast<unsigned>(r.faction),r.amount,3);}
inline std::string storage_payload(const ExchangeImperialFaction& r){return faction_mutation_payload(r.account,r.expected_revision,static_cast<unsigned>(r.destination),r.amount,2);}
template<class Profile,class Decoder> inline ProfileResult<Profile> decode_progression_profile(const fw::AccountResult& r,std::size_t bound,Decoder decode,bool allow_zero=false){
    if(r.status!=FW_OK)return {r.status,r.payload,{}};
    if(r.payload=="0")return {FW_OK,{},{}};
    try{const auto fields=fw::split(r.payload);if(fields.size()!=2)throw std::invalid_argument("progression framing");
        const auto revision=allow_zero&&fields[0]=="0"?0:read_revision(fields[0]);
        return {FW_OK,{},VersionedProfile<Profile>{revision,decode(profile_bytes(fields[1],bound))}};
    }catch(const std::exception&){return {FW_INTERNAL,"invalid progression storage response",{}};}
}
inline FactionsResult decode_factions(const fw::AccountResult& r){return decode_progression_profile<fw::gw::progression::Factions>(r,52,fw::gw::progression::decode_factions);}
// Account-wide titles can produce a merged profile before the character has a
// durable progression row; its optimistic revision is legitimately zero.
inline ProgressionResult decode_progression(const fw::AccountResult& r){return decode_progression_profile<fw::gw::progression::Character>(r,65536,fw::gw::progression::decode_character,true);}
struct GuildFaction {unsigned allegiance;std::uint32_t points;};
using GuildFactionResult=CharacterStateResult<GuildFaction>;
using TournamentPointsResult=CharacterStateResult<std::uint32_t>;
inline std::uint32_t progression_number(const std::string& s){
    std::uint32_t n=0;const auto p=std::from_chars(s.data(),s.data()+s.size(),n);
    if(s.empty()||p.ec!=std::errc{}||p.ptr!=s.data()+s.size())throw std::invalid_argument("progression number");return n;
}
inline GuildFactionResult decode_guild_faction(const fw::AccountResult& r){
    if(r.status!=FW_OK)return {r.status,r.payload,{}};
    try{const auto f=fw::split(r.payload);if(f.size()!=2)throw std::invalid_argument("guild faction framing");
        const auto allegiance=progression_number(f[0]);if(allegiance>1)throw std::invalid_argument("guild faction allegiance");
        return {FW_OK,{},GuildFaction{allegiance,progression_number(f[1])}};
    }catch(const std::exception&){return {FW_INTERNAL,"invalid guild faction storage response",{}};}
}
inline TournamentPointsResult decode_tournament_points(const fw::AccountResult& r){
    if(r.status!=FW_OK)return {r.status,r.payload,{}};
    try{return {FW_OK,{},progression_number(r.payload)};}catch(const std::exception&){return {FW_INTERNAL,"invalid tournament points response",{}};}
}
}
