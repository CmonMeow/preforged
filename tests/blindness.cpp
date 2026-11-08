#include "gw_blindness.h"
#include "gw_expertise.h"
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("blindness/Expertise regression");}
int main(){
    using namespace fw::gw::skills;using namespace effects;
    check(blindness(220,12)->duration==7000&&blindness(220,12)->range==1248&&blindness(220,12)->radius==0);
    check(blindness(424,12)->duration==13000&&blindness(424,12)->range==144&&blindness(424,12)->radius==166);
    for(unsigned rank=0;rank<=20;++rank){const auto p=*blindness(58,rank);check(p.duration==15000&&p.range==144&&p.radius==0&&p.blind_caster);}
    auto forged_midnight=*blindness(58);forged_midnight.blind_caster=false;check(!valid_blindness(forged_midnight));
    auto altered=*blindness(424,12);altered.radius=10000;check(!valid_blindness(altered)&&!blindness(220,21)&&!blindness(221,12));
    const auto& dirt=*find_skill(424);const auto& flash=*find_skill(220);const auto& touch=*find_skill(232);
    check(expertise_energy_cost(dirt,2,0)==5&&expertise_energy_cost(dirt,2,12)==3&&expertise_energy_cost(dirt,2,20)==1);
    check(expertise_energy_cost(dirt,6,20)==5&&expertise_energy_cost(flash,2,20)==10);
    check(expertise_energy_cost(touch,2,12)==3&&expertise_energy_cost(*find_skill(314),2,12)==5);
    check(expertise_energy_cost(*find_skill(446),2,12)==3);
    auto synthetic=dirt;synthetic.encoded_energy=1;check(expertise_energy_cost(synthetic,2,13)==0);
    bool rejected=false;try{expertise_energy_cost(dirt,2,21);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
}
