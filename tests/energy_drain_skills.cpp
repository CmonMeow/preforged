#include "gw_energy_drain_skills.h"
#include "gw_combat.h"
#include <fstream>
#include <string>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("energy drain regression");}
int main(){using namespace fw::gw;using namespace skills::effects;
 check(energy_drain(39,12)->amount==8&&energy_drain(39,12)->radius==252&&energy_drain(42,12)->amount==8&&energy_drain(59,12)->amount==7&&energy_drain(59,12)->weakness_ms==10000&&energy_drain(79,12)->amount==8&&energy_drain(80,12)->amount==6);
 check(!energy_drain(39,21)&&!energy_drain(81));auto forged=*energy_drain(39);forged.radius=9999;check(!drain_energy(forged,{480,480,40,40}));
 check(energy_drain(40,12)->heal_per_energy==56);
 auto feast=drain_energy(*energy_drain(40,12),{480,480,2,40});check(feast&&feast->lost==2&&feast->healed==112&&feast->gained==0&&feast->damage==0);
 check(drain_energy(*energy_drain(40,12),{480,480,0,40})->healed==0);
 auto low=drain_energy(*energy_drain(39,12),{480,480,3,40});check(low&&low->lost==3&&low->damage==27&&low->target_energy==0);auto empty=drain_energy(*energy_drain(79,12),{480,480,0,40});check(empty&&empty->lost==0&&empty->gained==0);check(!drain_energy(*energy_drain(79,12),{480,480,41,40})&&!drain_energy(*energy_drain(79,12),{480,480,0,0}));
 auto gain=drain_energy(*energy_drain(79,12),{480,480,3,40});check(gain&&gain->gained==9);check(drain_energy(*energy_drain(80,12),{480,480,3,40})->gained==6);
 std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));const auto compare=[&](combat::Packet packet){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);};
 for(unsigned id:{39u,42u,59u,79u,80u}){auto result=drain_energy(*energy_drain(id,12),{480,480,40,40});check(bool(result));compare(vitals::packets_2022(2,{480,480,result->target_energy,40})[2]);if(result->damage){combat::Actor actor{2,1,combat::Health{480,480},{},0,true};auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,double(result->damage),20,id},true);for(auto packet:combat::damage_packets_2022(1,hit))compare(packet);}if(result->gained)compare(vitals::packets_2022(1,{480,480,10+result->gained,40})[2]);}
}
