// DETACHED: real opposed-attack -> holy return -> NPC Seed cohort acceptance.
// No registered sources changed; normal248 remains unbound.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Retribution NPC return Seed draft line "+std::to_string(at.line()));
}
static world::Instance::Npc body(unsigned id,bool hostile,unsigned hp,float x,bool sensitive=false){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.primary=3;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
 d.spawn_health={double(hp),1000};d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=4;
 d.packed_allegiance=hostile?0x5000001u:0x3000001u;d.damageable=true;d.physical_armor=60;
 d.healing_allowed=true;d.healing_evidence="authored return Seed fixture";
 d.holy_traits={sensitive,false};d.light_sensitive_evidence=d.summoned_evidence="authored explicit holy traits";
 n.combat_definition=d;return n;
}
int main()try{unsigned cases=0;
 for(bool sensitive:{false,true})for(int reversal:{-1,0,12})for(unsigned initial:{10u,500u}){
  auto w=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>();
  w->configure_npcs({body(100,true,1000,110),body(101,true,1000,100),body(102,false,initial,80,sensitive),body(104,false,500,120),body(105,false,500,500)});
  w->join(1,"Seed caster",80,0,box,0,{}, {},100,1);w->set_vitals(1,{500,1000,100,100},100);
  const auto maintainer=*w->effect_scope(100),defender=*w->effect_scope(101),attacker=*w->effect_scope(102),monk=*w->effect_scope(1);
  const auto seed=*healing_seed(12);
  check(w->stage_retribution(maintainer,defender,*retribution(20),100));
  check(w->complete_healing_seed(monk,attacker,seed,0,100));
  if(reversal>=0)check(w->stage_npc_reversal_from(attacker,attacker,unsigned(reversal),100));
  const combat::ai::ordinary::Reviewed weapon{100,100,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"authored100damage return Seed"};
  check(w->configure_npc_opposed_ordinary(attacker,weapon,1000));
  const auto token=w->begin_npc_opposed_ordinary(attacker,defender,1000);check(bool(token));box->drain();w->tick(token->completes);
  const double amount=sensitive?50.:25.;
  const double converted=reversal<0?0.:std::min(amount,double(reversal_fortune(unsigned(reversal))->maximum_healing));
  double hp=std::min(1000.,initial+converted)-(amount-converted);hp=std::max(0.,hp);
  const bool triggers=amount>converted;
  if(hp>0&&triggers)hp=std::min(1000.,hp+seed.healing);
  check(w->npc_combat_snapshot(defender)->health->current==900);
  check(w->npc_combat_snapshot(attacker)->health->current==hp);
  check(w->npc_combat_snapshot(*w->effect_scope(104))->health->current==500+(triggers?seed.healing:0));
  check(w->npc_combat_snapshot(*w->effect_scope(105))->health->current==500);
  check(w->resources(1)->health==500+(triggers?seed.healing:0));
  check(!w->retribution_damage_failures()&&!w->retribution_pending_damage_contacts());
  const auto packets=box->drain();const auto heal=skills::particles::health_change_2022(104,1,seed.healing,1000);
  check(std::count(packets.begin(),packets.end(),heal)==unsigned(triggers));
  w->tick(token->completes);check(box->drain().empty());++cases;
 }
 std::cout<<"PASS "<<cases<<" detached real NPC return holy sensitivity/Reversal/Seed cohort cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
