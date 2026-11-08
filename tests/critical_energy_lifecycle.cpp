#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("critical Energy lifecycle line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance world;
 effects::Scope source,target;
 combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,2,0.};
 Fixture(){
  world.join(1,"Assassin",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  world.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  world.set_vitals(1,{1000,1000,30,30},100);world.set_vitals(2,{1000,1000,30,30},100);
  source=*world.effect_scope(1);target=*world.effect_scope(2);
  check(world.physical_armor(target,60));check(world.energy_capacity(1,7,0,100,{10,-2}));
  check(world.ordinary_weapon(source,weapon,20,100,combat::weapon::CriticalProfile{7,13,20}));
  check(world.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));
  world.configure_attack_miss_draws([]{return .5;});world.configure_attack_critical_draws([]{return 0.;});
 }
 void hit(std::uint64_t at){check(world.melee_attack(source,target,weapon,*combat::weapon::attack_skill(340,12,0),at));}
};
int main()try{
 // A critical reward uses the same precise resource ledger as degeneration,
 // including debt, fractional regeneration and the reduced Overcast ceiling.
 for(unsigned mode=0;mode<3;++mode){
  Fixture f;auto& w=f.world;w.set_vitals(1,{1000,1000,mode==2?29u:0u,30},100);
  check(w.activate_overcast_energy(f.source,231,0,1,100));
  if(mode==0)check(w.energy_capacity(1,7,0,100,{0,-2}));
  if(mode==1)check(w.energy_capacity(1,7,0,100,{10,-1}));
  f.hit(mode==1?1000:100);const auto e=w.overcast_energy_snapshot(f.source);check(e.has_value());
  const auto unit=std::int64_t(overcast::energy_unit);
  check(overcast::precise_current(*e)==(mode==0?-7*unit:mode==1?3*unit+90000:20*unit));
 }
 {Fixture f;f.hit(1000);check(f.world.resources(1)->energy==30);}
 // Killing the recipient does not discard a valid living attacker's reward.
 {Fixture f;auto& w=f.world;w.set_vitals(2,{1,1000,30,30},100);check(w.consume_energy(1,10,f.source,100));
  f.hit(1000);check(w.resources(2)->health==0&&w.resources(1)->energy==23);
  w.tick(1100);check(w.resources(1)->energy==23);
 }
 // A pre-damage reaction can kill the attacker: no post-hit Energy reward.
 {Fixture f;auto& w=f.world;w.set_vitals(1,{20,1000,20,30},100);
  check(w.apply_insidious_parasite(f.target,f.source,12,100));f.hit(1000);
  check(w.resources(1)->health==0&&w.resources(1)->energy==20);
  w.tick(1100);check(w.resources(1)->energy==20);
 }
 // A released arrow cannot reward a replacement actor that reuses either ID.
 for(bool replace_source:{false,true}){
  Fixture f;auto& w=f.world;unsigned draws=0;
  w.configure_attack_critical_draws([&]{++draws;return 0.;});
  auto bow=f.weapon;bow.item_type=5;bow.damage.raw_damage_type=1;
  check(w.ordinary_weapon(f.source,bow,20,100,combat::weapon::CriticalProfile{7,13,20}));
  check(w.melee_attack(f.source,f.target,bow,*combat::weapon::attack_skill(1467,12,0),1000));
  const unsigned actor=replace_source?1u:2u;const auto retired=replace_source?f.source:f.target;
  w.leave(actor);w.join(actor,"Replacement",replace_source?0.f:60.f,0,
      std::make_shared<world::Outbox>(),0,{}, {},1010,replace_source?1:2);
  w.set_vitals(actor,{1000,1000,0,30},100);check(*w.effect_scope(actor)!=retired);
  w.tick(1100);check(draws==0&&w.resources(2)->health==1000);
  check(w.resources(1)->energy==(replace_source?0u:30u));
 }
 std::cout<<"critical Energy lifecycle passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
