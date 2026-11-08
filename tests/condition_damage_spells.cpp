#include "gw_condition_damage_spells.h"
#include "gw_skill_casting.h"
#include <fstream>
#include <stdexcept>
void check(bool value){if(!value)throw std::runtime_error("condition damage regression");}
int main(){using namespace fw::gw;using namespace skills::effects;
 std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));
 const auto compare=[&](combat::Packet packet){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);};
 for(unsigned id:{828u,864u}){const auto plan=*condition_damage(id,12);check(valid_condition_damage(plan)&&plan.damage==(id==828?54u:26u)&&plan.duration==10000);
  combat::Actor actor{2,1,combat::Health{480,480},{},0,true,{},{},60};const auto hit=combat::apply(actor,{plan.kind,double(plan.damage),20,id},true);check(hit.reason==combat::Reason::accepted);
  for(const auto& packet:combat::damage_packets_2022(1,hit))compare(packet);
  if(id==828){compare(skills::casting::effect_add(2,id,12,0x80000000,10));compare(skills::casting::effect_remove(2,0x80000000));check(plan.pips==4&&plan.radius==0);}
  else {unsigned index=0;for(unsigned condition:{484u,486u}){compare(skills::casting::effect_add(2,condition,0,0x80000000+index,10));compare(skills::casting::effect_remove(2,0x80000000+index));++index;}check(plan.pips==0&&plan.radius==166);}
 }
 auto forged=*condition_damage(864,12);forged.kind=combat::Kind::cold;check(!valid_condition_damage(forged));check(!condition_damage(0)&&!condition_damage(864,21));
 check(condition_damage(828,0)->damage==10&&condition_damage(828,20)->damage==83&&condition_damage(828,20)->pips==6);
}
