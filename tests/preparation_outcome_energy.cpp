#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
using namespace skills;
std::string context;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Preparation outcome Energy line "+std::to_string(at.line())+context);
}
struct Fixture {
 world::Instance world;
 effects::Scope source,target;
 combat::weapon::Owned bow{12,1,{20,20,1},12,false,false,5};
 explicit Fixture(bool npc){
  if(npc){
   world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=250;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;
   d.physical_armor=60;d.physical_armor_evidence="fixture";n.combat_definition=d;world.configure_npcs({n});
  }
  world.join(1,"Ranger",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  world.set_vitals(1,{1000,1000,30,30},100);source=*world.effect_scope(1);
  check(world.energy_capacity(1,2,0,100,{10,-2}));
  if(npc)target=*world.effect_scope(100);
  else{
   world.join(2,"Target",250,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
   world.set_vitals(2,{1000,1000,30,30},100);target=*world.effect_scope(2);check(world.physical_armor(target,60));
  }
  check(world.ordinary_weapon(source,bow,20));
  check(world.status_mutation(source,430,123,12,100,18100,true));
  world.configure_attack_miss_draws([]{return .5;});
 }
 void shoot(unsigned id=394,std::uint64_t at=900){
  check(world.melee_attack(source,target,bow,*combat::weapon::attack_skill(id,0,0),at));world.tick(at+100);
 }
};
int main()try{
 // Exact gain/loss must preserve the shared precise ledger, not its displayed
 // integer projection. Debt cannot be drained further; positive fractions can.
 for(bool npc:{false,true})for(bool miss:{false,true})for(unsigned mode=0;mode<3;++mode){
  context=" npc="+std::to_string(npc)+" miss="+std::to_string(miss)+" mode="+std::to_string(mode);
  Fixture f(npc);auto& w=f.world;w.set_vitals(1,{1000,1000,mode==2?29u:0u,30},100);
  check(w.activate_overcast_energy(f.source,231,0,1,100));
  if(mode==0)check(w.energy_capacity(1,2,0,100,{0,-2}));
  if(mode==1)check(w.energy_capacity(1,2,0,100,{10,-1}));
  if(miss)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,3000));
  f.shoot();const auto snapshot=w.overcast_energy_snapshot(f.source);check(snapshot.has_value());
  const auto unit=std::int64_t(overcast::energy_unit);
  // At contact the Overcast ceiling has recovered .297 Energy over900ms.
  // Recovery opens capacity; it does not itself refill zero-pip Energy.
  const auto expected=mode==0?(miss?-10:-1)*unit:mode==1?(miss?0:9*unit+90000):miss?10*unit:20*unit+89100;
  check(overcast::precise_current(*snapshot)==expected);
  w.tick(1100); // Only one pip may add another 100ms of regeneration.
  const auto later=w.overcast_energy_snapshot(f.source);check(later.has_value());
  check(overcast::precise_current(*later)==expected+(mode==1?10000:0));
 }
 // A failed arrow's actual loss reaching zero must dispatch the existing
 // Mind Wrack observer once, including lethal reactions, in either ruleset.
 for(bool pvp:{false,true})for(bool lethal:{false,true}){
  context=" zero observer pvp="+std::to_string(pvp)+" lethal="+std::to_string(lethal);
  Fixture f(false);auto& w=f.world;w.set_vitals(1,{lethal?5u:1000u,1000,9,30},100);
  check(w.apply_mind_wrack(f.target,f.source,12,pvp,100));
  check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,3000));
  f.shoot();check(w.resources(1)->energy==0&&w.resources(1)->health==(lethal?0u:917u));
  const auto effects=w.status_snapshot(f.source,1000);check(effects.has_value());
  check(std::none_of(effects->begin(),effects->end(),[](const auto& e){return e.skill==49||e.skill==2734;}));
  w.tick(1100);check(w.resources(1)->health==(lethal?0u:917u));
 }
 context=" lethal recipient";
 {Fixture f(false);auto& w=f.world;w.set_vitals(1,{1000,1000,0,30},100);w.set_vitals(2,{1,1000,30,30},100);
  f.shoot();check(w.resources(2)->health==0&&w.resources(1)->energy==9);w.tick(1100);check(w.resources(1)->energy==9);
 }
 context=" lethal attacker reaction";
 {Fixture f(false);auto& w=f.world;w.set_vitals(1,{20,1000,20,30},100);
  check(w.apply_insidious_parasite(f.target,f.source,12,100));f.shoot();
  check(w.resources(1)->health==0&&w.resources(1)->energy==20);
 }
 context=" retired arrow source";
 {Fixture f(false);auto& w=f.world;check(w.melee_attack(f.source,f.target,f.bow,*combat::weapon::attack_skill(394,0,0),900));
  w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},950,1);w.set_vitals(1,{1000,1000,0,30},100);
  check(*w.effect_scope(1)!=f.source);w.tick(1000);check(w.resources(1)->energy==0&&w.resources(2)->health==1000);
 }
 std::cout<<"preparation outcome precise Energy lifecycle passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
