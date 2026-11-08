#include "gw_elemental_damage.h"
#include "gw_fire_storm.h"
#include <fstream>
#include <string>
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("elemental spell regression");}
int main(){using namespace fw::gw::skills::effects;using namespace fw::gw::combat;using conditions::Kind;
 check(fire_storm(0)->damage==5&&fire_storm(12)->damage==29&&fire_storm(15)->damage==35&&!fire_storm(21));
 auto storm=FireStormField::create({1,2,3},20,10,20,0,*fire_storm(12),100);
 check(storm.has_value()&&storm->advance(1099).empty());
 auto pulse=storm->advance(1100);check(pulse.size()==1&&pulse[0].at==1100&&pulse[0].x==10&&pulse[0].y==20);
 check(storm->advance(1000).empty()&&storm->advance(1100).empty());
 auto remaining=storm->advance(20000);check(remaining.size()==9&&remaining.back().at==10100&&storm->finished()&&storm->advance(30000).empty());
 auto bad_storm=*fire_storm(12);bad_storm.damage=1000;
 check(!FireStormField::create({1,2,3},20,10,20,0,bad_storm,100)&&
       !FireStormField::create({1,2,3},20,10,20,0,*fire_storm(12),UINT64_MAX-9999));
 check(elemental(183,12)->damage==114&&elemental(186,12)->damage==91&&elemental(189,12)->damage==99&&elemental(191,12)->damage==64&&elemental(1379,12,12)->energy_if_burning==11&&elemental(1095,12)->radius==322&&elemental(846,12)->blind_if_burning_ms==9000&&elemental(89,12)->shadow_above_half==41);
 check(elemental(189,12)->radius==240);
 check(elemental(194,0)->damage==20&&elemental(194,12)->damage==56&&elemental(194,15)->damage==65&&elemental(194)->projectile);
 check(!elemental(193)&&!elemental(214)&&!elemental(183,21)&&!elemental(183,12,21));auto forged=*elemental(189);forged.radius=252;check(!valid_elemental(forged));
 Actor target{2,5,Health{480,480},{},0,true,{},60,60};conditions::State burning;burning.apply(Kind::burning,1,0,10000);
 auto gaze=elemental_target(*elemental(1379,12,12),target,burning,20,1,100);check(gaze.accepted&&gaze.energy==11&&gaze.hits[0].damage==41);check(elemental_target(*elemental(1379,12,12),target,{},20,1,100).energy==0);
 auto steam=elemental_target(*elemental(846,12),target,burning,20,1,100);check(steam.accepted&&steam.conditions.has(Kind::blind,9099)&&!steam.conditions.has(Kind::blind,9100)&&steam.hits[0].damage==52);check(elemental_target(*elemental(846,12),target,{},20,1,100).additions.empty());
 auto immolate=elemental_target(*elemental(191,12),target,{},20,1,100);check(immolate.accepted&&immolate.conditions.has(Kind::burning,3099));
 auto star=elemental_target(*elemental(1095,12),target,{},20,1,100);check(star.energy==2&&star.conditions.has(Kind::burning,3099));
 target.fire_armor.reset();check(!elemental_target(*elemental(191,12),target,{},20,1,100).accepted);target.fire_armor=100;check(elemental_target(*elemental(191,12),target,{},20,1,100).hits[0].damage==32);
 target.cold_armor=100;auto chill=elemental_target(*elemental(89,12),target,{},20,1,100);check(chill.hits.size()==2&&chill.hits[0].damage==20.5&&chill.hits[1].damage==41);
 target.health=Health{240,480};check(elemental_target(*elemental(89,12),target,{},20,1,100).hits.size()==1);target.health=Health{241,480};check(elemental_target(*elemental(89,12),target,{},20,1,100).hits.size()==2);
 target.health=Health{1,480};auto killed=elemental_target(*elemental(191,12),target,{},20,1,100);check(killed.hits[0].killed&&killed.additions.empty());
 std::ifstream fixture(FIXTURE_FILE);check(bool(fixture));target={2,5,Health{480,480},{},0,true,{},60,60};for(unsigned id:{183u,186u,189u,191u,1379u,1095u,846u,89u}){const auto result=elemental_target(*elemental(id,12),target,{},20,1,100);check(result.accepted);for(const auto& hit:result.hits)for(auto packet:damage_packets_2022(1,hit)){std::string line;check(bool(std::getline(fixture,line)));std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected);}}
}
