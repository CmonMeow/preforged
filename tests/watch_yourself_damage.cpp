#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
unsigned mode;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Watch damage line "+std::to_string(at.line())+" mode "+std::to_string(mode));}
int main()try{
 for(mode=0;mode<10;++mode){world::Instance w;
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
  check(w.physical_armor(target,60)&&w.lightning_armor(target,60));check(w.stage_watch_yourself_recipient(source,target,*watch_yourself(0,true),100));
  if(mode==5){check(w.degeneration_hex(foe,target,*degeneration_hex(31,12),100));w.tick(700);check(w.resources(2)->health<1000&&w.watch_yourself_armor(target,1000)==5);continue;}
  combat::weapon::Owned weapon=mode==1?combat::weapon::Owned{12,1,{20,20,4},12,false,true,26}:mode==2||mode==6?combat::weapon::Owned{8,1,{500,500,mode==6?4u:1u},0,true,false,5}:combat::weapon::Owned{12,1,{20,20,2},12,false,false,27};
  if(mode==9){weapon={1,1,{20,20,1},12,false,false,2};check(w.physical_armor(source,60));}
  if(mode==7){weapon.damage.minimum=weapon.damage.maximum=1;check(w.physical_absorption(target,3));}
  check(w.ordinary_weapon(foe,weapon,20)&&w.configure_projectiles({10000,2}));
  if(mode==9)check(w.melee_target(foe,3,100,330));
  if(mode==3){w.configure_attack_block_draws([]{return 0.;});check(w.status_mutation(target,448,77,12,100,100+*escape(448,12),true));}
  if(mode==4){w.configure_attack_miss_draws([]{return .1;});check(w.apply_condition(foe,foe,combat::conditions::Kind::blind,100,10000));}
  if(mode==2||mode==6){check(w.melee_target(foe,2,100,394));check(w.melee_attack(foe,target,weapon,*combat::weapon::attack_skill(394,12),100));}
  else if(mode==8||mode==9)check(w.melee_attack(foe,mode==9?foe:target,weapon,*combat::weapon::attack_skill(mode==8?328:330,12),100));
  else check(w.ordinary_attack(foe,2,100));
  w.tick(600);w.tick(1000);const auto health=w.resources(2)->health;
  if(mode==0||mode==1||mode==8||mode==9){if(health>=1000||w.watch_yourself_armor(target,1000)!=0)std::cerr<<"health="<<health<<" armor="<<w.watch_yourself_armor(target,1000).value_or(999)<<'\n';check(health<1000&&w.watch_yourself_armor(target,1000)==0);}
  else if(mode==2||mode==6)check(health==955&&w.watch_yourself_armor(target,1000)==5);
  else check(health==1000&&w.watch_yourself_armor(target,1000)==5);
 }
 {world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::neutral;d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;d.physical_armor=60;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Owner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto owner=*w.effect_scope(1),guard=*w.effect_scope(100);
  auto enemy=n;enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
  check(w.accept_quest_waves(owner,guard,73,1,{{enemy}},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  check(w.physical_armor(owner,60)&&w.stage_watch_yourself_recipient(owner,owner,*watch_yourself(0,true),100));
  combat::weapon::Strike strike;strike.level=20;strike.minimum=strike.maximum=20;strike.base_roll=strike.miss_roll=.5;
  check(w.damage_quest_player(attacker,owner,128,1000,strike,1000,combat::weapon::AttackDelivery::melee));
  check(w.resources(1)->health<1000&&w.watch_yourself_armor(owner,1000)==0);
 }
 std::cout<<"PASS automatic Watch Yourself ordinary physical/caster hits,physical/elemental fixed PowerShot exclusions,blocked/missed attacks and degeneration\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}








