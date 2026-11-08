#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Phantom Pain World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;skills::effects::Scope source,target;
 Fixture(double health=480,bool conditions=true,float x=100){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=x;
  n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
  d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{health,480};
  d.damageable=true;d.conditions_allowed=conditions;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Phantom caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(1,{500,1000,50,100},100);source=*w.effect_scope(1);target=*w.effect_scope(100);
 }
};
int main()try{
 for(unsigned rank=0;rank<=20;++rank){
  Fixture f;const auto plan=*skills::effects::wound_spell(44,rank);
  check(f.w.wound_spell(f.source,f.target,plan,20,100));
  f.w.tick(10000);check(!f.w.has_condition(f.target,combat::conditions::Kind::deep_wound,10000));
  f.w.tick(10100);check(f.w.has_condition(f.target,combat::conditions::Kind::deep_wound,10100));
  check(f.w.npc_combat_snapshot(f.target)->health->maximum==384);
  f.w.tick(10100+plan.wound_duration);
  check(!f.w.has_condition(f.target,combat::conditions::Kind::deep_wound,10100+plan.wound_duration));
  check(f.w.npc_combat_snapshot(f.target)->health->maximum==480);
 }
 const auto plan=*skills::effects::wound_spell(44,12);
 {Fixture f;check(f.w.wound_spell(f.source,f.target,plan,20,100));
  check(f.w.wound_spell(f.source,f.target,plan,20,1100));f.w.tick(10100);
  check(!f.w.has_condition(f.target,combat::conditions::Kind::deep_wound,10100));
  f.w.tick(11100);check(f.w.has_condition(f.target,combat::conditions::Kind::deep_wound,11100));}
 {Fixture f(1);check(f.w.wound_spell(f.source,f.target,plan,20,100));f.w.tick(10100);
  check(!f.w.has_condition(f.target,combat::conditions::Kind::deep_wound,10100));}
 {Fixture f(480,false);check(f.w.wound_spell(f.source,f.target,plan,20,100));f.w.tick(10100);
  check(!f.w.has_condition(f.target,combat::conditions::Kind::deep_wound,10100));}
 {Fixture f(480,true,1300);check(!f.w.wound_spell(f.source,f.target,plan,20,100));}
 {Fixture f;auto stale=f.target;++stale.generation;check(!f.w.wound_spell(f.source,stale,plan,20,100));
  check(!f.w.wound_spell(f.source,f.target,plan,20,99));
  auto bad=plan;++bad.degeneration;check(!f.w.wound_spell(f.source,f.target,bad,20,100));}
 std::cout<<"Phantom Pain World NPC admission, deadlines, renewal, death, immunity and refusal passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
