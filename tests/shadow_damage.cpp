#include "gw_shadow_damage.h"
#include <fstream>
#include <stdexcept>
#include <string>
void check(bool ok){if(!ok)throw std::runtime_error("shadow regression");}
int main(){using namespace fw::gw;using namespace skills::effects;
 std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));
 for(unsigned id:{133u,102u,1067u}){
  const auto plan=*shadow_damage(id,12);auto result=shadow_target(plan,{100,480},{2,1,combat::Health{480,480},{},0,true},20);check(bool(result));
  auto packets=combat::damage_packets_2022(1,result->hit);if(result->steal){auto transfer=life_steal_packets(1,2,*result->steal);packets.insert(packets.end(),transfer.begin(),transfer.end());}
  for(auto packet:packets){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);}
  auto half=shadow_target(plan,{100,480},{2,1,combat::Health{240,480},{},0,true},20);check(half&&!half->steal);
  combat::Actor vulnerable{2,1,combat::Health{480,480},{},0,true};vulnerable.incoming_damage_multiplier=2;
  const auto amplified=shadow_target(plan,{100,480},vulnerable,20);
  check(amplified&&amplified->hit.damage==2*result->hit.damage);
  if(result->steal)check(amplified->steal&&amplified->steal->stolen==result->steal->stolen&&amplified->steal->gained==result->steal->gained);
  auto crossing=shadow_target(plan,{100,480},{2,1,combat::Health{250,480},{},0,true},20);check(crossing&&(id==133?!crossing->steal:bool(crossing->steal)));
  auto capped=shadow_target(plan,{479,480},{2,1,combat::Health{480,480},{},0,true},20);check(capped&&(id==133?capped->source.current==479:capped->source.current==480));
  check(!shadow_damage(id,21));
 }
 check(!shadow_damage(153));
}
