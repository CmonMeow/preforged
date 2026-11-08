#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Direct condition flesh line "+std::to_string(at.line()));
}
int main()try{
 using combat::conditions::Kind;
 unsigned cases=0;
 for(unsigned trait=0;trait<3;++trait)for(unsigned index=0;index<static_cast<unsigned>(Kind::count);++index){
  const auto kind=static_cast<Kind>(index);const bool flesh_only=kind==Kind::bleeding||kind==Kind::poison||kind==Kind::disease;
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.spawn_health={1000,1000};d.damageable=d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
  if(trait<2){d.fleshy=trait==0;d.fleshy_evidence="synthetic explicit body trait";}
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Source",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  check(w.degeneration_hex(source,target,*skills::effects::degeneration_hex(2998,12),100,20));box->drain();
  const bool applied=!flesh_only||trait==0;
  check(w.apply_condition(source,target,kind,200,5000)==(!flesh_only||trait<2));
  check(w.has_condition(target,kind,200)==applied);
  const auto loss=skills::effects::fragility(2998,12)->damage;
  const unsigned capacity=kind==Kind::deep_wound?900:1000; //20%, capped at100Health.
  check(w.npc_combat_snapshot(target)->health->current==capacity-(applied?loss:0));
  if(!applied)check(box->drain().empty());
  ++cases;
 }
 std::cout<<cases<<" direct NPC condition/flesh/unknown/Fragility cases pass\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
