#include "../tools/titles/capture_import.h"
#include <sstream>
#include <iostream>
void check(bool value,const char* label){if(!value)throw std::runtime_error(label);}
template<class F>void rejected(F fn){bool caught=false;try{fn();}catch(const std::exception&){caught=true;}check(caught,"invalid import accepted");}
std::string snapshot(){
 std::ostringstream out;
 out<<R"({"schema":1,"event":"progression_snapshot","character_guid":"12345678-1234-5678-9ABC-123456789ABC","stats_stride8":[1,2,3,4,5,6,7,8,9,20,100,10,11,12,13],"faction_caps_kurzick_luxon_balthazar_imperial":[10000,10000,10000,20000],"title_tiers":[{"index":1,"props":0,"number":1,"name_encoded_utf16":[16676]},{"index":2,"props":0,"number":4294967295,"name_encoded_utf16":[257]}],"titles_modern44":[)";
 for(unsigned i=0;i<48;++i){if(i)out<<',';out<<"{\"index\":"<<i<<",\"fields\":"<<(i?"[0,0,0,0,0,0,0,0,0]":"[4,25,1,25,0,2,4294967295,15,3]")<<",\"progress_encoded_utf16\":"<<(i?"[]":"[16677]")<<",\"hint_encoded_utf16\":[]}";}
 out<<"]}";return out.str();
}
int main()try{
 const auto raw=snapshot();auto row=capture::Parser(raw).parse();const auto profile=capture::progression::convert(row);
 check(profile.character.tracks.size()==1&&profile.character.ranks.size()==2,"preserve populated tracks/ranks");
 check(profile.character.tracks.at(0).fields==std::array<std::uint32_t,8>{4,25,1,25,2,UINT32_MAX,15,3},"drop only modern extension");
 check(profile.factions.counters[0].current==2&&profile.factions.counters[1].total==5&&profile.factions.counters[2].current==10&&profile.factions.counters[3].total==7,"faction native order conversion");
 check(profile.character.hero_points==8&&profile.character.skill_points==12,"native currency counters");
 auto bad=row;bad.members["titles_modern44"].items[0].members["fields"].items[4].number=1;rejected([&]{capture::progression::convert(bad);});
 bad=row;bad.members["titles_modern44"].items[1].members["index"].number=0;rejected([&]{capture::progression::convert(bad);});
 bad=row;bad.members["title_tiers"].items[0].members["name_encoded_utf16"].items[0].number=0;rejected([&]{capture::progression::convert(bad);});
 const auto selected=*fw::gw::parse_token("12345678-1234-5678-9ABC-123456789ABC");
 const std::string start=R"({"event":"start","schema":4,"client_sha256":"8e50edfb83515fabfec50bfd9d17968e8635a39b4c3e796e311bbc5cd15e5878"})";
 std::istringstream valid(start+"\n"+raw+"\n");check(capture::progression::read(valid,selected).character.tracks.size()==1,"provenance/character join");
 std::istringstream missing(raw+"\n");rejected([&]{capture::progression::read(missing,selected);});
 std::istringstream wrong(start+"\n"+raw+"\n");rejected([&]{capture::progression::read(wrong,*fw::gw::parse_token("22345678-1234-5678-9ABC-123456789ABC"));});
 std::string changed=raw;const auto pos=changed.find("[4,25,1,25");changed.replace(pos,10,"[4,26,1,25");
 std::istringstream unstable(start+"\n"+raw+"\n"+changed+"\n");rejected([&]{capture::progression::read(unstable,selected);});
 std::cout<<"PASS modern-to2022 title/faction capture conversion, text, provenance, identity, extension and changing-snapshot rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
