#pragma once
#include "gw_observer_wire.h"
#include "gw_observer_presentation.h"
#include "gw_observer_party.h"
#include "gw_layered_navigation.h"
#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include "gw_profession.h"
#include "gw_equipment_wire.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <functional>

namespace fw::gw::observer {
inline std::vector<std::string> fields(std::string_view s){std::vector<std::string> v;for(std::size_t p=0;;){auto end=s.find(';',p);v.emplace_back(s.substr(p,end==s.npos?s.size()-p:end-p));if(end==s.npos)break;p=end+1;}return v;}
inline unsigned integer(std::string_view s){unsigned n=0;auto r=std::from_chars(s.data(),s.data()+s.size(),n);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size())throw std::runtime_error("invalid replay integer");return n;}
inline float real(std::string_view s){float n=0;auto r=std::from_chars(s.data(),s.data()+s.size(),n);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||!std::isfinite(n))throw std::runtime_error("invalid replay float");return n;}
inline std::pair<unsigned,std::string> record(std::string line){
 if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.size()>16384||line.empty()||line[0]!='[')throw std::runtime_error("invalid replay record");
 auto close=line.find("] ");if(close==line.npos)throw std::runtime_error("invalid replay timestamp");
 const auto stamp=line.substr(1,close-1);auto colon=stamp.find(':'),dot=stamp.find('.');if(colon==stamp.npos||dot==stamp.npos||dot<colon||stamp.size()-dot!=4)throw std::runtime_error("invalid replay timestamp precision");
 auto minutes=integer(std::string_view(stamp).substr(0,colon)),seconds=integer(std::string_view(stamp).substr(colon+1,dot-colon-1)),ms=integer(std::string_view(stamp).substr(dot+1));
 if(minutes>180||seconds>=60||ms>=1000)throw std::runtime_error("replay time out of bounds");return {(minutes*60+seconds)*1000+ms,line.substr(close+2)};
}
struct Frame {unsigned time=0;float x=0,y=0,z=0,angle=0,health=1,vx=0,vy=0;unsigned effects=0,team=0,model=0,max_health=0;bool max_health_live=false;std::uint16_t plane=0;unsigned weapon_type=0;float attack_interval=0,attack_modifier=1;bool dead=false;};
struct Actor {unsigned id=0,player_number=0,appearance=0,primary=0,secondary=0;std::string name;std::vector<Frame> frames;unsigned guild_id=0;std::optional<world::Instance::Npc> npc;};
struct Event {unsigned time=0,order=0;std::string source;wire::Event value;};
struct TeamGuild {unsigned guild_id=0;std::string provenance;};
struct GuildNpcModel {unsigned asset,primary,health;const char* name;};
// Archived ReplayMapData.h explicit guild NPC asset/capacity mappings.
inline std::optional<GuildNpcModel> guild_npc_model(unsigned model){
 switch(model){case 170:return GuildNpcModel{184673,1,1680,"Guild Lord"};
 case 172:return GuildNpcModel{184886,6,480,"Bodyguard"};
 case 173:return GuildNpcModel{158794,1,480,"Footman"};
 case 174:return GuildNpcModel{158794,1,480,"Knight"};
 case 175:case 176:return GuildNpcModel{184714,2,480,"Archer"};default:return {};}
}
struct Match {unsigned id=0,map=0,version=0,start=0,end=0;std::string title;std::map<unsigned,Actor> actors;std::vector<Event> events;unsigned quarantined_records=0,unresolved_surfaces=0;std::map<unsigned,guild::General> guilds;std::map<unsigned,TeamGuild> team_guilds;};
// Guild GUIDs are replay-local synthetic identifiers (GSBO, match, localId, 1),
// not captured official account/guild GUIDs. No spectator membership is issued.
inline void load_guild_metadata(Match& m,const std::filesystem::path& folder){
 auto rows=[&](const char* name,const char* expected,const auto& decode){const auto path=folder/name;if(!std::filesystem::exists(path))return;std::ifstream in(path);std::string line;if(!std::getline(in,line))throw std::runtime_error("empty observer guild metadata");if(!line.empty()&&line.back()=='\r')line.pop_back();if(line!=expected)throw std::runtime_error("unsupported observer guild metadata");unsigned count=0;while(std::getline(in,line)){if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.empty())continue;if(++count>4096||line.size()>4096)throw std::runtime_error("observer guild metadata bound");decode(presentation::split(line,'\t'));}if(in.bad())throw std::runtime_error("observer guild metadata read failed");};
 auto bounded=[](std::string_view s,unsigned limit){auto n=integer(s);if(n>limit)throw std::runtime_error("observer guild field out of bounds");return n;};
 rows("guilds.tsv","gw-observer-guilds 1",[&](const auto& f){if(f.size()!=16)throw std::runtime_error("invalid observer guild schema");guild::General g;g.local_id=static_cast<std::uint16_t>(bounded(f[0],65535));if(!g.local_id)throw std::runtime_error("zero observer guild identity");g.key={0x4f425347u,m.id,g.local_id,1};g.name=character_name_to_utf16(f[1],31);g.tag=character_name_to_utf16(f[2],4);if(g.name.empty())throw std::runtime_error("empty observer guild name");g.features=static_cast<std::uint8_t>(bounded(f[3],255));g.rank=integer(f[4]);g.rating=integer(f[5]);g.faction=static_cast<std::uint8_t>(bounded(f[6],255));g.faction_points=integer(f[7]);g.qualifier_points=integer(f[8]);g.background=static_cast<std::uint8_t>(bounded(f[9],255));g.detail_color=static_cast<std::uint8_t>(bounded(f[10],255));g.emblem_color=static_cast<std::uint8_t>(bounded(f[11],255));g.shape=static_cast<std::uint8_t>(bounded(f[12],255));g.detail=static_cast<std::uint8_t>(bounded(f[13],255));g.emblem=static_cast<std::uint16_t>(bounded(f[14],65535));g.trim=static_cast<std::uint8_t>(bounded(f[15],255));if(!m.guilds.emplace(g.local_id,std::move(g)).second)throw std::runtime_error("duplicate observer guild identity");});
 rows("affiliations.tsv","gw-observer-affiliations 1",[&](const auto& f){if(f.size()!=3)throw std::runtime_error("invalid observer affiliation schema");auto id=integer(f[0]),gid=integer(f[1]),team=integer(f[2]);auto a=m.actors.find(id);if(a==m.actors.end()||!m.guilds.contains(gid)||a->second.frames.empty()||a->second.frames.front().team!=team||a->second.guild_id)throw std::runtime_error("inconsistent observer player guild affiliation");a->second.guild_id=gid;});
 rows("team-guilds.tsv","gw-observer-team-guilds 1",[&](const auto& f){if(f.size()!=3)throw std::runtime_error("invalid observer team guild schema");auto team=bounded(f[0],65535),gid=integer(f[1]);if(!team||!m.guilds.contains(gid)||f[2].empty()||std::none_of(m.actors.begin(),m.actors.end(),[&](const auto& actor){return !actor.second.frames.empty()&&actor.second.frames.front().team==team;}))throw std::runtime_error("inconsistent observer team guild mapping");if(!m.team_guilds.emplace(team,TeamGuild{gid,std::string(f[2])}).second)throw std::runtime_error("duplicate observer team guild mapping");});
}
inline unsigned observer_asset(unsigned map){switch(map){case 168:return 127610;case 356:return 157221;case 358:return 165764;case 542:return 210082;default:return 0;}}
// Snapshot files omit floor numbers. Recover connected surfaces from the local
// DAT geometry, using recorded destination-plane values to disambiguate XY
// overlap. These are derived surfaces, not newly captured floor observations.
inline void resolve_surfaces(Match& m,const std::filesystem::path& root){
 const auto path=root/"navigation"/(std::to_string(observer_asset(m.map))+".fwnav");
 if(!std::filesystem::exists(path))throw std::runtime_error("observer layered geometry unavailable");
 const auto mesh=navigation::Mesh::load(path);const navigation::LayeredMesh graph(mesh);
 std::map<unsigned,std::vector<std::pair<unsigned,unsigned>>> hints;
 for(const auto& e:m.events)if(e.value.name=="GAME_SMSG_AGENT_MOVE_TO_POINT"&&e.value.fields.size()==4)hints[integer(e.value.fields[0])].push_back({e.time,integer(e.value.fields[3])});
 for(auto& [id,a]:m.actors){std::optional<navigation::Surface> previous;navigation::Point origin{};std::size_t hi=0;const auto& values=hints[id];unsigned hint=values.empty()?0:values.front().second;
  for(auto& f:a.frames){while(hi<values.size()&&values[hi].first<=f.time){hint=values[hi].second;++hi;}const navigation::Point point{f.x,f.y};std::optional<navigation::Surface> surface;
   if(previous)surface=graph.trace(*previous,origin,point);
   if(hint<mesh.plane_count())if(const auto cell=mesh.locate_cell(hint,point))surface=navigation::Surface{hint,*cell};
   if(!surface){std::optional<navigation::Surface> unique;unsigned found=0;for(unsigned plane=0;plane<mesh.plane_count();++plane)if(auto cell=mesh.locate_cell(plane,point)){unique=navigation::Surface{plane,*cell};++found;}if(found==1)surface=unique;}
   if(surface){f.plane=static_cast<std::uint16_t>(surface->plane);previous=surface;}else{++m.unresolved_surfaces;f.plane=previous?static_cast<std::uint16_t>(previous->plane):0;}
   origin=point;
  }
 }
}
struct CatalogEntry {unsigned id,map;std::filesystem::path folder;};
inline std::vector<CatalogEntry> catalog(const std::filesystem::path& root="data/observer"){
 std::ifstream in(root/"catalog.txt");if(!in)return {};std::string line;if(!std::getline(in,line))return {};if(!line.empty()&&line.back()=='\r')line.pop_back();if(line!="gw-observer-catalog 1")throw std::runtime_error("unsupported replay catalog");std::vector<CatalogEntry> result;
 unsigned id,map;std::string folder;while(in>>id>>map>>folder){if(!id||map>876||folder!=std::to_string(id)||result.size()>=4096)throw std::runtime_error("invalid replay catalog entry");if(std::any_of(result.begin(),result.end(),[&](auto& e){return e.id==id;}))throw std::runtime_error("duplicate replay ID");result.push_back({id,map,root/folder});}if(!in.eof())throw std::runtime_error("malformed replay catalog");return result;
}
inline Match load(const CatalogEntry& entry){
 Match m;m.id=entry.id;std::ifstream meta(entry.folder/"meta.txt");std::string header;std::getline(meta,header);if(!header.empty()&&header.back()=='\r')header.pop_back();if(header!="gw-observer-match 1"||!(meta>>m.map>>m.version)||m.map!=entry.map)throw std::runtime_error("invalid replay metadata");std::getline(meta,header);std::getline(meta,m.title);if(!m.title.empty()&&m.title.back()=='\r')m.title.pop_back();
 auto read=[&](const auto& path,const auto& fn){std::ifstream file(path);if(!file)throw std::runtime_error("missing replay file: "+path.string());std::string line;unsigned index=0;while(std::getline(file,line)){if(++index>2000000)throw std::runtime_error("replay file bound");if(line.empty()||line[0]=='#')continue;std::pair<unsigned,std::string> decoded;try{decoded=record(std::move(line));}catch(...){if(path.filename()=="unknown_events.txt"){++m.quarantined_records;continue;}throw;}fn(decoded.first,decoded.second,index);}if(file.bad())throw std::runtime_error("replay read failed");};
 auto joins=entry.folder/"StoC/player_join_events.txt";
 if(std::filesystem::exists(joins))read(joins,[&](unsigned,const std::string& s,unsigned){auto f=fields(s);if(f.size()<8||f[0]!="PLAYER_JOIN")throw std::runtime_error("invalid replay player identity");auto id=integer(f[2]);if(!id)return;if(id>65535)throw std::runtime_error("invalid replay actor ID");auto& a=m.actors[id];a.id=id;a.player_number=integer(f[1]);a.appearance=integer(f[3]);a.name=f[7];if(!a.name.empty()&&a.name.back()==')'){auto suffix=a.name.rfind(" (");if(suffix!=a.name.npos){auto slot=std::string_view(a.name).substr(suffix+2,a.name.size()-suffix-3);if(!slot.empty()&&slot.find_first_not_of("0123456789")==slot.npos)a.name.resize(suffix);}}});
 if(m.actors.empty())throw std::runtime_error("recording has no player identities");
 {std::ifstream in(entry.folder/"players.tsv");std::string line;std::getline(in,line);while(std::getline(in,line)){std::istringstream row(line);unsigned id,body,primary,secondary;if(!(row>>id>>body>>primary>>secondary)||primary>10||secondary>10)throw std::runtime_error("invalid replay professions");if(auto it=m.actors.find(id);it!=m.actors.end()){it->second.primary=primary;it->second.secondary=secondary;}}}
 const auto appearances=entry.folder/"StoC/player_appearance.txt";
 // Professions come from captured party metadata; PA describes visual assets,
 // not a replacement source for primary/secondary profession identity.
 // Resolve explicit ReplayMapData.h guild-NPC assets through captured
 // definitions. Unsupported models remain omitted. Shared visual assets
 // do not imply shared names;154 overrides the actor label after spawn.
 const auto population_path=entry.folder.parent_path().parent_path()/"gw/npcs.txt";
 std::map<unsigned,world::Instance::Npc> guild_npcs;
 if(std::ifstream population(population_path);population){
  const auto catalog=world::load_npc_catalog(population);
  for(const auto& [map,population]:catalog.maps)for(const auto& candidate:population)
   for(unsigned model:{170u,172u,173u,174u,175u,176u}){const auto mapping=*guild_npc_model(model);
    if(!guild_npcs.contains(model)&&std::find(candidate.model_files.begin(),candidate.model_files.end(),mapping.asset)!=candidate.model_files.end())guild_npcs.emplace(model,candidate);
   }
 }
 const auto lifecycle=entry.folder/"StoC/lifecycle_events.txt";
 if(std::filesystem::exists(lifecycle))read(lifecycle,[&](unsigned time,const std::string& text,unsigned){
  const auto f=fields(text);if(f.size()!=7||f[0]!="AGENT_ADD"||integer(f[3])!=1)return;
  const auto raw_model=integer(f[2]);if((raw_model&0xffff0000u)!=0x20000000u)return;
  const auto model=raw_model&65535u;const auto mapping=guild_npc_model(model);if(!mapping||!guild_npcs.contains(model))return;
  const auto id=integer(f[1]);if(!id||id>65535||m.actors.contains(id)||!std::filesystem::exists(entry.folder/"Agents"/(std::to_string(id)+".txt")))return;
  Actor actor;actor.id=id;actor.name=mapping->name;actor.primary=mapping->primary;actor.npc=guild_npcs.at(model);
  actor.npc->id=id;actor.npc->definition.id=model;actor.npc->definition.primary=static_cast<std::uint8_t>(mapping->primary);actor.npc->definition.level=20;
  // Registration can decode the source name immediately. Keep its valid
  // captured encoding until154 replaces the actor's display name.
  m.actors.emplace(id,std::move(actor));
 });
 m.start=0xffffffffu;
 for(auto& [id,a]:m.actors){
  read(entry.folder/"Agents"/(std::to_string(id)+".txt"),[&](unsigned time,const std::string& s,unsigned){auto f=fields(s);if(f.size()<33)throw std::runtime_error("snapshot too short");Frame v;v.time=time;v.x=real(f[0]);v.y=real(f[1]);v.z=real(f[2]);v.angle=real(f[3]);v.model=integer(f[5]);v.dead=integer(f[8])!=0;v.health=real(f[9]);v.max_health=integer(f[11]);if(f.size()>50)v.max_health_live=integer(f[50])!=0;v.vx=real(f[29]);v.vy=real(f[30]);v.effects=presentation::snapshot_effects(f);if(a.npc&&!v.max_health){
   // Archived LookupGuildNpcMaxHealth supplies each guild NPC capacity;
   // Deep Wound removes20%.
   // Snapshot capacity, when actually present, always takes precedence.
   const auto base=guild_npc_model(a.npc->definition.id)->health;v.max_health=(v.effects&0x20u)?base*4u/5u:base;
  }v.team=integer(f[32]);if(f.size()>35){v.weapon_type=integer(f[33]);v.attack_interval=real(f[34]);v.attack_modifier=real(f[35]);}if(v.health<0||v.health>1||v.team>255)throw std::runtime_error("invalid replay actor state");if(!a.frames.empty()&&time<a.frames.back().time)throw std::runtime_error("snapshot timestamps regressed");a.frames.push_back(v);});
  if(a.frames.empty())throw std::runtime_error("empty replay actor");m.start=std::min(m.start,a.frames.front().time);m.end=std::max(m.end,a.frames.back().time);
 }
 load_guild_metadata(m,entry.folder);
 unsigned sequence=0;std::vector<std::filesystem::path> paths;for(const auto& file:std::filesystem::directory_iterator(entry.folder/"StoC"))if(file.path().extension()==".txt")paths.push_back(file.path());std::sort(paths.begin(),paths.end());
 const auto time_limit=m.end+5000;for(const auto& path:paths)read(path,[&](unsigned time,const std::string& s,unsigned){if(time>time_limit){++m.quarantined_records;return;}auto f=fields(s);Event e{time,sequence++,path.stem().string(),{f.front(),{f.begin()+1,f.end()}}};m.events.push_back(std::move(e));m.end=std::max(m.end,time);});
 // No global packet sequence is recorded across files. This tie ordering is
 // deterministic; within each source every original line is retained.
 std::stable_sort(m.events.begin(),m.events.end(),[](auto& a,auto& b){return a.time<b.time;});resolve_surfaces(m,entry.folder.parent_path());return m;
}
struct Metrics {std::uint64_t frames=0,events=0,applied=0,unsupported=0,invalid=0,missing=0,packets=0;std::map<std::string,unsigned> failures;};
// A viewer consumes response traces. It never reruns AI, collisions, damage or
// skill effects against the live gameplay Instance and cannot mutate accounts.
class Playback {
 struct MotionPlan {unsigned start,end;float x,y,vx,vy;std::uint16_t plane;};
 std::map<unsigned,MotionPlan> motion_plans_;
 std::set<unsigned> retired_npcs_;
 const Match& match_;wire::Adapter adapter_;std::map<unsigned,std::size_t> cursor_;std::map<unsigned,unsigned> health_capacity_;std::size_t event_=0;unsigned time_=0;bool started_=false;Metrics metrics_;
 void capacity(std::vector<wire::Packet>& packets,unsigned id,const Frame& f){if(!f.max_health||health_capacity_[id]==f.max_health)return;wire::Packet packet{158,0};world::u32(packet,42);world::u32(packet,id);world::u32(packet,f.max_health);packets.push_back(std::move(packet));health_capacity_[id]=f.max_health;}
 void spawn_npc(std::vector<wire::Packet>& out,unsigned id,const Actor& a){
  const auto& f=a.frames.front();
    out.push_back(npc::properties_2022(a.npc->definition));
    out.push_back(npc::model_files_2022(a.npc->definition.id,a.npc->model_files));
    out.push_back(agent::spawn_2022(id,0x20000000u|a.npc->definition.id,f.x,f.y,f.plane,std::cos(f.angle),std::sin(f.angle),f.team));
    out.push_back(npc::name_2022(id,u"\u0108\u0107"+character_name_to_utf16(a.name,28)+u"\x0001"));
    out.push_back(combat::detail::effects(id,f.effects|(f.dead?0x10u:0u)));
    out.push_back(professions::agent_packet_2022(id,a.primary,a.secondary));
    adapter_.seed(id,{f.plane,f.health,f.effects|(f.dead?0x10u:0u),0});
    capacity(out,id,f);wire::Packet health{161,0};world::u32(health,34);world::u32(health,id);world::f32(health,f.health);out.push_back(std::move(health)); }
public:
 explicit Playback(const Match& m):match_(m),time_(m.start){}
 unsigned time()const{return time_;}bool finished()const{return started_&&time_>=match_.end;}const Metrics& metrics()const{return metrics_;}
 unsigned focus()const{for(const auto& [id,actor]:match_.actors)if(actor.player_number)return id;throw std::runtime_error("replay lacks a player focus");}
 std::vector<wire::Packet> teams()const{
  std::map<unsigned,party_ui::Team> groups;for(const auto& [id,a]:match_.actors){if(a.npc)continue;const auto token=a.frames.front().team;if(!token||!a.player_number||a.player_number>65535)throw std::runtime_error("recording lacks native team/player identity");auto& team=groups[token];team.party_id=static_cast<std::uint16_t>(token);team.token=token;team.color_key=static_cast<std::uint8_t>(token);team.encoded_name=token==1?u"Team 1":token==2?u"Team 2":u"Team";team.player_numbers.push_back(static_cast<std::uint16_t>(a.player_number));}
  std::vector<party_ui::Team> teams;for(auto& [token,team]:groups){if(auto found=match_.team_guilds.find(token);found!=match_.team_guilds.end()){team.flags|=2;team.encoded_name.clear();}teams.push_back(std::move(team));}auto packets=party_ui::publish(teams);for(const auto& [token,mapping]:match_.team_guilds)packets.push_back(party_ui::mission_guild(static_cast<std::uint16_t>(token),match_.guilds.at(mapping.guild_id)));return packets;
 }
 std::vector<wire::Packet> initialize(){
  std::vector<wire::Packet> out;for(const auto& [id,g]:match_.guilds)out.push_back(guild::general(g));for(auto& [id,a]:match_.actors){const auto& f=a.frames.front();if(a.npc){
    cursor_[id]=0;
    if(f.time<=match_.start)spawn_npc(out,id,a);
    continue;
   }
   auto identities=world::player_identity_packets_2022(id,a.name,f.x,f.y,a.appearance,f.plane,f.team);if(!a.player_number||a.player_number>65535)throw std::runtime_error("invalid captured player number");for(unsigned byte=0;byte<4;++byte)identities[0][2+byte]=static_cast<std::uint8_t>(a.player_number>>(8*byte));identities[1]=agent::spawn_2022(id,0x30000000u|a.player_number,f.x,f.y,f.plane,1,0,f.team);out.insert(out.end(),identities.begin(),identities.end());
   out.push_back(equipment::appearance_2022(id,a.appearance));if(a.guild_id)out.push_back(presentation::player_guild(id,static_cast<std::uint16_t>(a.guild_id)));if(f.team==1||f.team==2)out.push_back(presentation::team_name_color(id,f.team));if(a.primary>=1&&a.primary<=10)out.push_back(professions::agent_packet_2022(id,a.primary,a.secondary));out.push_back(wire::Adapter::facing(id,f.angle,1000));
   out.push_back(combat::detail::effects(id,f.effects|(f.dead?0x10u:0u)));adapter_.seed(id,{f.plane,f.health,f.effects|(f.dead?0x10u:0u),0});cursor_[id]=0;
   capacity(out,id,f);wire::Packet health{161,0};world::u32(health,34);world::u32(health,id);world::f32(health,f.health);out.push_back(std::move(health));
  }
  std::set<unsigned> dressed;
  for(const auto& event:match_.events)if(event.value.name=="PA"&&!event.value.fields.empty()){
   const auto actor=presentation::unsigned_value(event.value.fields[0]);
   if(actor&&match_.actors.contains(*actor)&&dressed.insert(*actor).second){auto armor=presentation::player_equipment(event.value);out.insert(out.end(),armor.packets.begin(),armor.packets.end());for(const auto& reason:armor.diagnostics)metrics_.failures["appearance: "+reason]++;}
  }
  metrics_.packets+=out.size();return out;
 }
 std::vector<wire::Packet> advance(unsigned target){
  if(target<time_||target>match_.end)throw std::runtime_error("replay clock outside range");started_=true;std::vector<wire::Packet> out;
  auto apply_event=[&](const Event& e){++metrics_.events;
   if(e.value.name=="AGENT_REMOVE"){
    const auto id=e.value.fields.size()==1?presentation::unsigned_value(e.value.fields[0]):std::nullopt;
    if(!id||!*id||*id>65535){++metrics_.invalid;metrics_.failures["AGENT_REMOVE: invalid identity"]++;return;}
    const auto actor=match_.actors.find(*id);
    if(actor==match_.actors.end()||!actor->second.npc){++metrics_.unsupported;metrics_.failures["AGENT_REMOVE: unmodeled lifecycle"]++;return;}
    retired_npcs_.insert(*id);
    if(!adapter_.state(*id)){++metrics_.missing;metrics_.failures["AGENT_REMOVE: absent NPC"]++;return;}
    wire::Packet packet{33,0};world::u32(packet,*id);out.push_back(std::move(packet));
    adapter_.erase(*id);health_capacity_.erase(*id);motion_plans_.erase(*id);++metrics_.applied;return;
   }

   if(e.value.name=="PA"){
    auto armor=presentation::player_equipment(e.value);
    for(const auto& reason:armor.diagnostics)metrics_.failures["appearance: "+reason]++;
    if(armor.packets.empty()){++metrics_.unsupported;return;}
    ++metrics_.applied;out.insert(out.end(),armor.packets.begin(),armor.packets.end());return;
   }
   // Sample trace supplies movement segments and stops, because these archives
   // do not record base speed/stop/facing messages or a complete floor state.
   if(e.value.name=="GAME_SMSG_AGENT_MOVE_TO_POINT"){++metrics_.unsupported;metrics_.failures["movement destination retained; snapshot trace owns playback"]++;return;}
   if(e.value.name=="ATTACK_STARTED"&&!e.value.fields.empty()){auto actor=presentation::unsigned_value(e.value.fields[0]);if(actor&&match_.actors.contains(*actor)&&adapter_.state(*actor)){const auto& frames=match_.actors.at(*actor).frames;auto frame=std::upper_bound(frames.begin(),frames.end(),e.time,[](unsigned time,const Frame& f){return time<f.time;});const auto& state=frame==frames.begin()?frames.front():*std::prev(frame);float interval=state.attack_interval;if(interval<=0){switch(state.weapon_type){case 2:case 4:case 7:interval=1.33f;break;case 5:case 6:interval=1.5f;break;case 3:case 8:case 9:case 10:case 11:case 12:case 13:case 14:interval=1.75f;break;}}if(interval<=0||state.attack_modifier<=0){++metrics_.unsupported;metrics_.failures["attack: recording lacks weapon subtype/timing"]++;return;}out.push_back(presentation::attack_timing(*actor,interval,state.attack_modifier));auto seeded=*adapter_.state(*actor);seeded.ranged_attack=state.weapon_type==1||state.weapon_type==6||state.weapon_type>=8;adapter_.seed(*actor,seeded);}}if((e.value.name=="DAMAGE"||e.value.name=="HEAL")&&e.value.fields.size()>1){auto target=presentation::unsigned_value(e.value.fields[1]);if(target&&match_.actors.contains(*target)&&adapter_.state(*target)){const auto& frames=match_.actors.at(*target).frames;auto frame=std::upper_bound(frames.begin(),frames.end(),e.time,[](unsigned time,const Frame& f){return time<f.time;});capacity(out,*target,frame==frames.begin()?frames.front():*std::prev(frame));}}auto r=adapter_.apply(e.value);if(r.status==wire::Status::applied){++metrics_.applied;out.insert(out.end(),r.packets.begin(),r.packets.end());}
   else {if(r.status==wire::Status::invalid)++metrics_.invalid;else if(r.status==wire::Status::missing_actor)++metrics_.missing;else ++metrics_.unsupported;metrics_.failures[e.value.name+": "+r.reason]++;}
  };
  for(;;){
   unsigned next=0xffffffffu;if(event_<match_.events.size())next=match_.events[event_].time;
   for(auto& [id,a]:match_.actors){auto at=cursor_.at(id);if(at<a.frames.size())next=std::min(next,a.frames[at].time);}
   if(next>target)break;
   if(next>time_){out.push_back(agent::movement_tick_2022(next-time_));time_=next;}
   for(const auto& [id,a]:match_.actors)if(a.npc&&!retired_npcs_.contains(id)&&!adapter_.state(id)&&a.frames.front().time==next)spawn_npc(out,id,a);
   while(event_<match_.events.size()&&match_.events[event_].time==next)apply_event(match_.events[event_++]);
   for(auto& [id,a]:match_.actors){auto& at=cursor_.at(id);while(at<a.frames.size()&&a.frames[at].time==next){const auto& f=a.frames[at];++metrics_.frames;
    if(retired_npcs_.contains(id)){++at;continue;}
    if(at){const auto& old=a.frames[at-1];const auto elapsed=f.time-old.time;const auto distance=std::hypot(f.x-old.x,f.y-old.y);if(distance>.01f&&(!elapsed||old.dead||distance*1000.f/elapsed>2000)){out.push_back(wire::Adapter::position(id,f.x,f.y,f.plane));motion_plans_.erase(id);}}
    capacity(out,id,f);auto s=*adapter_.state(id);s.plane=f.plane;s.health_fraction=f.health;s.effects=f.effects|(f.dead?0x10u:0u);auto sync=adapter_.sync(id,s);out.insert(out.end(),sync.begin(),sync.end());
    if(at+1<a.frames.size()){const auto& n=a.frames[at+1];const auto dt=n.time-f.time;const auto distance=std::hypot(n.x-f.x,n.y-f.y);
     if(dt&&distance>.01f&&!f.dead&&distance*1000.f/dt<=2000){
      bool continuing=false;
      if(const auto plan=motion_plans_.find(id);plan!=motion_plans_.end()){
       const auto& p=plan->second;
       const auto agrees=[&](const Frame& sample){const auto elapsed=float(sample.time-p.start);return std::hypot(sample.x-(p.x+p.vx*elapsed),sample.y-(p.y+p.vy*elapsed))<=.005f;};
       continuing=f.time<p.end&&n.time<=p.end&&f.plane==p.plane&&n.plane==p.plane&&agrees(f)&&agrees(n);
      }
      // Keep the native renderer moving beyond a single 100ms sample instead
      // of reaching a short goal and stopping before its next update. Extend
      // only across samples on this exact straight, constant-speed trajectory.
      if(!continuing){const Frame* goal=&n;for(std::size_t j=at+2;j<a.frames.size();++j){const auto& candidate=a.frames[j];if(candidate.plane!=f.plane||candidate.dead)break;const float ratio=float(candidate.time-f.time)/dt;if(std::hypot(candidate.x-(f.x+(n.x-f.x)*ratio),candidate.y-(f.y+(n.y-f.y)*ratio))>.005f)break;goal=&candidate;}
       out.push_back(wire::Adapter::speed(id,distance*1000.f/dt));wire::Packet p{41,0};world::u32(p,id);world::f32(p,goal->x);world::f32(p,goal->y);world::u16(p,goal->plane);world::u16(p,f.plane);out.push_back(agent::movement_speed_2022(id,1.f,1));out.push_back(std::move(p));
       motion_plans_[id]={f.time,goal->time,f.x,f.y,(n.x-f.x)/dt,(n.y-f.y)/dt,f.plane};
      }
     }else if(distance>.01f||f.dead){out.push_back(wire::Adapter::stop(id));motion_plans_.erase(id);}
     // A finite goal already stops at the recorded endpoint. Repeated stop40
     // cancels the displayed child before it arrives, leaving it short of the
     // authoritative endpoint until the next movement (observed in live reads).
    }else if(f.dead)out.push_back(wire::Adapter::stop(id));
    out.push_back(wire::Adapter::facing(id,f.angle,1000));++at;
   }}
  }
  if(target>time_)out.push_back(agent::movement_tick_2022(target-time_));time_=target;metrics_.packets+=out.size();return out;
 }
};
}









