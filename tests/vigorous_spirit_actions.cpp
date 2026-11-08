#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("Vigorous actions line "+std::to_string(at.line()));}
int main()try{
 for(unsigned kind=0;kind<3;++kind)for(unsigned mode=0;mode<4;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Attacker",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);check(w.physical_armor(foe,60)&&w.combat_armor(foe,60));
  check(w.complete_vigorous_spirit(owner,owner,*vigorous_spirit(12),0,100));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return .0;});
  if(mode==1)check(w.apply_condition(foe,owner,combat::conditions::Kind::blind,100,10000));
  if(mode==2){Slots bar{};bar[0]={380,0};check(w.configure_adrenaline(foe,bar,100));check(w.status_mutation(foe,380,500,12,100,10100,true));}
  combat::weapon::Owned weapon{3,1,{20,20,2},12,false,false,27};
  if(kind==1)weapon={3,1,{20,20,11},0,false,true,22};if(kind==2)weapon={3,1,{20,20,1},12,false,false,5};
  check(w.ordinary_weapon(owner,weapon,20)&&w.ordinary_attack(owner,2,100));box->drain();
  w.tick(499);check(w.resources(1)->health==500);
  if(mode==3){w.cancel_ordinary_attack(owner);w.tick(600);w.tick(1200);check(w.resources(1)->health==500);continue;}
  w.tick(600);check(w.resources(1)->health==517);
  const auto wire=box->drain();const auto heal=skills::particles::health_change_2022(1,1,17,1000);
  check(std::count(wire.begin(),wire.end(),heal)==1);
  if(kind==0&&mode==0){
   const auto damage=std::find_if(wire.begin(),wire.end(),[](const auto& p){return p.size()>2&&p[0]==162&&p[1]==0&&p[2]==16;});
   check(damage!=wire.end()&&std::find(wire.begin(),wire.end(),heal)<damage);
  }
  w.tick(600);w.tick(1200);w.cancel_ordinary_attack(owner);check(w.resources(1)->health==517);
  const auto later=box->drain();check(std::count(later.begin(),later.end(),heal)==0);
  const auto outcomes=w.take_attack_outcomes(owner);check(outcomes.size()==1);
  check(outcomes.front().outcome==(mode==1?world::Instance::AttackOutcome::miss:mode==2?world::Instance::AttackOutcome::block:world::Instance::AttackOutcome::hit));
 }
 for(bool bow:{false,true})for(unsigned mode=0;mode<4;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;d.physical_armor=60;d.physical_armor_evidence="authored test";n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Attacker",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);const auto owner=*w.effect_scope(1),foe=*w.effect_scope(100);
  check(w.complete_vigorous_spirit(owner,owner,*vigorous_spirit(12),0,100));w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return .0;});
  if(mode==1)check(w.apply_condition(owner,owner,combat::conditions::Kind::blind,100,10000));
  if(mode==2)check(w.stage_npc_shield_deflection(foe,*shield_deflection(12),100));
  combat::weapon::Owned weapon{3,1,{20,20,2},12,false,false,27};if(bow)weapon={3,1,{20,20,1},12,false,false,5};
  check(w.ordinary_weapon(owner,weapon,20)&&w.ordinary_attack(owner,100,100));box->drain();w.tick(499);check(w.resources(1)->health==500);
  if(mode==3)w.cancel_ordinary_attack(owner);w.tick(600);check(w.resources(1)->health==(mode==3?500:517));
  w.tick(600);w.tick(1200);w.cancel_ordinary_attack(owner);check(w.resources(1)->health==(mode==3?500:517));
  const auto heal=skills::particles::health_change_2022(1,1,17,1000);const auto wire=box->drain();check(std::count(wire.begin(),wire.end(),heal)==unsigned(mode!=3));
  const auto outcomes=w.take_attack_outcomes(owner);check(outcomes.size()==unsigned(mode!=3));
  if(mode!=3)check(outcomes.front().outcome==(mode==1?world::Instance::AttackOutcome::miss:mode==2?world::Instance::AttackOutcome::block:world::Instance::AttackOutcome::hit));
 }
 for(bool cancel:{false,true}){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);const auto owner=*w.effect_scope(1);
  check(w.complete_vigorous_spirit(owner,owner,*vigorous_spirit(12),0,100));
  casting::Runtime runtime(1);casting::Hooks h;unsigned callbacks=0;
  h.resources=[&](auto id){return w.resources(id);};h.events=[](const auto&){};h.target_valid=[](auto id,float,bool){return id==1;};
  h.consume_energy_at=[&](auto n,auto at){return w.consume_energy(1,n,owner,at);};
  // Isolate completion delivery from the already-tested base Orison heal.
  h.heal=[](unsigned,unsigned,float,bool){return true;};
  h.completed_action=[&](auto skill,auto at){++callbacks;return w.completed_skill_action(owner,skill,at);};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[281]=1;Slots bar{};bar[0]={281,0};attributes::Ranks ranks{};ranks[13]=12;
  check(runtime.start(100,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);
  if(cancel)runtime.cancel(h);runtime.update(1100,h);
  check(callbacks==unsigned(!cancel)&&w.resources(1)->health==(cancel?500:517));
  runtime.update(1100,h);check(callbacks==unsigned(!cancel)&&w.resources(1)->health==(cancel?500:517));
  check(w.completed_skill_action(owner,347,1200)&&w.resources(1)->health==(cancel?500:517));
 }
 std::cout<<"PASS Vigorous ordinary melee/wand/bow releases,miss/block/cancel,pre-impact feedback,no projectile repeat and Runtime spell completion/cancel delivery;skill attack/NPC producer coverage pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
