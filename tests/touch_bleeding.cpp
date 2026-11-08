#include "gw_shadow_damage.h"
#include "gw_skill_casting.h"
#include <fstream>
#include <stdexcept>
#include <string>
void check(bool ok){if(!ok)throw std::runtime_error("touch Bleeding regression");}
int main(){using namespace fw::gw;using namespace skills::effects;std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));
 const auto compare=[&](combat::Packet packet){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);};
 for(unsigned id:{158u,1078u}){const auto plan=*shadow_damage(id,12);check(plan.range==144&&plan.damage==50&&!plan.steal_above_half);auto result=shadow_target(plan,{100,480},{2,1,combat::Health{480,480},{},0,true},20);check(bool(result));for(const auto& p:combat::damage_packets_2022(1,result->hit))compare(p);}
 compare(skills::casting::effect_add(2,478,0,0x80000001,13.f));
}
