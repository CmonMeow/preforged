#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Ward175 World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<9;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=3;++id){w.join(id,"Actor",float((id-1)*60),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{10000,10000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
  check(w.combat_armor(target,60)&&w.elemental_armor(target,60,60)&&w.lightning_armor(target,60)&&w.physical_armor(target,60));
  const auto plan=*ward_elements_plan(0);check(w.apply_ward_elements(source,plan,100));
  if(mode==7)check(w.apply_ward_elements(source,plan,100)); // Same ward never doubles24.
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::particles::positional_effect_2022(1,900,0,0,0))==(mode==7?2:1));
  double expected=0;const unsigned at=mode==8?8100:200;const int armor=mode==8?60:84;
  if(mode==0||mode==7||mode==8){const auto p=*elemental(191,0);check(w.elemental(foe,target,p,20,at));expected=p.damage*std::exp2((60.-armor)/40.);}
  if(mode==1){const auto p=*elemental(846,0);check(w.elemental(foe,target,p,20,at));expected=p.damage*std::exp2((60.-armor)/40.);}
  if(mode==2){const auto p=*lightning(224,0);check(w.lightning(foe,target,p,20,at));expected=p.damage*std::exp2((60.-armor*.75)/40.);}
  if(mode==3){const auto p=*earth_projectile(1374,0,0);check(w.launch_projectile(foe,target.actor,p,20,at));w.tick(500);expected=p.earth_damage*std::exp2((60.-armor)/40.);}
  if(mode==4||mode==5){combat::weapon::Owned weapon{1,1,{20,20,mode==4?5u:2u},12,false,false,27};check(w.ordinary_weapon(foe,weapon,20));
   const auto p=*combat::weapon::attack_skill(322,0);check(w.melee_attack(foe,target,weapon,p,at));combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;
   strike.location_armor=mode==4?84:60;strike.base_roll=.5;strike.miss_roll=.5;expected=combat::weapon::resolve_attack_skill(strike,p,false)->damage;}
  if(mode==6){const auto p=*condition_damage(864,0);check(w.condition_damage(foe,target,p,20,at));expected=p.damage;}
  check(w.resources(2)->health==unsigned(std::ceil(10000-expected)));
 }
 // One immutable source callback covers all NPC elemental armor consumers,
 // including transaction copies and quarter-penetration calculations.
 for(auto kind:{combat::Kind::earth,combat::Kind::fire,combat::Kind::cold,combat::Kind::lightning}){
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.physical_armor=60;
  combat::npcs::LiveActor actor(d,{1,1,1,146,100});const auto life=actor.lifetime();
  actor.environmental_elemental_armor([&](auto scope,auto now){check(scope==actor.hex_scope());return now<500?24u:0u;});
  check(actor.final_earth_armor(life,100)==84&&actor.final_physical_armor(life,100)==60&&actor.final_caster_armor(life,5,100)==84);
  // PreparedHealthDamage intentionally admits armor-ignoring bursts only;
  // elemental transaction probes use the LiveActor copy, as World does.
  auto copy=actor;const auto hit=copy.apply_damage(life,{kind,100,20,1},100);
  check(hit.reason==combat::Reason::accepted&&std::abs(hit.applied-100*std::exp2(-24./40.))<.001);
  check(actor.final_caster_armor(life,5,500)==60&&actor.final_earth_armor(life,500)==60);
 }
 {world::Instance w;
  std::vector<world::Instance::Npc> npcs;
  for(unsigned id=100;id<=103;++id){world::Instance::Npc n;n.id=n.definition.id=id;n.definition.visual_adjustment={0,0,0,100};n.x=id==103?323:100;
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.spawn_health=combat::Health{1000,1000};d.damageable=true;
   d.relation=id==101?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;d.creature_type=id==102?"spirit":"human";
   d.creature_type_evidence="authored fixture";d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=60;n.combat_definition=d;npcs.push_back(n);}
  w.configure_npcs(npcs);for(unsigned id=1;id<=3;++id){w.join(id,"Actor",id==3?400:0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.apply_ward_elements(source,*ward_elements_plan(0),100));
  check(w.ward_elemental_armor(target,100)==24&&w.ward_elemental_armor(*w.effect_scope(3),100)==0);
  check(w.ward_elemental_armor(*w.effect_scope(100),100)==24);
  for(unsigned id:{101u,102u,103u})check(w.ward_elemental_armor(*w.effect_scope(id),100)==0);
  w.leave(1);w.join(1,"Replacement",400,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{1000,1000,100,100},100);
  check(w.ward_elemental_armor(target,200)==24&&w.ward_elemental_armor(*w.effect_scope(1),200)==0); // Stationary retained private lifetime policy.
  w.move(2,400,0,300);w.tick(2000);check(w.ward_elemental_armor(target,2000)==0);
  w.move(2,0,0,2100);w.tick(4000);check(w.ward_elemental_armor(target,4000)==24);
  check(w.ward_elemental_armor(target,8099)==24);
  check(w.ward_elemental_armor(target,8100)==0);
  world::Instance mixed;mixed.configure_npcs(npcs);
  for(unsigned id:{1u,2u}){mixed.join(id,"Opposed",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);mixed.set_vitals(id,{1000,1000,100,100},100);}
  const auto owner=*mixed.effect_scope(1),opponent=*mixed.effect_scope(2),ally=*mixed.effect_scope(100);
  check(mixed.apply_ward_elements(owner,*ward_elements_plan(0),100));
  check(mixed.ward_elemental_armor(owner,100)==24&&mixed.ward_elemental_armor(opponent,100)==0);
  check(mixed.ward_elemental_armor(ally,100)==0); // Unowned NPC team is ambiguous.
  check(mixed.apply_ward_elements(opponent,*ward_elements_plan(20),200));
  check(mixed.ward_elemental_armor(ally,200)==0);
  check(mixed.spawn_kill_encounter(owner,75,1,{npcs[1]},[]{return true;}));
  const std::array allies{ally};check(mixed.bind_npc_opposed_allies({owner,75,1},allies));
  check(mixed.ward_elemental_armor(ally,200)==24); // Exact quest owner supplies team.
  check(mixed.ward_elemental_armor(ally,8100)==0); // Opponent's longer ward is excluded.
 }
 std::cout<<"Ward175 World elemental spell/projectile/weapon armor, no physical/ignoring benefit, expiry/overlap and NPC authoritative query passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

