#include "gw_spiteful_reaction.h"
#include <iostream>
#include <limits>
#include <source_location>
#include <string>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Spiteful reaction policy line "+std::to_string(at.line()));
}
int main()try{
 check(spiteful_trigger(SpitefulPhase::attack_release,0)==SpitefulTrigger::trigger);
 check(spiteful_trigger(SpitefulPhase::attack_release,322)==SpitefulTrigger::trigger);
 check(spiteful_trigger(SpitefulPhase::skill_completed,322)==SpitefulTrigger::none);
 for(unsigned skill:{1239u,1253u,1232u,2110u,2235u,1237u,113u,343u}){
  check(spiteful_trigger(SpitefulPhase::skill_completed,skill)==SpitefulTrigger::trigger);
  check(spiteful_trigger(SpitefulPhase::attack_release,skill)==SpitefulTrigger::invalid);
 }
 check(spiteful_trigger(SpitefulPhase::skill_completed,0)==SpitefulTrigger::invalid);
 check(spiteful_trigger(static_cast<SpitefulPhase>(99),322)==SpitefulTrigger::invalid);
 check(spiteful_trigger(SpitefulPhase::skill_completed,65535)==SpitefulTrigger::invalid);
 const auto plan=*spiteful_spirit(12);
 const SpitefulAreaActor center{{1,10,2},0,0,0,true,true};
 auto candidate=center;candidate.scope.actor=11;candidate.x=166;
 check(spiteful_contact(plan,center,candidate)==SpitefulContact::eligible);
 candidate.x=166.01f;candidate.ally_to_center.reset();check(spiteful_contact(plan,center,candidate)==SpitefulContact::no_contact);
 candidate.x=166;check(spiteful_contact(plan,center,candidate)==SpitefulContact::unknown);
 candidate.ally_to_center=false;check(spiteful_contact(plan,center,candidate)==SpitefulContact::no_contact);
 candidate.ally_to_center=true;candidate.alive=false;check(spiteful_contact(plan,center,candidate)==SpitefulContact::no_contact);
 candidate.alive=true;candidate.plane=1;check(spiteful_contact(plan,center,candidate)==SpitefulContact::no_contact);
 candidate.plane=0;candidate.scope.instance=2;check(spiteful_contact(plan,center,candidate)==SpitefulContact::no_contact);
 candidate=center;++candidate.scope.generation;check(spiteful_contact(plan,center,candidate)==SpitefulContact::invalid);
 auto unknown=center;unknown.ally_to_center.reset();check(spiteful_contact(plan,unknown,unknown)==SpitefulContact::eligible);
 candidate=center;candidate.x=std::numeric_limits<float>::infinity();check(spiteful_contact(plan,center,candidate)==SpitefulContact::invalid);
 auto ally=center;ally.scope.actor=12;ally.x=100;auto foe=ally;foe.scope.actor=13;foe.ally_to_center=false;
 std::vector<SpitefulAreaActor> population{center,ally,foe};
 auto result=spiteful_area(plan,center.scope,population);check(result&&result->complete()&&result->recipients==std::vector<Scope>{center.scope,ally.scope});
 std::reverse(population.begin(),population.end());const auto reversed=spiteful_area(plan,center.scope,population);
 check(reversed&&reversed->recipients==result->recipients);
 population[0].ally_to_center.reset();result=spiteful_area(plan,center.scope,population);check(result&&!result->complete()&&result->unknown==1);
 population.push_back(center);check(!spiteful_area(plan,center.scope,population));
 population={center,ally};population[1].scope=center.scope;++population[1].scope.generation;check(!spiteful_area(plan,center.scope,population));
 check(!spiteful_area(plan,Scope{1,99,1},{center,ally}));
 auto forged=plan;++forged.radius;check(!spiteful_area(forged,center.scope,{center,ally}));
 check(!spiteful_area(plan,center.scope,std::vector<SpitefulAreaActor>(4097,center)));
 {
  const Scope owner{1,10,2};SpitefulEvents events(owner);
  Entry hex{1,20,121,12,Category::hex,100,18100};
  auto result=events.observe({owner,SpitefulPhase::attack_release,322,1,100,true},hex);
  check(result&&result->reaction&&result->reaction->damage==29&&result->hex->instance==1);
  check(!events.observe({owner,SpitefulPhase::attack_release,322,1,100,true},hex));
  // Same logical attack's skill completion is consumed without a second pulse.
  result=events.observe({owner,SpitefulPhase::skill_completed,322,1,100,true},hex);check(result&&!result->reaction);
  result=events.observe({owner,SpitefulPhase::skill_completed,1239,2,200,false},hex);check(result&&!result->reaction);
  result=events.observe({owner,SpitefulPhase::skill_completed,1239,3,300,true},hex);check(result&&result->reaction);
  auto wrong=owner;++wrong.generation;check(!events.observe({wrong,SpitefulPhase::skill_completed,1239,4,400,true},hex));
  check(!events.observe({owner,SpitefulPhase::skill_completed,1239,4,299,true},hex));
  auto broken=hex;broken.cause=owner.actor;check(!events.observe({owner,SpitefulPhase::skill_completed,1239,4,400,true},broken));
  result=events.observe({owner,SpitefulPhase::skill_completed,1239,4,400,true},hex);check(result&&result->reaction);
  result=events.observe({owner,SpitefulPhase::attack_release,0,2,18100,true},hex);check(result&&!result->reaction);
  hex.instance=2;hex.applied=18100;hex.expires=36100;
  check(!events.observe({owner,SpitefulPhase::attack_release,0,2,18100,true},hex));
  result=events.observe({owner,SpitefulPhase::attack_release,0,3,18100,true},hex);check(result&&result->hex->instance==2);
 }
 {
  const Scope owner{1,10,2};SpitefulEvents events(owner);const Entry hex{1,20,121,0,Category::hex,200,8200};
  const auto absent=events.observe({owner,SpitefulPhase::skill_completed,1239,1,100,true},{});check(absent&&!absent->reaction);
  check(!events.observe({owner,SpitefulPhase::skill_completed,1239,1,200,true},hex));
  const auto present=events.observe({owner,SpitefulPhase::skill_completed,1239,2,200,true},hex);check(present&&present->reaction);
 }
 std::cout<<"PASS Spiteful attack/completed-use partition and immutable adjacent-alliance preflight;event ownership,timing and damage commit remain separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
