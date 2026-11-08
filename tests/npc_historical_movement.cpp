#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace combat::npcs;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("NPC historical movement line "+std::to_string(at.line()));}
bool near(double a,double b){return std::abs(a-b)<1e-6;}
int main()try{
 const Scope scope{1,100,2};
 {control::State state(scope);MovementEffects movement;movement.apply({1,1,76,0,Category::hex,100,1000});
  check(state.begin_movement_pause(scope,100));check(state.knock_down(scope,true,200,500)==control::Knockdown::accepted);
  check(state.end_movement_pause(scope,400,750));check(!state.end_movement_pause(scope,400,750));
  check(state.historical_movement_rate(movement,99,1)==1.);
  for(auto at:{100u,199u,200u,699u,700u,1000u,1149u})check(state.historical_movement_rate(movement,at,.5f)==0.);
  check(state.historical_movement_rate(movement,1150,.5f)==.5);
  check(state.historical_movement_boundaries(movement,0,1200)==std::optional(std::vector<std::uint64_t>{100,200,700,1000,1150}));
  check(state.historical_movement_boundaries(movement,100,1150)==std::optional(std::vector<std::uint64_t>{200,700,1000,1150}));
  for(float bad:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN()})check(!state.historical_movement_rate(movement,200,bad));
  state.consume_movement(1100);check(!state.historical_movement_rate(movement,1099,1)&&!state.historical_movement_boundaries(movement,0,1200));
  check(state.historical_movement_rate(movement,1100,1)==0.);state.consume_movement(1150);
  check(state.historical_movement_rate(movement,1150,1)==1.&&state.historical_movement_boundaries(movement,1150,1200)->empty());
 }
 {control::State state(scope);MovementEffects movement;
  movement.apply({1,1,210,0,Category::hex,100,1000}); //34% strongest-snare floor.
  const auto composed=state.historical_movement_rate(movement,200,.5f);check(composed&&near(*composed,.34));
  check(*composed>movement.at(200)*.5); // Factor must enter before clamp.
  check(state.begin(scope,control::Action::spell,300,600).has_value());
  check(state.historical_movement_rate(movement,400,1).has_value()); // Generic token creates no spellbook pause.
 }
 Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
 d.relation=Relation::hostile_to_players;d.spawn_health=combat::Health{100,100};d.damageable=d.conditions_allowed=true;
 d.spawn_energy=Energy{100,100};d.energy_pips=0;d.knockdown_allowed=true;d.knockdown_evidence="authored fixture";
 const Lifetime life{1,1,2,146,100};const combat::ai::SpellTarget target{{1,1,1},100,true,true,true,true};
 for(unsigned mode=0;mode<5;++mode){LiveActor actor(d,life);
  check(actor.configure_spellbook(life,{{{194,0},1248,"authored history fixture"}}));
  const auto cast=actor.begin_spell(life,{194,0},target,1000);check(cast&&cast->completes==2000);
  check(actor.historical_movement_rate(life,999,1)==1.&&actor.historical_movement_rate(life,1000,1)==0.);
  check(actor.historical_movement_rate(life,100000,1)==0.); // No invented future completion.
  if(mode==0)check(actor.complete_action(life,*cast,2000));
  if(mode==1)check(actor.voluntarily_cancel_spell(life,1500));
  if(mode==2)check(actor.fail_target_action(life,*cast,2000));
  if(mode==3)check(actor.cancel_action(life,1500));
  if(mode==4)check(actor.knock_down(life,1500,2000)==control::Knockdown::accepted);
  const auto resumes=mode==0?2750u:mode==2?2000u:mode==4?3500u:1500u;
  check(actor.historical_movement_rate(life,999,1)==1.); // Completion clock does not erase history.
  check(actor.historical_movement_rate(life,resumes-1,1)==0.&&actor.historical_movement_rate(life,resumes,.5f)==.5);
  const auto points=actor.historical_movement_boundaries(life,0,resumes);check(points&&points->front()==1000&&points->back()==resumes);
  if(mode==4)check(*points==std::vector<std::uint64_t>({1000,1500,3500}));
  if(mode==0){auto copy=actor;check(copy.historical_movement_rate(life,2500,1)==0.&&actor.historical_movement_rate(life,2500,1)==0.);}
  auto stale=life;++stale.actor_generation;check(!actor.historical_movement_rate(stale,1000,1)&&!actor.historical_movement_boundaries(stale,0,resumes));
  actor.consume_movement(life,resumes);check(!actor.historical_movement_rate(life,resumes-1,1));
  check(actor.historical_movement_rate(life,resumes,1)==1.&&actor.historical_movement_boundaries(life,resumes,resumes+100)->empty());
 }
 {LiveActor actor(d,life);check(actor.configure_spellbook(life,{{{194,0},1248,"two retained casts"}}));
  const auto first=actor.begin_spell(life,{194,0},target,1000);check(first&&actor.complete_action(life,*first,2000));
  const auto second=actor.begin_spell(life,{194,0},target,3000);check(bool(second));
  check(actor.historical_movement_rate(life,2800,1)==1.&&actor.historical_movement_rate(life,3200,1)==0.);
  check(actor.voluntarily_cancel_spell(life,3500));
  check(actor.historical_movement_rate(life,2500,1)==0.&&actor.historical_movement_rate(life,2800,1)==1.);
  check(actor.historical_movement_boundaries(life,0,4000)==std::optional(std::vector<std::uint64_t>{1000,2750,3000,3500}));
 }
 std::cout<<"NPC retained movement rate, before-clamp Ward composition, exact KD/cast/aftercast boundaries and consumed/stale rejection passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
