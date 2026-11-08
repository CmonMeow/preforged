#include "gw_life_drain_hexes.h"
#include "gw_health_regeneration.h"
#include "gw_skill_casting.h"
#include <fstream>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("life-drain hex regression");}
int main(){using namespace fw::gw;using namespace skills::effects;
 std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));
 const auto compare=[&](combat::Packet packet){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);};
 unsigned index=0;for(unsigned id:{109u,126u}){const auto p=*life_drain_hex(id,12);check(valid_life_drain_hex(p));check(p.pips==(id==109?3u:7u)&&p.duration==(id==109?22000u:11000u));
  compare(skills::casting::effect_add(2,id,12,0x80000000+index,p.duration/1000.f));compare(skills::casting::effect_remove(2,0x80000000+index));++index;}
 check(!life_drain_hex(0)&&!life_drain_hex(109,21));check(life_drain_hex(109)->break_range==5000&&life_drain_hex(126)->break_range==0&&life_drain_hex(126)->radius==166);
 auto forged=*life_drain_hex(109,12);forged.break_range=6000;check(!valid_life_drain_hex(forged));
 Scope scope{1,1,1};skills::regeneration::State state(scope,100);double health=100;
 check(state.apply(scope,{1,2,109,3,100,5000},false)&&state.apply(scope,{2,3,109,3,100,5000},false));check(state.pips(100)==6);
 check(state.end(scope,1,1100)&&state.advance(scope,2100,health,480)&&health==118&&state.pips(2100)==3);
 check(!state.end({1,1,2},2,2100)&&!state.end(scope,2,2000));
 check(state.apply(scope,{3,4,109,3,2100,5000},false)&&state.pips(2100)==6);
 // Default applications retain renewal semantics; independent mode is explicit.
 check(state.apply(scope,{4,5,109,2,2100,5000})&&state.pips(2100)==2);
 check(state.end(scope,4,2100)&&state.pips(2100)==0);
}
