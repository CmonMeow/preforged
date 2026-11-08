#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Absorption player line "+std::to_string(at.line()));}
int main()try{
 for(bool reversal:{false,true}){
  world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{500,500,20,40},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.status_mutation(target,1399,80,12,100,6100,true));
  if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
  check(w.before_spell_effect(source,42,1,100));
  check(w.energy_drain(source,target,*skills::effects::energy_drain(42,12),20,100,nullptr,1));
  check(w.resources(2)->health==(reversal?500u:433u));
  check(w.before_spell_effect(source,42,2,101));
  check(w.energy_drain(source,target,*skills::effects::energy_drain(42,12),20,101,nullptr,2));
  check(w.resources(2)->health==(reversal?438u:371u));
 }
 for(bool reversal:{false,true}){
  world::Instance w;w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .99;});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,20,40},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.physical_armor(target,60));
  check(w.status_mutation(target,1399,80,12,100,6100,true));if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
  combat::weapon::Owned sword{12,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(source,sword,20));
  const auto attack=*combat::weapon::attack_skill(322,0);
  check(w.melee_attack(source,target,sword,attack,100));check(w.resources(2)->health==(reversal?500u:475u));
  check(w.melee_attack(source,target,sword,attack,800));check(w.resources(2)->health==(reversal?480u:455u));
 }
 for(bool reversal:{false,true}){
  const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/shield_absorption_player.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);
  auto enemies=population.for_map(146);auto guardian=enemies.front();guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  world::Instance w;w.configure_npcs({guardian});w.configure_attack_miss_draws([]{return .5;});
  w.join(2,"Owner",100,0,std::make_shared<world::Outbox>());w.set_vitals(2,{500,500,100,100},100);
  const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100);check(w.physical_armor(owner,60));
  check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  check(w.status_mutation(owner,1399,80,12,100,6100,true));if(reversal)check(w.status_mutation(owner,307,81,12,100,8100,true));
  combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=30;strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;
  const auto raw=*combat::weapon::resolve(strike);
  check(w.damage_quest_player(attacker,owner,128,1000,strike,1000));
  check(w.damage_quest_player(attacker,owner,128,2000,strike,1000));
  const double expected=500-(reversal?0:std::max(0.,raw.damage-5))-std::max(0.,raw.damage-10);
  check(w.resources(2)->health==unsigned(std::ceil(expected)));
 }
 for(bool reversal:{false,true}){
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,20,40},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.combat_armor(target,60));
  check(w.status_mutation(target,1399,80,12,100,6100,true));if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
  check(w.shockwave(source,*skills::effects::caster_earth_aoe(937,0),20,100));
  check(w.resources(2)->health==(reversal?495u:485u));
  check(w.before_spell_effect(source,42,1,101));check(w.energy_drain(source,target,*skills::effects::energy_drain(42,12),20,101,nullptr,1));
  check(w.resources(2)->health==(reversal?443u:433u));
 }
 {
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,20,40},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.combat_armor(target,60));
  check(w.status_mutation(target,1399,80,12,100,6100,true));check(w.gale(source,target,12,{},100));
  const auto plan=*skills::effects::caster_earth_aoe(174,0);check(w.shockwave(source,plan,20,100));
  const auto loss=std::max(0.,double(plan.strikes[0].earth_damage)-5)+std::max(0.,double(plan.strikes[0].knocked_down_bonus)-10);
  check(w.resources(2)->health==unsigned(500-loss));
 }
 {
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==2?1u:500u,500,20,40},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.combat_armor(target,60));
  check(w.status_mutation(target,1399,80,12,100,6100,true));check(w.status_mutation(target,307,81,12,100,8100,true));
  check(w.shockwave(source,*skills::effects::caster_earth_aoe(937,0),20,100));
  check(w.resources(2)->health==6);
 }
 {
  world::Instance w;w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .99;});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,20,40},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.physical_armor(target,60)&&w.elemental_armor(target,60,60));
  check(w.status_mutation(target,1399,80,12,100,6100,true));
  const auto prep=*combat::weapon::preparation_contact_definition(433,12);check(w.status_mutation(source,433,81,12,100,100+prep.duration,true));
  combat::weapon::Owned sword{12,1,{20,20,1},12,false,false,5};check(w.ordinary_weapon(source,sword,20));
  check(w.ordinary_attack(source,target.actor,100));w.tick(600);w.tick(700);w.cancel_ordinary_attack(source);check(w.resources(2)->health==475);
  check(w.ordinary_attack(source,target.actor,800));w.tick(1300);w.tick(1400);w.cancel_ordinary_attack(source);check(w.resources(2)->health==470);
 }
 {
  world::Instance w;check(w.configure_projectiles({100,2}));
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,20,40},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.lightning_armor(target,60));
  check(w.status_mutation(target,1399,80,12,100,6100,true));
  check(w.lightning_javelin(source,target,*skills::effects::lightning_javelin(0),20,20,100));w.tick(800);
  check(w.lightning_javelin(source,target,*skills::effects::lightning_javelin(0),20,20,801));w.tick(1500);
  const auto before=w.resources(2)->health;
  check(w.before_spell_effect(source,42,1,1501));check(w.energy_drain(source,target,*skills::effects::energy_drain(42,12),20,1501,nullptr,1));
  check(w.resources(2)->health==before-57);
 }
 // Stone Daggers has two separately delivered hits: both must consume a
 // reduction step, including when Reversal converts the first into healing.
 for(bool reversal:{false,true}){
  world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:600,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.combat_armor(target,60));
  check(w.status_mutation(target,1399,80,12,100,6100,true));
  if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
  check(w.stone_daggers(source,target,*skills::effects::stone_daggers(15),20,1000));
  w.tick(1334);check(w.resources(2)->health==(reversal?500u:472u));
  w.tick(1534);w.tick(1868);check(w.resources(2)->health==(reversal?477u:449u));
  const auto before=w.resources(2)->health;
  check(w.before_spell_effect(source,42,1,1869));check(w.energy_drain(source,target,*skills::effects::energy_drain(42,12),20,1869,nullptr,1));
  check(w.resources(2)->health==before-57);
 }
 for(bool reversal:{false,true}){
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.status_mutation(target,1399,80,12,100,6100,true));if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
  const auto plan=*skills::effects::crystal_wave(12);const auto damage=*skills::effects::crystal_wave_damage(plan,0);
  check(w.crystal_wave(source,plan,100));check(w.resources(2)->health==(reversal?500u:500u-damage+5));
  const auto before=w.resources(2)->health;check(w.crystal_wave(source,plan,101));check(w.resources(2)->health==before-damage+10);
 }
 for(unsigned skill:{1052u,3184u})for(bool reversal:{false,true}){
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.status_mutation(target,1399,80,12,100,6100,true));if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
  const auto plan=*skills::effects::wound_spell(skill,12);
  check(w.wound_spell(source,target,plan,20,100));check(w.resources(2)->health==(reversal?500u:500u-plan.damage+5));
  const auto before=w.resources(2)->health;check(w.wound_spell(source,target,plan,20,101));check(w.resources(2)->health==before-plan.damage+10);
 }
 // Two hexes enable Accumulated Pain's post-damage Deep Wound. Conversion
 // must preserve survival and condition admission without losing the hit step.
 for(unsigned skill:{1052u,3184u})for(bool reversal:{false,true}){
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.status_mutation(target,1399,80,12,100,6100,true));if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
  check(w.degeneration_hex(source,target,*skills::effects::degeneration_hex(31,0),100));
  check(w.degeneration_hex(source,target,*skills::effects::degeneration_hex(99,0),100));
  const auto plan=*skills::effects::wound_spell(skill,12);check(w.wound_spell(source,target,plan,20,100));
  check(w.resources(2)->max_health==900&&w.has_condition(target,combat::conditions::Kind::deep_wound,100));
  const auto before=w.resources(2)->health;check(w.wound_spell(source,target,plan,20,100));check(w.resources(2)->health==before-plan.damage+10);
 }
 std::cout<<"PASS player spell absorption steps and Reversal overlap\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
