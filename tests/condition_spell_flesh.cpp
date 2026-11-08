#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Condition flesh cohort line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){
   std::optional<bool> flesh=true;
   if((id==100&&(mode==1||mode==2))||(id==101&&mode==2))flesh=false;
   if((id==101&&(mode==3||mode==5))||(id==100&&mode==4))flesh.reset();
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?100:mode==5?500:150;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.spawn_health={1000,1000};
   d.damageable=d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;
   d.fleshy=flesh;if(flesh)d.fleshy_evidence="synthetic body trait";
   n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),a=*w.effect_scope(100),b=*w.effect_scope(101);
  for(auto target:{a,b})check(w.degeneration_hex(source,target,*effects::degeneration_hex(2998,12),100,20));
  box->drain();const bool result=w.condition_spell(source,a,*effects::condition_spell(131,12),200);
  const bool refused=mode==3||mode==4;check(result!=refused);
  const bool bleed_a=!refused&&mode!=1&&mode!=2,bleed_b=!refused&&mode!=2&&mode!=5;
  check(w.has_condition(a,combat::conditions::Kind::bleeding,200)==bleed_a);
  check(w.has_condition(b,combat::conditions::Kind::bleeding,200)==bleed_b);
  const auto loss=effects::fragility(2998,12)->damage;
  check(w.npc_combat_snapshot(a)->health->current==1000-(bleed_a?loss:0));
  check(w.npc_combat_snapshot(b)->health->current==1000-(bleed_b?loss:0));
  if(refused)check(box->drain().empty());
 }
 std::cout<<"6Barbed Signet flesh/immunity/unknown/out-of-area cohorts and Fragility guards pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
