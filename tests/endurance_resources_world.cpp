#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Endurance resources line "+std::to_string(at.line()));}
void buff(warrior_test::Fixture& f){check(f.world.status_mutation(f.source,374,123,12,100,29100,true));}
void hit(warrior_test::Fixture& f,std::uint64_t at){check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(383,12),at));}
int main()try{
 // Tight Overcast headroom accepts a fractional reward,not one rounded Energy.
 {warrior_test::Fixture f(383,false);buff(f);check(f.world.activate_overcast_energy(f.source,231,0,1,1000));hit(f,1501);const auto state=*f.world.overcast_energy_snapshot(f.source);check(overcast::precise_current(state)==20*overcast::energy_unit+49599);check(state.regeneration.remainder()==495&&state.fractional_tail==99);}
 // Endurance's own ceiling is tighter than the recovering Overcast ceiling.
 {warrior_test::Fixture f(383,false);buff(f);check(f.world.activate_overcast_energy(f.source,231,0,1,1000));check(f.world.energy_capacity(1,1,0,1000,{20,-1}));check(f.world.consume_energy(1,9,f.source,1000));check(f.world.settle_resources(f.source,2500));check(overcast::precise_current(*f.world.overcast_energy_snapshot(f.source))==21*overcast::energy_unit+150000);hit(f,2500);const auto state=*f.world.overcast_energy_snapshot(f.source);check(overcast::precise_current(state)==22*overcast::energy_unit&&state.regeneration.remainder()==0&&state.fractional_tail==0);}
 // Capped rewards repay real negative Energy debt when native headroom allows.
 {warrior_test::Fixture f(383,false);f.world.set_vitals(1,{1000,1000,0,30},100);check(f.world.energy_capacity(1,1,0,100,{0,-2}));buff(f);check(f.world.activate_overcast_energy(f.source,231,0,1,1000));check(overcast::precise_current(*f.world.overcast_energy_snapshot(f.source))==-10*overcast::energy_unit);hit(f,1000);const auto state=*f.world.overcast_energy_snapshot(f.source);check(state.debt==7&&overcast::precise_current(state)==-7*overcast::energy_unit);}
 // Non-Overcast players must not retain hidden fractions above the skill cap.
 {warrior_test::Fixture f(383,false);f.world.set_vitals(1,{1000,1000,21,30},100);check(f.world.energy_capacity(1,1,0,100,{10,-1}));buff(f);check(f.world.settle_resources(f.source,1600));check(!f.world.overcast_energy_snapshot(f.source));hit(f,1600);check(f.world.resources(1)->energy==22&&!f.world.overcast_energy_snapshot(f.source));check(f.world.consume_energy(1,22,f.source,1600));check(f.world.activate_overcast_energy(f.source,231,0,1,1600));check(overcast::precise_current(*f.world.overcast_energy_snapshot(f.source))==0);}
 // Blocked contact grants nothing even when there is fractional native room.
 {warrior_test::Fixture f(383,false);buff(f);check(f.world.activate_overcast_energy(f.source,231,0,1,1000));check(f.world.apply_health_regeneration(f.target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,f.target));f.world.configure_attack_block_draws([]{return 0.;});hit(f,1501);check(overcast::precise_current(*f.world.overcast_energy_snapshot(f.source))==20*overcast::energy_unit);}
 std::cout<<"PASS real World Endurance fractional Overcast/skill caps,debt repayment,non-Overcast hidden-fraction clipping and blocked fractional rewards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
