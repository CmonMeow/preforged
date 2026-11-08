#include "gw_pet_default_ai.h"
#include "gw_pet_combat.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace fw::gw::pets;
using namespace fw::gw::pets::combat_model;
void check(bool value){if(!value)throw std::runtime_error("pet combat policy failed");}
void near(double a,double b){check(std::abs(a-b)<1e-9);}
int main()try{
    const auto defaults=default_stalker_ai({1,100,4},Pet{});check(bool(defaults));
    check(defaults->ordinary.period_ms==2000&&defaults->ordinary.windup_ms==900&&defaults->ordinary.damage_type==2);
    check(defaults->motion.speed==360&&!defaults->motion.proximity_acquisition&&defaults->motion.valid()&&defaults->ordinary.valid());
    auto evolved=Pet{};evolved.evolution=Evolution::dire;check(!default_stalker_ai({1,100,4},evolved));

    const Owned owned{{1,100,4},{1,1,2},true};
    check(owned.valid());
    near(*incoming_multiplier(owned,owned.pet,Loss::direct_damage),.67);
    for(auto loss:{Loss::degeneration,Loss::health_loss,Loss::life_steal})near(*incoming_multiplier(owned,owned.pet,loss),1.);
    auto stale=owned.pet;++stale.generation;check(!incoming_multiplier(owned,stale,Loss::direct_damage));
    auto other=owned;other.owner.instance=2;check(!incoming_multiplier(other,owned.pet,Loss::direct_damage));
    auto pvp=owned;pvp.pve=false;near(*incoming_multiplier(pvp,pvp.pet,Loss::direct_damage),1.);
    Strike s{owned,owned.pet,Pet{},20,12,60,.5};s.profile.level=20;
    near(*ordinary(s),23.*1.33);
    s.profile.level=5;near(*ordinary(s),23.*.25*1.33);
    s.beast_mastery=0;near(*ordinary(s),23.*.25*std::exp2(-1.5)*1.33);
    s.owner_level=1;s.beast_mastery=12;near(*ordinary(s),23.*.25*std::exp2((30.-60.)/40.)*1.33);
    s.owner_level=20;s.beast_mastery=12;s.critical=true;s.roll=0;
    near(*ordinary(s),17.*.25*std::sqrt(2.)*1.33);
    s.roll=.5;near(*ordinary(s),23.*.25*std::sqrt(2.)*1.33);
    s.critical_immune=true;near(*ordinary(s),23.*.25*1.33);
    s.weakness=true;near(*ordinary(s),23.*.25*.34*1.33);
    s.attacker=stale;check(!ordinary(s));s.attacker=owned.pet;
    s.profile.evolution=Evolution::dire;check(!ordinary(s));s.profile.evolution=Evolution::unevolved;
    s.roll=1;check(!ordinary(s));s.roll=0;s.owner_level=0;check(!ordinary(s));
    std::cout<<"Scoped pet reviewed scaling, variable criticals and PvE direct-only modifiers passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
