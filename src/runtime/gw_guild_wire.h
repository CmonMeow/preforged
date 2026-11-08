#pragma once
#include "gw_item_wire.h"
#include <array>
#include <algorithm>

namespace fw::gw::guild {
using Packet=items::Packet;using Batch=std::vector<Packet>;
using Key=std::array<std::uint32_t,4>;
enum class Rank:std::uint8_t {member=3,officer=4,leader=5};
inline std::uint8_t native_rank(unsigned private_rank){if(private_rank>2)throw std::invalid_argument("private guild rank invalid");return static_cast<std::uint8_t>(5-private_rank);}
inline void text(Packet& out,const std::u16string& value,unsigned capacity){
 if(value.size()>=capacity||value.find(u'\0')!=std::u16string::npos)throw std::invalid_argument("guild text exceeds native capacity");
 items::put(out,static_cast<std::uint32_t>(value.size()),2);for(auto c:value)items::put(out,c,2);
}
struct General {
 std::uint16_t local_id=0;Key key{};std::u16string name,tag;
 std::uint8_t features=0,territory=0,background=0,detail_color=0,emblem_color=0,shape=0,detail=0,trim=0,faction=0,unknown=0;
 std::uint16_t emblem=0;
 std::uint32_t faction_points=0,qualifier_points=0,rating=0,rank=0;
};
inline Packet general(const General& g){
 if(!g.local_id||g.name.empty()||g.tag.size()>4)throw std::invalid_argument("guild identity/name/tag invalid");
 Packet p;items::put(p,288,2);items::put(p,g.local_id,2);for(auto k:g.key)items::put(p,k,4);text(p,g.name,32);text(p,g.tag,6);
 for(auto b:{g.features,g.territory,g.background,g.detail_color,g.emblem_color,g.shape,g.detail})items::put(p,b,1);
 items::put(p,g.emblem,2);items::put(p,g.trim,1);items::put(p,g.faction,1);
 for(auto v:{g.faction_points,g.qualifier_points,g.rating,g.rank})items::put(p,v,4);items::put(p,g.unknown,1);return p;
}
// Native attribute30 assigns a session-local guild index to a living actor.
// Publish general288 before a nonzero affiliation. Zero removes the cape/tag.
inline Packet affiliation(std::uint32_t actor,std::uint16_t local_guild){
 if(!actor||actor>65535)throw std::invalid_argument("invalid guild actor");Packet p;items::put(p,158,2);items::put(p,30,4);items::put(p,actor,4);items::put(p,local_guild,4);return p;
}
inline Batch appearance_update(const General& guild,std::uint32_t actor,bool already_affiliated){
 Batch packets{general(guild)};
 // Native779E00 skips the model rebuild when the guild index is unchanged.
 // A changed cape record needs a real affiliation transition; membership,
 // rank and the guild panel remain intact.
 if(already_affiliated)packets.push_back(affiliation(actor,0));
 packets.push_back(affiliation(actor,guild.local_id));return packets;
}
// Native288 final flags bit2 selects the visited hall at GuildContext+2E8,
// separate from own membership+60. No role279 is implied.
inline Packet visited_hall(General hall){
 if(hall.local_id<=1||std::all_of(hall.key.begin(),hall.key.end(),[](auto part){return part==0;}))
     throw std::invalid_argument("distinct visited hall identity required");
 hall.unknown|=2;return general(hall);
}
// Numeric rank is explicitly supplied by the private guild policy. Packet279
// references a guild already created by288; never send the role first.
inline Packet role(std::uint16_t id,const Key& key,std::uint8_t rank){
 if(!id)throw std::invalid_argument("local guild role identity required");Packet p;items::put(p,279,2);items::put(p,id,2);for(auto k:key)items::put(p,k,4);items::put(p,rank,1);return p;
}
//280 clears local membership, announcement/author and roster and sends UI CA.
inline Packet clear_membership(){Packet p;items::put(p,280,2);return p;}
// Original 7BED50: marks GuildContext+5C ready and emits UI100000CC.
// Without this completion, the Guild window keeps its child panel empty even
// when general, own role, roster and announcement records are present.
inline Packet complete(){Packet p;items::put(p,283,2);return p;}
struct Member {
 std::u16string invited_name,current_name,inviter,context;
 std::uint32_t unknown=0,minutes_since_login=0,join_date=0,status=0;
 std::uint8_t rank=0;
};
inline Packet member(const Member& m){
 if(m.invited_name.empty())throw std::invalid_argument("guild roster key required");Packet p;items::put(p,294,2);
 text(p,m.invited_name,20);text(p,m.current_name,20);text(p,m.inviter,20);text(p,m.context,64);
 for(auto v:{m.unknown,m.minutes_since_login,m.join_date,m.status})items::put(p,v,4);items::put(p,m.rank,1);return p;
}
inline Packet remove(const std::u16string& invited_name){if(invited_name.empty())throw std::invalid_argument("guild removal key required");Packet p;items::put(p,295,2);text(p,invited_name,20);return p;}
inline Packet announcement(const std::u16string& body,const std::u16string& author){Packet p;items::put(p,302,2);text(p,body,256);text(p,author,20);return p;}
inline Batch restore(const General& g,std::uint8_t local_rank,const std::vector<Member>& members,const std::u16string& body={},const std::u16string& author={}){
 if(members.size()>100)throw std::invalid_argument("guild roster exceeds100");Batch p{general(g),role(g.local_id,g.key,local_rank)};for(const auto& m:members)p.push_back(member(m));p.push_back(announcement(body,author));p.push_back(complete());return p;
}
}
