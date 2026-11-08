#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("Vigorous NPC actions line "+std::to_string(at.line()));}
world::Instance::Npc creature(unsigned id,float x,std::optional<bool> allowed=true){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;
 d.spawn_health=combat::Health{100,500};d.damageable=d.conditions_allowed=true;d.physical_armor=60;d.physical_armor_evidence="authored fixture";
 d.knockdown_allowed=true;d.knockdown_evidence="authored fixture";d.spawn_energy=combat::npcs::Energy{0,30};d.energy_pips=0;
 d.healing_allowed=allowed;if(allowed.has_value())d.healing_evidence="authored fixture";n.combat_definition=d;return n;
}
int main()try{
 for(bool ranged:{false,true})for(bool allowed:{false,true})for(unsigned mode=0;mode<4;++mode){
  world::Instance w;w.configure_npcs({creature(100,0,allowed)});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Target",100,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(100),target=*w.effect_scope(1);check(w.elemental_armor(target,60,60)&&w.physical_armor(target,60));
  combat::ai::FlarePolicy policy;policy.source=source;policy.level=20;policy.fire_magic=12;policy.speed=300;policy.acquire=2000;policy.follow=2500;policy.leash=2500;policy.flight={100,32};policy.evidence="authored fixture";check(w.configure_npc_flare_ai(policy,1000));
  combat::ai::ordinary::Reviewed ordinary{10,10,0,ranged?5u:2u,ranged,ranged?combat::ai::ordinary::Delivery::ranged:combat::ai::ordinary::Delivery::melee,500,1500,ranged?1000.:128.,{100,32},"authored fixture"};
  check(w.configure_npc_ordinary_ai(source,ordinary,1000));check(w.stage_npc_vigorous_spirit(source,*vigorous_spirit(12),1000));
  const auto staged=*w.status_snapshot(source,1000);check(staged.size()==1&&staged.front().skill==254);
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return .0;});
  if(mode==1)check(w.blindness(target,source,*blindness(220,0),1000));
  if(mode==2)check(w.apply_health_regeneration(target,target.actor,*skills::regeneration::reviewed(258,12),1000,7000,1248,true,target));
  box->drain();w.tick(1000);check(w.npc_ordinary_pending(source).has_value());w.tick(1400);check(w.npc_combat_snapshot(source)->health->current==100);
  const auto active=*w.status_snapshot(source,1400);check(std::any_of(active.begin(),active.end(),[](auto e){return e.skill==254;}));
  if(mode==3)check(w.gale(target,source,12,{},1499));
  w.tick(1500);const double expected=100+(allowed&&mode!=3?17:0);
  if(w.npc_combat_snapshot(source)->health->current!=expected){std::cerr<<"ranged="<<ranged<<" allowed="<<allowed<<" mode="<<mode<<" health="<<w.npc_combat_snapshot(source)->health->current<<" expected="<<expected<<'\n';const auto entries=*w.status_snapshot(source,1500);for(const auto& e:entries)std::cerr<<"effect skill="<<e.skill<<" rank="<<e.rank<<" cause="<<e.cause<<'\n';}
  check(w.npc_combat_snapshot(source)->health->current==expected);
  const auto first=box->drain();const auto heal=skills::particles::health_change_2022(100,100,17,500);check(std::count(first.begin(),first.end(),heal)==unsigned(allowed&&mode!=3));
  w.tick(1500);w.tick(2000);w.tick(2500);check(w.npc_combat_snapshot(source)->health->current==expected);
  const auto later=box->drain();check(std::count(later.begin(),later.end(),heal)==0);
  const auto outcomes=w.take_attack_outcomes(source);check(outcomes.size()==unsigned(mode!=3));
  if(mode!=3)check(outcomes[0].outcome==(mode==1?world::Instance::AttackOutcome::miss:mode==2?world::Instance::AttackOutcome::block:world::Instance::AttackOutcome::hit));
 }
 {world::Instance w;w.configure_npcs({creature(100,0,std::nullopt)});check(!w.stage_npc_vigorous_spirit(*w.effect_scope(100),*vigorous_spirit(12),100));}
 for(unsigned mode=0;mode<6;++mode){
  auto enemy=creature(100,100);auto guardian=enemy;guardian.combat_definition->relation=combat::npcs::Relation::neutral;guardian.x=mode>=4?100:1000;
  world::Instance w;w.configure_npcs({guardian});w.join(1,"Owner",160,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto owner=*w.effect_scope(1),guard=*w.effect_scope(100);check(w.physical_armor(owner,60));check(w.accept_quest_waves(owner,guard,73,1,{{enemy}},[]{return true;}));
  const auto source=*w.effect_scope(60000);check(w.stage_npc_vigorous_spirit(source,*vigorous_spirit(12),100));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return .0;});
  if(mode==1)check(w.blindness(owner,source,*blindness(220,0),100));if(mode==2)check(w.apply_health_regeneration(owner,owner.actor,*skills::regeneration::reviewed(258,12),100,6100,1248,true,owner));
  combat::weapon::Strike strike;strike.level=20;strike.minimum=strike.maximum=10;strike.base_roll=strike.miss_roll=.5;
  if(mode>=4){const auto hit=w.damage_quest_guardian(source,guard,{combat::Kind::armor_ignoring,10,20,121},128,1000,mode==4?std::optional(strike):std::nullopt,1000,mode==4?combat::weapon::AttackDelivery::melee:combat::weapon::AttackDelivery::unknown);if(mode==5)check(hit.reason==combat::Reason::accepted&&hit.applied>0);}
  else w.damage_quest_player(source,owner,mode==3?10.f:128.f,1000,strike,1000,combat::weapon::AttackDelivery::melee);
  check(w.npc_combat_snapshot(source)->health->current==(mode==3||mode==5?100:117));
  check(w.take_attack_outcomes(source).size()==unsigned(mode!=3&&mode!=5));
 }
 std::cout<<"PASS Vigorous NPC self-heal,ordinary melee/ranged releases,hit/miss/block/control cancel,healing policy,quest weapon drivers and non-attack exclusion\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
