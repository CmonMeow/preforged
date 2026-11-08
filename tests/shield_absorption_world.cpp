#include "gw_world_simulation.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Absorption World line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned hp,double fire=1,std::optional<bool> healing=true){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=10;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;
  d.physical_armor=d.fire_armor=d.cold_armor=d.lightning_armor=60;d.physical_armor_evidence="synthetic weapon recipient";
  d.fire_multiplier=fire;d.fire_multiplier_evidence="synthetic vulnerability";d.healing_allowed=healing;d.healing_evidence="synthetic healing admission";
  n.combat_definition=d;w.configure_npcs({n});w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  source=*w.effect_scope(1);target=*w.effect_scope(100);w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .99;});
 }
 double hp(){return w.npc_combat_snapshot(target)->health->current;}
 bool rr(unsigned at){const auto entries=*w.status_snapshot(target,at);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
 void attack(unsigned route,unsigned raw,bool admit=true){
  const unsigned type=route==0?26:route==3?5:route==4?2:27;
  combat::weapon::Owned weapon{12,1,{20,20,raw},12,false,route==0,type};
  check(w.ordinary_weapon(source,weapon,20)&&w.configure_projectiles({100,2}));
  if(route<2){check(w.ordinary_attack(source,100,100));w.tick(600);if(!route)w.tick(700);w.cancel_ordinary_attack(source);}
  else {const auto plan=*combat::weapon::attack_skill(route==2?322:route==3?394:330,0);if(route!=4)check(w.melee_target(source,100,100,plan.skill));check(w.melee_attack(source,route==4?source:target,weapon,plan,100)==admit);if(route==3)w.tick(200);}
 }
};
int main()try{
 for(unsigned route=0;route<6;++route){
  Fixture f(1000);check(f.w.stage_npc_shield_absorption(f.target,12,100));
  check(f.w.stage_npc_reversal(f.target,12,100));
  if(route<5)f.attack(route,route==0?5u:2u);
  else check(f.w.npc_reversal_damage(f.source,f.target,{combat::Kind::armor_ignoring,50,20,42},100));
  check(f.hp()==1000);
  combat::weapon::Owned sword{12,1,{20,20,2},12,false,false,27};
  check(f.w.ordinary_weapon(f.source,sword,20));
  check(f.w.melee_attack(f.source,f.target,sword,*combat::weapon::attack_skill(322,0),800));
  check(f.hp()==980);
 }
 std::cout<<"PASS six World Absorption/Reversal routes and next-hit count\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

