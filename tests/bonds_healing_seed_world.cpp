// Reflected cold damage must trigger Seed only for actual remaining damage.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Bonds Seed line "+std::to_string(at.line()));
}
int main()try{
 unsigned cases=0;
 for(bool lethal_source:{false,true})for(int reversal:{-1,0,20})
 for(unsigned amount:{3u,100u,200u})for(bool absorption:{false,true}){
  ++cases;world::Instance w;
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;
  n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={lethal_source?double(amount):1000.,1000};
  d.damageable=true;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",id==1?0:700,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{500,1000,100,100},100);
  }
  const auto attacker=*w.effect_scope(1),victim=*w.effect_scope(2),healer=*w.effect_scope(3),npc=*w.effect_scope(100);
  check(w.elemental_armor(victim,60,60));
  check(w.complete_healing_seed(healer,victim,*effects::healing_seed(12),0,100));
  check(w.stage_npc_bonds_of_torment(npc,victim,*effects::bonds_of_torment_npc(),100));
  if(absorption)check(w.status_mutation(victim,1399,901,12,100,6100,true));
  if(reversal>=0)check(w.status_mutation(victim,307,900,reversal,100,8100,true));
  const auto hit=w.damage_npc(attacker,npc,{combat::Kind::armor_ignoring,double(amount),20,42},1248,200);
  check(hit.reason==combat::Reason::accepted&&hit.applied==amount&&hit.killed==lethal_source);
  const unsigned incoming=amount-std::min(amount,absorption?5u:0u);
  const unsigned converted=reversal<0?0:std::min(incoming,effects::reversal_fortune(reversal)->maximum_healing);
  const bool triggers=incoming>converted;
  const unsigned healing=triggers?effects::healing_seed(12)->healing:0;
  check(w.resources(2)->health==500+converted-(incoming-converted)+healing);
  check(w.resources(3)->health==500+healing);
  check(w.take_npc_deaths(attacker).size()==unsigned(lethal_source));
 }
 check(cases==36);
 std::cout<<cases<<" Bonds Seed reflection cases: actual loss, full conversion and lethal caster\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
