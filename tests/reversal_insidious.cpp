#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Reversal Insidious line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;unsigned route;effects::Scope source,target,victim;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned r,unsigned hp,unsigned source_hp):route(r){
  w.configure_attack_miss_draws([]{return .5;});
  if(route==3){
   const std::string hash(64,'a');
   std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
   auto pop=world::load_npc_catalog(appearance);
   std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/reversal_insidious.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 1000 1000 60 1 1 test test test - test 60 60 test test 0 test\n");
   const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(pop,catalog,hash);
   auto enemies=pop.for_map(146);auto guard=enemies.front();guard.combat_definition->relation=combat::npcs::Relation::neutral;
   guard.combat_definition->physical_armor=60;guard.combat_definition->physical_armor_evidence="synthetic known armor";
   enemies.front().combat_definition->spawn_health={double(hp),1000};
   enemies.front().combat_definition->healing_allowed=true;enemies.front().combat_definition->fleshy=true;
   enemies.front().combat_definition->healing_evidence=enemies.front().combat_definition->fleshy_evidence="synthetic conversion recipient";
   w.configure_npcs({guard});w.join(1,"Owner",100,0,box);w.set_vitals(1,{source_hp,1000,100,100},100);
   source=*w.effect_scope(1);victim=*w.effect_scope(100);
   check(w.accept_quest_waves(source,victim,73,1,{enemies},[]{return true;}));target=*w.effect_scope(60000);
  }else{
   for(unsigned id:{1u,2u,3u}){
    w.join(id,"Actor",id==1?1000.f:id==2?0.f:60.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);
    w.set_vitals(id,{id==1?source_hp:id==2?hp:1000u,1000,100,100},100);
    check(w.physical_armor(*w.effect_scope(id),60));
   }
   source=*w.effect_scope(1);target=*w.effect_scope(2);victim=*w.effect_scope(3);
   check(w.status_mutation(victim,299,80,20,100,10100,true)); // Zero attack damage still hits.
  }
  box->drain();
 }
 double hp(){return route==3?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 void reversal(int rank){if(rank>=0)check(route==3?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,5,rank,100,8100,true));}
 bool has_reversal(){const auto entries=*w.status_snapshot(target,1000);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
 static combat::weapon::Strike guardian_strike(){
  combat::weapon::Strike strike;strike.level=1;strike.minimum=strike.maximum=1;strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;return strike;
 }
 void hit(bool miss){
  if(miss)check(w.apply_condition(source,target,combat::conditions::Kind::blind,100,5000));
  if(route==3){
   const auto strike=guardian_strike();
   w.damage_quest_guardian(target,victim,{combat::Kind::armor_ignoring,1,1,0},128,1000,strike,1000);
  }else{
   combat::weapon::Owned weapon{1,1,{1,1,2},12,false,false,route==2?2u:27u};check(w.ordinary_weapon(target,weapon,20));
   if(route==0){check(w.ordinary_attack(target,3,100));w.tick(600);w.cancel_ordinary_attack(target);}
   else w.melee_attack(target,route==2?target:victim,weapon,*combat::weapon::attack_skill(route==2?330:322,0),600);
  }
 }
};
int main()try{
 unsigned cases=0;
 for(unsigned route=0;route<4;++route)for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})
 for(unsigned hp:{10u,300u,1000u})for(unsigned source_hp:{100u,990u,1000u})for(bool miss:{false,true})for(bool absorption:{false,true}){
  Fixture f(route,hp,source_hp);check(f.w.apply_insidious_parasite(f.source,f.target,rank,100));
  if(absorption)check(route==3?f.w.stage_npc_shield_absorption(f.target,12,100):f.w.status_mutation(f.target,1399,81,12,100,6100,true));
  f.reversal(rr);f.box->drain();const auto plan=*effects::insidious_parasite(rank);
  const double converted=miss||rr<0?0:std::min(plan.amount,effects::reversal_fortune(rr)->maximum_healing);
  const double healed=std::min(1000.,hp+converted),stolen=miss?0:std::min(double(plan.amount)-converted,healed+1);
  const double expected=std::max(0.,healed-stolen),source=std::min(1000.,source_hp+stolen);
  f.hit(miss);
  if(f.hp()!=expected||f.w.resources(1)->health!=source)std::cerr<<"route="<<route<<" rank="<<rank<<" rr="<<rr<<" hp="<<hp<<" miss="<<miss<<" actual="<<f.hp()<<'/'<<f.w.resources(1)->health<<" expected="<<expected<<'/'<<source<<'\n';
  check(f.hp()==expected&&f.w.resources(1)->health==source&&f.has_reversal()==(miss&&rr>=0));
  const auto outcomes=f.w.take_attack_outcomes(f.target);check(outcomes.size()==1);
  check(outcomes.front().outcome==(miss?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit));
  const auto stable=f.hp();f.w.tick(1100);check(f.hp()==stable&&f.w.take_attack_outcomes(f.target).empty());++cases;
 }
 for(unsigned route=0;route<4;++route)for(unsigned rr:{0u,12u,20u}){
  Fixture f(route,10,100);check(f.w.apply_insidious_parasite(f.source,f.target,12,100));
  check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));f.reversal(rr);
  f.hit(false);check(f.hp()==0&&f.w.resources(1)->health==100&&!f.has_reversal());
  // Preserve the existing committed-contact policy, not a new claim that a
  // fatal pre-damage reaction cancels an already-qualified hit. Retail fatal
  // ordering remains unverified. Player fixtures absorb their attack damage.
  if(route==3){const auto damage=combat::weapon::resolve(Fixture::guardian_strike());check(damage.has_value());
   if(f.w.npc_combat_snapshot(f.victim)->health->current!=1000-damage->damage)std::cerr<<"fatal guardian health="<<f.w.npc_combat_snapshot(f.victim)->health->current<<" predicted="<<1000-damage->damage<<" rr="<<rr<<'\n';
   check(f.w.npc_combat_snapshot(f.victim)->health->current==1000-damage->damage);}
  else check(f.w.resources(3)->health==1000);
  ++cases;
 }
 for(unsigned rr:{0u,12u,20u}){
  Fixture f(1,300,100);check(f.w.apply_insidious_parasite(f.source,f.target,0,100));
  check(f.w.status_mutation(f.target,299,81,20,100,10100,true));f.reversal(rr);f.hit(false);
  check(f.hp()==300&&f.w.resources(1)->health==100&&f.has_reversal());++cases;
 }
 std::cout<<cases<<" Insidious conversion cases across ordinary, single, Cyclone and NPC attacks,miss/zero damage,source cap,wound death and zero input passed;paid307 unbound\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
