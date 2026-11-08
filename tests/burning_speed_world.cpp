#include "gw_world_simulation.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;
static void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Burning Speed World line "+std::to_string(at.line()));
}
int main()try{
 world::Instance w;auto box=std::make_shared<world::Outbox>();
 world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=400;
 npc.definition.level=20;npc.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition definition;definition.binding.map=146;
 definition.binding.actor=definition.binding.definition=100;
 definition.binding.level=20;definition.relation=combat::npcs::Relation::hostile_to_players;
 definition.spawn_health=combat::Health{1000.,1000};
 definition.damageable=definition.conditions_allowed=true;
 npc.combat_definition=definition;w.configure_npcs({npc});
 w.join(1,"Elementalist",0,0,box,0,{}, {},100,1);
 w.set_vitals(1,{1000,1000,100,100},100);
 auto foe_box=std::make_shared<world::Outbox>();
 w.join(2,"Foe",400,0,foe_box,0,{}, {},100,2);
 w.set_vitals(2,{1000,1000,100,100},100);
 auto ally_box=std::make_shared<world::Outbox>();
 w.join(3,"Ally",400,0,ally_box,0,{}, {},100,1);
 w.set_vitals(3,{1000,1000,100,100},100);
 const auto owner=*w.effect_scope(1);
 check(!w.status_mutation(owner,823,123,15,100,7101,true));
 check(w.status_snapshot(owner,100)->empty());
 check(w.status_mutation(owner,823,123,15,100,7100,true));
 const auto active=*w.status_snapshot(owner,100);
 check(active.size()==1&&active[0].skill==823&&active[0].instance==123&&active[0].expires==7100);
 check(!w.status_mutation(owner,823,123,15,200,7200,true));
 w.move(1,10000,0,100);w.tick(1100);
 check(std::abs(w.position(1,1100).first-417.6)<1.);
 check(w.resources(1)->health<1000);
 check(w.status_mutation(owner,823,123,15,1100,{},false));
 check(w.status_snapshot(owner,1100)->empty());
 check(w.has_condition(*w.effect_scope(100),combat::conditions::Kind::burning,1100));
 w.tick(2100);
 check(w.resources(2)->health<1000&&w.resources(3)->health==1000);
 {
  world::Instance expiry;
  expiry.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  expiry.set_vitals(1,{1000,1000,100,100},100);
  expiry.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  expiry.set_vitals(2,{1000,1000,100,100},100);
  const auto scope=*expiry.effect_scope(1),target=*expiry.effect_scope(2);
  check(expiry.status_mutation(scope,823,200,15,100,7100,true));
  expiry.tick(7100);
  check(expiry.status_snapshot(scope,7100)->empty());
  check(expiry.has_condition(target,combat::conditions::Kind::burning,7100));
  check(expiry.status_mutation(scope,823,200,15,7100,{},false));
  expiry.tick(8100);check(expiry.resources(2)->health<1000);
 }
 {
  world::Instance strip;
  strip.join(1,"Enchanted",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  strip.set_vitals(1,{1000,1000,100,100},100);
  strip.join(2,"Stripper",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  strip.set_vitals(2,{1000,1000,100,100},100);
  const auto owner=*strip.effect_scope(1),stripper=*strip.effect_scope(2);
  check(strip.status_mutation(owner,823,300,15,100,7100,true));
  check(strip.enchantment_removal(stripper,owner,*skills::effects::enchantment_removal(68,0),20,1100));
  check(strip.status_snapshot(owner,1100)->empty());
  check(strip.has_condition(stripper,combat::conditions::Kind::burning,1100));
 }
 {
  world::Instance renewal;
  renewal.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  renewal.set_vitals(1,{1000,1000,100,100},100);
  renewal.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  renewal.set_vitals(2,{1000,1000,100,100},100);
  const auto scope=*renewal.effect_scope(1),target=*renewal.effect_scope(2);
  check(renewal.status_mutation(scope,823,400,15,100,7100,true));
  check(renewal.status_mutation(scope,823,401,15,1100,8100,true));
  const auto effects=*renewal.status_snapshot(scope,1100);
  check(effects.size()==1&&effects[0].instance==401);
  check(renewal.has_condition(target,combat::conditions::Kind::burning,1100));
  check(!renewal.status_mutation(scope,823,401,15,1200,8200,true));
 }
 {
  world::Instance extended;
  extended.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  extended.set_vitals(1,{1000,1000,100,100},100);
  extended.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  extended.set_vitals(2,{1000,1000,100,100},100);
  const auto scope=*extended.effect_scope(1),target=*extended.effect_scope(2);
  inventory::Profile profile;
  profile.bags.emplace(1,inventory::Bag{1,1,0,20});
  inventory::Item weapon;weapon.definition.id=100;weapon.definition.type=26;
  weapon.definition.quantity=1;weapon.definition.interaction=0x200000;
  weapon.definition.modifiers={0x22b80014u};weapon.bag=1;weapon.slot=0;
  profile.items.emplace(100,weapon);profile.weapons.sets[0]={100,0};
  const auto equipment=items::weapon_passives::equipped_enchanting(profile,7);
  check(equipment&&equipment->percent()==20);
  const auto quote=skills::effects::enchanting_duration(823,7000,scope,*equipment,{},100);
  check(quote&&quote->for_enchantment(823,7000,scope,100)==8000);
  check(!extended.status_mutation(scope,823,600,15,100,8100,true));
  check(!extended.status_mutation(scope,823,600,15,100,7100,true,{},&*quote));
  check(extended.status_mutation(scope,823,600,15,100,8100,true,{},&*quote));
  check(extended.has_condition(scope,combat::conditions::Kind::burning,7100));
  extended.tick(8100);
  check(extended.status_snapshot(scope,8100)->empty());
  check(!extended.has_condition(scope,combat::conditions::Kind::burning,8100));
  check(extended.has_condition(target,combat::conditions::Kind::burning,8100));
 }
 {
  world::Instance death;
  death.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  death.set_vitals(1,{1000,1000,100,100},100);
  death.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  death.set_vitals(2,{1000,1000,100,100},100);
  const auto scope=*death.effect_scope(1),target=*death.effect_scope(2);
  check(death.status_mutation(scope,823,500,15,100,7100,true));
  check(death.kill(scope,1100));
  check(death.status_snapshot(scope,1100)->empty());
  check(death.has_condition(target,combat::conditions::Kind::burning,1100));
 }
 std::cout<<"PASS staged Burning Speed World application, self-Burning, movement and hostile player/NPC end Burning; Runtime binding remains gated\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
