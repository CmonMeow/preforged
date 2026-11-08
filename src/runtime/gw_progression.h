#pragma once
#include "gw_outgoing_frame.h"
#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>
#include <limits>
namespace fw::gw::progression {
using Packet=std::vector<std::uint8_t>;
inline void word(Packet& b,unsigned n){b.push_back(n);b.push_back(n>>8);}
inline void dword(Packet& b,std::uint32_t n){for(unsigned i=0;i<4;++i)b.push_back(n>>(8*i));}
// Pinned37130 descriptors60b8e0/60b8f8: account name then six counters.
// Unimplemented tournament standings remain zero; reward balance is durable.
inline Packet tournament_account(std::uint32_t points,bool initialize){
 Packet b;word(b,initialize?48:50);if(initialize)word(b,0);
 for(unsigned i=0;i<5;++i)dword(b,0);dword(b,points);return b;
}
inline void text(Packet& b,const std::u16string& s){if(s.size()>=8||s.find(u'\0')!=s.npos)throw std::invalid_argument("title encoded string bound");word(b,s.size());for(auto c:s)word(b,c);}
enum class Faction:unsigned {kurzick,luxon,balthazar,imperial};
struct Counter {std::uint32_t current=0,total=0,maximum=0;};
struct Factions {std::array<Counter,4> counters{{{0,0,10000},{0,0,10000},{0,0,10000},{0,0,20000}}};};
inline void validate(const Factions& p){for(auto c:p.counters)if(c.current>c.maximum||c.current>c.total)throw std::invalid_argument("faction current exceeds cap/lifetime");
 if(p.counters[0].maximum>70000||p.counters[1].maximum>70000||p.counters[2].maximum>250000||p.counters[3].maximum!=p.counters[0].maximum+p.counters[1].maximum)throw std::invalid_argument("invalid faction caps");}
inline Factions earn(Factions p,Faction kind,std::uint32_t amount){validate(p);auto& c=p.counters.at(unsigned(kind));const auto granted=std::min(amount,c.maximum-c.current);if(granted>UINT32_MAX-c.total)throw std::invalid_argument("faction lifetime overflow");c.current+=granted;c.total+=granted;return p;}
inline Factions spend(Factions p,Faction kind,std::uint32_t amount){validate(p);auto& c=p.counters.at(unsigned(kind));if(amount>c.current)throw std::invalid_argument("insufficient faction");c.current-=amount;return p;}
inline Factions exchange_imperial(Factions p,Faction destination,std::uint32_t amount){
 validate(p);if(destination==Faction::imperial||!amount)throw std::invalid_argument("invalid Imperial exchange");
 const auto credit=destination==Faction::balthazar?amount/3:amount;
 return earn(spend(p,Faction::imperial,amount),destination,credit);
}
struct Rank {std::uint32_t props=0,number=0;std::u16string name;};
// Eight DWORDs copied into pinned2022 Title40-byte struct, not modern44-byte.
struct Track {std::array<std::uint32_t,8> fields{};std::u16string progress,hint;};
struct Character {std::uint32_t skill_points=0,skill_points_total=0,hero_points=0,hero_points_total=0;std::uint8_t active=255;std::map<std::uint32_t,Rank> ranks;std::map<std::uint32_t,Track> tracks;};
// GWCA TitleID::Vanguard is 40. The selected display title is unrelated to
// PvE-skill scaling: resolve the earned tier from this character's track.
inline std::optional<unsigned> earned_pve_rank(const Character& p,unsigned track_id){
 if(track_id!=17&&track_id!=20&&track_id!=39&&track_id!=40&&track_id!=41)return {};
 auto track=p.tracks.find(track_id);if(track==p.tracks.end()||!track->second.fields[2])return 0u;
 auto rank=p.ranks.find(track->second.fields[2]);
 if(rank==p.ranks.end()||rank->second.number>10)return {};
 return rank->second.number;
}
inline std::optional<unsigned> ebon_vanguard_rank(const Character& p){return earned_pve_rank(p,40);}
// GWCA TitleID::Lightbringer is20. The earned tier scales Lightbringer skills;
// the character's selected display title is independent of this value.
inline std::optional<unsigned> lightbringer_rank(const Character& p){return earned_pve_rank(p,20);}
// GWCA TitleID::Deldrimor is39; the earned tier, not the selected display
// title, scales Eye of the North Deldrimor skills.
inline std::optional<unsigned> deldrimor_rank(const Character& p){return earned_pve_rank(p,39);}
// GWCA TitleID::Norn immediately follows Vanguard. PvE skill rank is an
// earned account track, independent of the displayed character title.
inline std::optional<unsigned> norn_rank(const Character& p){return earned_pve_rank(p,41);}
// Legacy GWCA TitleID::Sunspear is17; the earned rank, not the selected
// display title, scales Sunspear Rebirth Signet.
inline std::optional<unsigned> sunspear_rank(const Character& p){return earned_pve_rank(p,17);}
// Kurzick/Luxon title tracks are account-earned, independent of the selected
// display title. Their PvE skills use the corresponding track, never the
// opposing faction's rank.
inline std::optional<unsigned> allegiance_rank(const Character& p,unsigned skill){
 const unsigned track_id=(skill==1950||skill==1951||skill==1952||skill==1953||skill==1954)?5u:(skill==2093||skill==2094||skill==2095||skill==2096||skill==2097)?6u:UINT32_MAX;
 if(track_id==UINT32_MAX)return {};
 const auto track=p.tracks.find(track_id);
 if(track==p.tracks.end()||!track->second.fields[2])return 0u;
 const auto rank=p.ranks.find(track->second.fields[2]);
 if(rank==p.ranks.end()||rank->second.number>12)return {};
 return rank->second.number;
}
inline void validate(const Character& p){if(p.skill_points>p.skill_points_total||p.hero_points>p.hero_points_total||p.ranks.size()>1024||p.tracks.size()>48)throw std::invalid_argument("invalid progression counters");
 for(const auto& [id,r]:p.ranks){const bool terminal=r.number==UINT32_MAX&&r.name==std::u16string{char16_t(0x101)};if(!id||id>4095||(!terminal&&r.number>((r.props&1)?1000u:50u))||r.name.empty()||r.name.size()>=8||r.name.find(u'\0')!=r.name.npos)throw std::invalid_argument("invalid title rank");}
 for(const auto& [id,t]:p.tracks){if(id>=48||id==8||id==11||id==12||t.progress.size()>=8||t.hint.size()>=8||t.progress.find(u'\0')!=t.progress.npos||t.hint.find(u'\0')!=t.hint.npos)throw std::invalid_argument("invalid title track");
  if(t.fields[2]&&!p.ranks.contains(t.fields[2]))throw std::invalid_argument("title references missing current rank");
  if(t.fields[4]&&t.fields[5]!=UINT32_MAX&&!p.ranks.contains(t.fields[4]))throw std::invalid_argument("title references missing next rank");
  if(t.fields[2]&&t.fields[1]<t.fields[3])throw std::invalid_argument("unearned current title rank");
 }
 if(p.active!=255){auto t=p.tracks.find(p.active);if(t==p.tracks.end()||!t->second.fields[2]||!p.ranks.at(t->second.fields[2]).number||p.ranks.at(t->second.fields[2]).number==UINT32_MAX)throw std::invalid_argument("active title not earned");}
}
inline Packet active_title(std::uint16_t player_number,std::uint32_t rank){if(!player_number||rank>4095)throw std::invalid_argument("title display identity");Packet b;word(b,243);word(b,player_number);dword(b,rank);return b;}
inline std::optional<Character> select_title(const Character& p,std::span<const std::uint8_t> request){validate(p);auto frame=world::outgoing_frame_2022(request);if(frame.state!=world::FrameState::complete||frame.size!=request.size())return {};auto out=p;const auto op=(request[0]|(unsigned(request[1])<<8))&0x7fff;
 if(op==87){out.active=255;return out;}if(op!=86||request.size()!=6)return {};std::uint32_t id=0;for(unsigned i=0;i<4;++i)id|=std::uint32_t(request[i+2])<<(8*i);auto t=p.tracks.find(id);if(t==p.tracks.end()||!t->second.fields[2]||!p.ranks.at(t->second.fields[2]).number||p.ranks.at(t->second.fields[2]).number==UINT32_MAX)return {};out.active=static_cast<std::uint8_t>(id);return out;}
inline std::vector<Packet> restore(const Factions& f,const Character& p,std::uint32_t experience,unsigned level,unsigned morale,std::uint16_t player_number){validate(f);validate(p);if(!level||level>20||morale<40||morale>110)throw std::invalid_argument("progression level/morale");std::array<std::uint32_t,15> stats{experience,f.counters[0].current,f.counters[0].total,f.counters[1].current,f.counters[1].total,f.counters[3].current,f.counters[3].total,p.hero_points,p.hero_points_total,level,morale,f.counters[2].current,f.counters[2].total,p.skill_points,p.skill_points_total};
 std::vector<Packet> packets;Packet b;word(b,232);for(auto n:stats)dword(b,n);packets.push_back(std::move(b));for(unsigned i=0;i<4;++i){b.clear();word(b,233+i);dword(b,f.counters[i].maximum);packets.push_back(b);}
 for(const auto& [id,r]:p.ranks){b.clear();word(b,242);dword(b,id);dword(b,r.props);dword(b,r.number);text(b,r.name);packets.push_back(b);}
 for(const auto& [id,t]:p.tracks){b.clear();word(b,245);dword(b,id);for(auto n:t.fields)dword(b,n);text(b,t.progress);text(b,t.hint);packets.push_back(b);}
 packets.push_back(active_title(player_number,p.active==255?0:p.tracks.at(p.active).fields[2]));return packets;
}
inline Packet encode(const Factions& p){validate(p);Packet b{'F','W','F',1};for(auto c:p.counters){dword(b,c.current);dword(b,c.total);dword(b,c.maximum);}return b;}
struct Reader {std::span<const std::uint8_t> b;std::size_t at=4;unsigned read(unsigned n){if(n>4||at+n>b.size())throw std::invalid_argument("truncated progression");unsigned v=0;for(unsigned i=0;i<n;++i)v|=unsigned(b[at++])<<(8*i);return v;}std::u16string string(){auto n=read(2);if(n>=8)throw std::invalid_argument("title string bound");std::u16string s;for(unsigned i=0;i<n;++i)s.push_back(read(2));return s;}void end(){if(at!=b.size())throw std::invalid_argument("progression trailing bytes");}};
inline Factions decode_factions(std::span<const std::uint8_t> b){if(b.size()!=52||b[0]!='F'||b[1]!='W'||b[2]!='F'||b[3]!=1)throw std::invalid_argument("faction storage format");Reader r{b};Factions p;for(auto& c:p.counters)c={r.read(4),r.read(4),r.read(4)};validate(p);return p;}
inline Packet encode(const Character& p){validate(p);Packet b{'F','W','P',1};dword(b,p.skill_points);dword(b,p.skill_points_total);dword(b,p.hero_points);dword(b,p.hero_points_total);b.push_back(p.active);word(b,p.ranks.size());word(b,p.tracks.size());for(const auto& [id,r]:p.ranks){dword(b,id);dword(b,r.props);dword(b,r.number);text(b,r.name);}for(const auto& [id,t]:p.tracks){dword(b,id);for(auto n:t.fields)dword(b,n);text(b,t.progress);text(b,t.hint);}return b;}
inline Character decode_character(std::span<const std::uint8_t> b){if(b.size()<25||b.size()>65536||b[0]!='F'||b[1]!='W'||b[2]!='P'||b[3]!=1)throw std::invalid_argument("progression storage format");Reader r{b};Character p;p.skill_points=r.read(4);p.skill_points_total=r.read(4);p.hero_points=r.read(4);p.hero_points_total=r.read(4);p.active=r.read(1);auto ranks=r.read(2),tracks=r.read(2);if(ranks>1024||tracks>48)throw std::invalid_argument("progression row bounds");for(unsigned i=0;i<ranks;++i){auto id=r.read(4);Rank rank{r.read(4),r.read(4),r.string()};if(!p.ranks.emplace(id,std::move(rank)).second)throw std::invalid_argument("duplicate rank");}for(unsigned i=0;i<tracks;++i){auto id=r.read(4);Track t;for(auto& n:t.fields)n=r.read(4);t.progress=r.string();t.hint=r.string();if(!p.tracks.emplace(id,std::move(t)).second)throw std::invalid_argument("duplicate track");}r.end();validate(p);return p;}
// GWCA TitleID identities, cross-checked against the local wiki's account
// title list and the captured property4 tracks. Do not infer scope from rank.
inline bool account_title(unsigned id){switch(id){case 0:case 3:case 4:case 5:case 6:case 15:case 16:case 23:case 44:case 45:case 46:case 47:return true;default:return false;}}
inline Character account_titles(const Character& source){
 validate(source);Character result;
 for(const auto& [id,track]:source.tracks)if(account_title(id)){
  result.tracks.emplace(id,track);
  for(unsigned field:{2u,4u})if(auto rank=source.ranks.find(track.fields[field]);rank!=source.ranks.end())result.ranks.emplace(*rank);
 }
 validate(result);return result;
}
inline Character with_account_titles(Character local,const Character& shared){
 validate(local);validate(shared);
 for(auto it=local.tracks.begin();it!=local.tracks.end();)if(account_title(it->first))it=local.tracks.erase(it);else ++it;
 for(const auto& [id,track]:shared.tracks){if(!account_title(id))throw std::invalid_argument("character track in account title profile");local.tracks[id]=track;}
 for(const auto& [id,rank]:shared.ranks)local.ranks[id]=rank;
 if(local.active!=255){auto track=local.tracks.find(local.active);if(track==local.tracks.end()||!track->second.fields[2]||!local.ranks.at(track->second.fields[2]).number||local.ranks.at(track->second.fields[2]).number==UINT32_MAX)local.active=255;}
 validate(local);return local;
}
}
