#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Spiteful NPC weapon line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<9;++mode){
  world::Instance::Npc enemy;enemy.id=enemy.definition.id=100;enemy.x=100;enemy.definition.level=8;enemy.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=8;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{mode==5?10.:500.,500};d.damageable=d.conditions_allowed=true;
  d.physical_armor=60;d.physical_armor_evidence="authored fixture";enemy.combat_definition=d;
  auto guardian=enemy;guardian.x=(mode==7||mode==8)?100:1000;guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  world::Instance w;w.configure_npcs({guardian});w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return .1;});
  auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Player",160,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{1000,1000,100,100},100);check(w.physical_armor(*w.effect_scope(id),60));}
  const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2),guard=*w.effect_scope(100);
  check(w.accept_quest_waves(owner,guard,73,1,{{enemy}},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  if(mode!=6)check(w.spiteful_spirit(caster,attacker,*spiteful_spirit(12),20,100));
  if(mode==1)check(w.blindness(caster,attacker,*blindness(220,0),100));
  if(mode==2)check(w.apply_health_regeneration(owner,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,owner));
  combat::weapon::Strike strike;strike.level=8;strike.minimum=strike.maximum=10;strike.base_roll=strike.miss_roll=.5;
  using combat::weapon::AttackDelivery;box->drain();
  const auto delivery=mode==3?AttackDelivery::ranged:AttackDelivery::melee;
  if(mode==8)check(w.damage_quest_guardian(attacker,guard,{},128,1000,strike,1000,delivery).reason==combat::Reason::accepted);
  else w.damage_quest_player(attacker,owner,mode==4?10.f:128.f,1000,strike,1000,delivery);
  const bool released=mode!=3&&mode!=4;const bool pulse=released&&mode!=6&&mode!=7&&mode!=8;
  check(w.npc_combat_snapshot(attacker)->health->current==(mode==5?0.:pulse?471.:500.));
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==unsigned(released));
  if(released)check(events.front().outcome==(mode==1?combat::weapon::AttackOutcome::miss:mode==2?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,attacker.actor,121))==unsigned(pulse));
  const auto victim_health=w.resources(2)->health;
  check((!released||mode==1||mode==2||mode==8)?victim_health==1000:victim_health<1000);
  if(mode==8)check(w.npc_combat_snapshot(guard)->health->current<500);
  check(w.take_npc_deaths(caster).size()==unsigned(mode==5));
  if(mode==5){w.damage_quest_player(attacker,owner,128,1100,strike,1000,delivery);check(w.take_npc_deaths(caster).empty()&&w.resources(2)->health==victim_health);}
 }
 std::cout<<"PASS NPC single-strike Spiteful hit/miss/block,invalid delivery/range,no hex,unknown alliance rejection,lethal post-hit reaction and once-only credit\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
