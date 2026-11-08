#pragma once
#include "character_state.h"
#include <set>
namespace gwserver {
struct GuildMember {Identity account;std::string alias;unsigned rank;};
struct GuildState {Identity id;std::string name,tag,announcement,author;std::uint16_t hall;std::vector<GuildMember> members;};
using GuildResult=CharacterStateResult<GuildState>;
inline std::string guild_text(const std::string& hex,unsigned units,bool empty=false){
    if(hex.empty()&&empty)return {};
    auto bytes=profile_bytes(hex,units*4);std::string text(bytes.begin(),bytes.end());
    fw::gw::character_name_to_utf16(text,units);return text;
}
inline GuildResult decode_guild(const fw::AccountResult& r,const Identity& owner){
    if(r.status!=FW_OK)return {r.status,r.payload,{}};
    if(r.payload=="0")return {FW_OK,{},{}};
    try{
        const auto fields=fw::split(r.payload);if(fields.size()<7)throw std::invalid_argument("guild framing");
        const auto count=fw::number(fields[6],100);if(!count||fields.size()!=7+count*3)throw std::invalid_argument("guild roster framing");
        GuildState state{Identity::from_hex(fields[0]),guild_text(fields[1],31),guild_text(fields[2],4),guild_text(fields[3],255,true),guild_text(fields[4],19,true),static_cast<std::uint16_t>(fw::number(fields[5],65535)),{}};
        std::set<std::string> seen;bool owned=false;unsigned leaders=0;
        for(unsigned i=0;i<count;++i){const auto at=7+3*i;auto id=Identity::from_hex(fields[at]);
            if(!seen.insert(fields[at]).second)throw std::invalid_argument("duplicate guild member");
            const auto rank=static_cast<unsigned>(fw::number(fields[at+2],2));owned|=id.bytes==owner.bytes;leaders+=rank==0;
            state.members.push_back({id,guild_text(fields[at+1],19),rank});
        }
        if(!owned||leaders!=1)throw std::invalid_argument("guild membership missing");
        return {FW_OK,{},std::move(state)};
    }catch(const std::exception&){return {FW_INTERNAL,"invalid guild storage response",{}};}
}
}
