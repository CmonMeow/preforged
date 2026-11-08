#include "gw_observer_replay.h"
#include <iostream>
using namespace fw::gw;
static void require(bool v,const char* s){if(!v)throw std::runtime_error(s);}
static unsigned u32(const agent::Packet& p,unsigned at){require(at+4<=p.size(),"short packet");return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
static float f32(const agent::Packet& p,unsigned at){return std::bit_cast<float>(u32(p,at));}
struct Pose {double x=0,y=0,gx=0,gy=0;float speed=220,health=1;bool moving=false;};
struct ClientModel {
 std::map<unsigned,Pose> poses;
 void apply(const std::vector<agent::Packet>& packets){for(auto& p:packets){auto op=p[0]|(unsigned(p[1])<<8);if(op==32){auto& a=poses[u32(p,2)];a.x=f32(p,12);a.y=f32(p,16);}else if(op==30){auto dt=u32(p,2)/1000.;for(auto& [id,a]:poses)if(a.moving){auto distance=std::hypot(a.gx-a.x,a.gy-a.y);auto step=a.speed*dt;if(distance<=step){a.x=a.gx;a.y=a.gy;a.moving=false;}else{a.x+=(a.gx-a.x)*step/distance;a.y+=(a.gy-a.y)*step/distance;}}}
  else if(op==39)poses[u32(p,2)].speed=f32(p,6);else if(op==41){auto& a=poses[u32(p,2)];a.gx=f32(p,6);a.gy=f32(p,10);a.moving=true;}else if(op==40)poses[u32(p,2)].moving=false;else if(op==44){auto& a=poses[u32(p,2)];a.x=f32(p,6);a.y=f32(p,10);}else if(op==161&&u32(p,2)==34)poses[u32(p,6)].health=f32(p,10);
 }}
};
int main(int argc,char** argv)try{
 {observer::Match m;m.start=0;m.end=500;observer::Actor a;a.id=7;a.player_number=3;a.name="Motion Fixture";a.appearance=123772957;
  for(unsigned t=0;t<=500;t+=100){observer::Frame f;f.time=t;f.x=float(std::min(t,400u))/10;f.y=t>400?10.f:0.f;a.frames.push_back(f);}m.actors.emplace(7,a);
  observer::Playback replay(m);ClientModel client;client.apply(replay.initialize());auto packets=replay.advance(399);unsigned goals=0;
  for(const auto& p:packets)if(p[0]==41&&p[1]==0){++goals;require(f32(p,6)==40&&f32(p,10)==0,"straight plan crossed recorded corner");}
  require(goals==1,"straight trajectory repeatedly replaced native movement goal");client.apply(packets);require(std::abs(client.poses.at(7).x-39.9)<.001,"persistent straight goal changed trajectory");
  packets=replay.advance(500);goals=0;for(const auto& p:packets)if(p[0]==41&&p[1]==0){++goals;require(f32(p,6)==40&&f32(p,10)==10,"recorded turn destination changed");}require(goals==1,"recorded turn suppressed by active straight plan");client.apply(packets);require(std::abs(client.poses.at(7).y-10)<.001,"corner trajectory diverged");
 }
 {observer::Match m;m.start=0;m.end=1200;observer::Actor a;a.id=7;a.player_number=3;a.name="Long Straight";a.appearance=123772957;
  for(unsigned t=0;t<=1200;t+=100){observer::Frame f;f.time=t;f.x=float(std::min(t,1000u))/10;f.y=t>1000?float(t-1000)/10:0.f;a.frames.push_back(f);}m.actors.emplace(7,a);
  observer::Playback replay(m);ClientModel client;client.apply(replay.initialize());auto packets=replay.advance(999);unsigned goals=0;
  for(const auto& packet:packets)if(packet[0]==41&&packet[1]==0){++goals;require(f32(packet,6)==100&&f32(packet,10)==0,"long straight goal truncated or crossed turn");}
  require(goals==1,"verified straight trajectory split at artificial duration boundary");client.apply(packets);require(std::abs(client.poses.at(7).x-99.9)<.001,"long straight trajectory changed");
  packets=replay.advance(1200);goals=0;for(const auto& packet:packets)if(packet[0]==41&&packet[1]==0){++goals;require(f32(packet,6)==100&&f32(packet,10)==20,"long goal failed to stop at recorded turn");}require(goals==1,"recorded turn omitted after long goal");client.apply(packets);require(std::abs(client.poses.at(7).y-20)<.001,"long turn endpoint changed");
 }
 {observer::Match m;m.id=123;m.start=0;m.end=1;observer::Actor a;a.id=7;a.player_number=3;a.name="Guild Fixture";a.appearance=123772957;observer::Frame f;f.team=1;a.frames={f};m.actors.emplace(7,a);auto b=a;b.id=8;b.player_number=4;b.frames.front().team=2;m.actors.emplace(8,b);
 const std::filesystem::path folder="build/tests/observer-guild-metadata";std::filesystem::create_directories(folder);
 {std::ofstream file(folder/"guilds.tsv");file<<"gw-observer-guilds 1\n3\tRecorded Guild With Long Name\tTAG\t40\t7\t1500\t1\t12\t13\t240\t177\t101\t3\t10\t35\t2\n";}
 {std::ofstream file(folder/"affiliations.tsv");file<<"gw-observer-affiliations 1\n7\t3\t1\n";}
 {std::ofstream file(folder/"team-guilds.tsv");file<<"gw-observer-team-guilds 1\n1\t3\tfolder-exact-tag\n";}
 observer::load_guild_metadata(m,folder);require(m.actors.at(7).guild_id==3&&m.actors.at(8).guild_id==0,"captured affiliations not retained");require(m.guilds.at(3).key==guild::Key{0x4f425347u,123,3,1},"replay-local guild namespace missing");require(m.team_guilds.size()==1&&!m.team_guilds.contains(2),"uncaptured second host guild guessed");observer::Playback replay(m);auto init=replay.initialize();require(init.front()[0]==32&&init.front()[1]==1,"guild288 must precede actors");bool affiliated=false;for(const auto& packet:init){require(!(packet[0]==23&&packet[1]==1),"spectator guild membership issued");if(packet[0]==158&&u32(packet,2)==30)affiliated=u32(packet,6)==7&&u32(packet,10)==3;}require(affiliated,"native actor affiliation missing");auto teams=replay.teams();unsigned decorated=0;bool host=false;for(const auto& packet:teams){if(packet[0]==172&&packet[1]==1&&(packet[8]&2))++decorated;if(packet[0]==173&&packet[1]==1)host=packet[2]==1&&packet[3]==0;}require(decorated==1&&host,"exact mission guild decoration missing");
 {std::ofstream file(folder/"affiliations.tsv");file<<"gw-observer-affiliations 1\n7\t3\t2\n";}m.guilds.clear();m.actors.at(7).guild_id=0;m.team_guilds.clear();bool bad=false;try{observer::load_guild_metadata(m,folder);}catch(...){bad=true;}require(bad,"wrong captured team affiliation accepted");
 }
 // A resurrection/teleport must happen at its recorded time, not at the
 // preceding sample; this catches the previous early-jump presentation bug.
 {observer::Match m;m.start=0;m.end=1100;observer::Actor a;a.id=7;a.player_number=3;a.appearance=123772957;a.name="Replay Fixture";observer::Frame first;first.time=0;observer::Frame warped=first;warped.time=1000;warped.x=10000;warped.plane=4;observer::Frame next=warped;next.time=1100;next.x=10020;a.frames={first,warped,next};m.actors.emplace(7,a);observer::Playback p(m);ClientModel c;auto initial=p.initialize();require(u32(initial[0],2)==3&&u32(initial[1],6)==(0x30000000u|3),"captured player number lost in native identity");c.apply(initial);c.apply(p.advance(999));require(c.poses.at(7).x==0,"teleport played before recorded time");auto at=p.advance(1000);bool plane=false;for(const auto& packet:at)if(packet[0]==41)plane=packet[14]==4&&packet[16]==4;require(plane,"recorded floor lost in movement destination");c.apply(at);require(c.poses.at(7).x==10000,"teleport did not occur at recorded time");}
 {observer::Match m;m.start=0;m.end=1;observer::Actor a;a.id=7;a.player_number=3;a.appearance=123772957;a.name="Attack Fixture";observer::Frame f;f.weapon_type=13;a.frames={f};m.actors.emplace(7,a);observer::Event e;e.time=1;e.value.name="ATTACK_STARTED";e.value.fields={"7","7"};m.events.push_back(e);observer::Playback replay(m);replay.initialize();auto packets=replay.advance(1);bool timing=false,attack=false;for(const auto& packet:packets){if(packet[0]==53)timing=true;if(packet[0]==159&&u32(packet,2)==4){require(timing,"attack emitted before native interval initialization");attack=true;}}require(attack,"known weapon attack omitted");m.actors.at(7).frames.front().weapon_type=1;observer::Playback bow(m);bow.initialize();auto ambiguous=bow.advance(1);for(const auto& packet:ambiguous)require(!(packet[0]==159&&u32(packet,2)==4),"ambiguous bow attack could assert without interval");} {observer::Match m;m.start=0;m.end=1;observer::Actor a;a.id=7;a.player_number=3;a.appearance=123772957;a.name="Health Fixture";observer::Frame unknown;unknown.team=1;observer::Frame known=unknown;known.time=1;known.max_health=600;a.frames={unknown,known};m.actors.emplace(7,a);observer::Event event;event.time=1;event.value.name="DAMAGE";event.value.fields={"0","7","-0.1","55"};m.events={event};observer::Playback replay(m);auto initial=replay.initialize();bool initial_effects=false;for(const auto& packet:initial)if(packet[0]==240&&u32(packet,2)==7&&u32(packet,6)==0)initial_effects=true;require(initial_effects,"initial zero effects not published");bool color=false;for(const auto& packet:initial)if(packet[0]==158&&u32(packet,2)==65)color=u32(packet,10)==1;require(color,"observer team name color omitted");auto packets=replay.advance(1);bool capacity=false,damage=false;for(const auto& packet:packets){if(packet[0]==158&&u32(packet,2)==42)capacity=u32(packet,10)==600;if(packet[0]==162){require(capacity,"damage displayed before captured HP capacity");damage=true;}}require(damage,"health fixture damage missing");} require(observer::record("[01:23.456] A;1").first==83456,"timestamp units");for(auto s:{"[00:60.000] bad","[00:00.1] bad","[00:00.000]x"}){bool fail=false;try{observer::record(s);}catch(...){fail=true;}require(fail,"malformed timestamp accepted");}
 auto entries=observer::catalog(argc==2?argv[1]:"data/observer");require(!entries.empty(),"no replay corpus");double worst=0;std::uint64_t checked=0;
 for(auto& entry:entries){auto m=observer::load(entry);
  if(entry.id==6){
   for(const auto [id,model]:{std::pair{29u,174u},{30u,176u},{31u,172u},{32u,176u},{40u,174u},{45u,176u}}){
    const auto& npc=m.actors.at(id);require(npc.npc&&npc.npc->definition.id==model&&!npc.player_number,"captured guild defender omitted");
    require(npc.frames.front().max_health==480,"defender inherited lord capacity");
   }
   for(unsigned id:{27u,28u}){const auto& lord=m.actors.at(id);require(lord.npc&&lord.npc->definition.id==170&&lord.player_number==0,"recorded guild lord missing or treated as player");require(lord.frames.front().max_health==1680,"guild lord health capacity unavailable");}
   {observer::Match late;late.start=0;late.end=200;
    auto lord=m.actors.at(27);lord.frames.resize(2);lord.frames[0].time=100;lord.frames[1].time=200;lord.frames[0].attack_interval=1.33f;
    late.actors.emplace(27,lord);
    observer::Event attack;attack.time=25;attack.value.name="ATTACK_STARTED";attack.value.fields={"27","27"};late.events.push_back(attack);
    observer::Event early;early.time=50;early.value.name="DAMAGE";early.value.fields={"0","27","-0.1","55"};late.events.push_back(early);
    observer::Event arrived=early;arrived.time=100;late.events.push_back(arrived);
    observer::Event removal;removal.time=150;removal.value.name="AGENT_REMOVE";removal.value.fields={"27"};late.events.push_back(removal);
    auto after=attack;after.time=175;late.events.push_back(after);
    observer::Playback arrival(late);require(arrival.initialize().empty(),"future NPC spawned before first observation");
    const auto before=arrival.advance(99);for(const auto& packet:before)require(packet[0]==30&&packet[1]==0,"unspawned NPC received state publication");
    require(arrival.metrics().missing==2,"pre-arrival combat did not remain missing actor");
    const auto at=arrival.advance(100);unsigned births=0;bool registered=false,spawned=false,hit=false;
    for(const auto& packet:at){const auto op=packet[0]|(unsigned(packet[1])<<8);if(op==85)registered=true;if(op==32){require(registered&&u32(packet,2)==27,"arrival precedes NPC definition");spawned=true;++births;}if(op==162){require(spawned,"same-time combat precedes NPC arrival");hit=true;}}
    require(births==1&&hit,"timed NPC arrival/combat missing");
    for(const auto& packet:arrival.advance(149))require(!(packet[0]==33&&packet[1]==0),"NPC removed before recorded departure");
    unsigned removals=0;for(const auto& packet:arrival.advance(150))if(packet[0]==33&&packet[1]==0){require(u32(packet,2)==27,"wrong NPC retired");++removals;}
    require(removals==1,"recorded NPC departure missing or duplicated");
    for(const auto& packet:arrival.advance(200))require(packet[0]==30&&packet[1]==0,"retired NPC received packets or respawned");
    require(arrival.metrics().missing==3&&arrival.metrics().frames==2,"post-departure combat/frame accounting incorrect");
   }
   observer::Playback fixture(m);require(fixture.focus()!=27&&fixture.focus()!=28,"NPC selected as observer player focus");
   auto initial=fixture.initialize();bool definition=false;unsigned spawned=0;
   for(const auto& packet:initial){const auto op=packet[0]|(unsigned(packet[1])<<8);if(op==85&&u32(packet,2)==170)definition=true;if(op==32&&(u32(packet,2)==27||u32(packet,2)==28)){require(definition&&u32(packet,6)==0x200000aau,"lord spawned before definition or with player appearance");++spawned;}}
   require(spawned==2,"both captured guild lords not published");
   std::set<unsigned> registered,named;
   for(const auto& packet:initial){const auto op=packet[0]|(unsigned(packet[1])<<8);
    if(op==85)require(packet.at(28)||packet.at(29),"NPC registration discarded valid encoded source name");
    if(op==32)registered.insert(u32(packet,2));
    if(op==154){const auto id=u32(packet,2);require(registered.contains(id)&&m.actors.at(id).npc.has_value(),"NPC name addressed definition ID or preceded actor spawn");named.insert(id);}
   }
   for(const auto& [id,a]:m.actors)if(a.npc)require(named.contains(id),"guild defender inherited source catalog label");
   fixture.teams();
   auto growth=fixture.advance(91057);bool blue_bridge=false;
   for(const auto& packet:growth)if(packet.size()==9&&packet[0]==13&&packet[1]==1&&packet[2]==38&&packet[3]==200&&packet[4]==16)blue_bridge=true;
   require(blue_bridge,"match6 recorded vine growth absent from replay timeline");
  }
  observer::Playback replay(m);ClientModel client;client.apply(replay.initialize());std::map<unsigned,std::size_t> at;
  for(unsigned time=m.start;;time=std::min(m.end,time+47)){client.apply(replay.advance(time));for(auto& [id,a]:m.actors){auto& i=at[id];while(i+1<a.frames.size()&&a.frames[i+1].time<=time)++i;const auto& f=a.frames[i];if(f.time>time)continue;double x=f.x,y=f.y;if(i+1<a.frames.size()){const auto& n=a.frames[i+1];const auto dt=n.time-f.time;auto distance=std::hypot(n.x-f.x,n.y-f.y);if(dt&&distance>.01&&!f.dead&&distance*1000./dt<=2000){auto alpha=std::clamp(double(time-f.time)/dt,0.,1.);x+=(n.x-f.x)*alpha;y+=(n.y-f.y)*alpha;}}
    auto& p=client.poses.at(id);auto error=std::hypot(p.x-x,p.y-y);worst=std::max(worst,error);if(error>=.1)std::cerr<<"match="<<m.id<<" actor="<<id<<" time="<<time<<" source="<<x<<","<<y<<" replay="<<p.x<<","<<p.y<<" error="<<error<<"\n";require(error<.1,"serialized movement diverged from source trajectory");require(std::isfinite(p.health)&&p.health>=0&&p.health<=1,"invalid serialized health");if(f.time==time)require(std::abs(p.health-f.health)<.00001,"serialized health diverged at source checkpoint");++checked;
   }if(time==m.end)break;}
  require(replay.finished(),"match did not complete");require(replay.metrics().events==m.events.size(),"events lost");std::uint64_t samples=0;for(auto& [id,a]:m.actors)samples+=a.frames.size();require(samples==replay.metrics().frames,"samples lost");
  bool fail=false;try{replay.advance(m.start);}catch(...){fail=true;}require(fail,"clock regressed");
  std::cout<<"match="<<m.id<<" map="<<m.map<<" actors="<<m.actors.size()<<" unresolved_surfaces="<<m.unresolved_surfaces<<" samples="<<samples<<" events="<<m.events.size()<<" adapted="<<replay.metrics().applied<<" unsupported="<<replay.metrics().unsupported<<" invalid="<<replay.metrics().invalid<<" missing="<<replay.metrics().missing<<"\n";
 }
 std::cout<<"PASS checkpoints="<<checked<<" worst_position_error="<<worst<<" game_units (packet consumer model; live client separately required)\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}







