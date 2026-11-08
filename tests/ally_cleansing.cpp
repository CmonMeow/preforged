#include "gw_ally_cleansing.h"
#include "gw_condition_removal.h"
#include <stdexcept>
#include "gw_skill_casting.h"
#include "gw_conditions.h"
#include <fstream>
#include <string>
void check(bool ok){if(!ok)throw std::runtime_error("ally cleansing regression");}
int main(){using namespace fw::gw::skills::effects;
 std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));std::vector<fw::gw::combat::Packet> packets;
 for(unsigned effect:{0x80000001u,0x80000002u})packets.push_back(fw::gw::skills::casting::effect_remove(2,effect));packets.push_back(fw::gw::skills::casting::effect_add(1,478,0,0x80000001,10.f));packets.push_back(fw::gw::skills::casting::effect_add(1,479,0,0x80000002,5.f));for(auto [actor,health]:{std::pair{1u,144u},std::pair{2u,138u},std::pair{2u,201u}})packets.push_back(fw::gw::combat::detail::fraction(actor,{double(health),480}));
 for(auto packet:packets){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);}
 // All ten real application records must retain their IDs and float durations
 // through our writer, independently compared with the retail serializer.
 std::ifstream conditions(CONDITION_FIXTURE_FILE);check(bool(conditions));
 unsigned actor,skill,rank,effect,count=0;float duration;std::string wire;
 std::vector<unsigned> seen;
 while(conditions>>actor>>skill>>rank>>effect>>duration>>wire){
  auto packet=fw::gw::skills::casting::effect_add(actor,skill,rank,effect,duration);packet[1]|=0x80;
  std::vector<unsigned char> expected;for(std::size_t at=0;at<wire.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(wire.substr(at,2),nullptr,16)));check(packet==expected);
  check(std::find(seen.begin(),seen.end(),skill)==seen.end());seen.push_back(skill);
  check(std::find(fw::gw::combat::conditions::skills.begin(),fw::gw::combat::conditions::skills.end(),skill)!=fw::gw::combat::conditions::skills.end());++count;
 }check(count==10);
 auto draw=*ally_cleansing(311,12,12);check(draw.draw&&!draw.self&&draw.health_gain==22&&draw.divine_heal==38&&valid_ally_cleansing(draw));auto purge=*ally_cleansing(295);check(!purge.draw&&purge.self&&purge.divine_heal==0);
 draw.health_gain=999;check(!valid_ally_cleansing(draw));check(!ally_cleansing(311,21)&&!ally_cleansing(301));auto dismiss=*condition_removal(1691,12,12);check(dismiss.self&&dismiss.count==1&&dismiss.per_condition_heal==63&&condition_heal(dismiss,0,0,true)==101&&condition_heal(dismiss,0,0,false)==38);
}
