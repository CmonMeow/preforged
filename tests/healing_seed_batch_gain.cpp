// Integration: real two-strike Holy Strike uses shared copied batch
// admission and committed Seed cohorts. An immune spirit must gain Health in
// BOTH paths; applying candidate publication alone must still reject this test.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Seed batch gain line "+std::to_string(at.line()));
}
static world::Instance::Npc body(unsigned id,float x,bool spirit,unsigned hp=500){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={double(hp),1000};d.damageable=true;
 d.creature_type=spirit?"spirit":"human";d.creature_type_evidence="authored batch gain recipient";
 d.healing_allowed=!spirit;d.healing_evidence="explicit spirit immunity";
 d.fleshy=!spirit;d.fleshy_evidence="explicit non-fleshy spirit";d.conditions_allowed=!spirit;
 n.combat_definition=d;return n;
}
int main()try{
 for(bool wound:{false,true}){
  auto w=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>();
  w->configure_npcs({body(102,80,true),body(103,500,true),body(104,110,false,10)});
  for(unsigned id:{1u,2u,3u,4u}){
   w->join(id,"Actor",id==1?0.f:id==2?60.f:100.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w->set_vitals(id,{id==4?10u:500u,1000,100,100},100);
  }
  const auto source=*w->effect_scope(1),target=*w->effect_scope(2),caster=*w->effect_scope(3),fragile=*w->effect_scope(4),npc_fragile=*w->effect_scope(104);
  check(w->player_holy_equipment(target,0));
  for(const auto recipient:{target,caster}){
   check(w->degeneration_hex(source,recipient,*degeneration_hex(129,12),100));
   if(wound)check(w->apply_condition(source,recipient,combat::conditions::Kind::deep_wound,100,10000));
  }
  check(w->apply_condition(source,fragile,combat::conditions::Kind::deep_wound,100,10000));
  check(w->apply_condition(source,npc_fragile,combat::conditions::Kind::deep_wound,100,10000));
  const auto epoch=*w->control_epoch(fragile);
  const auto before_target=w->resources(2)->health,before_caster=w->resources(3)->health;
  const auto seed=*healing_seed(12);check(w->complete_healing_seed(caster,target,seed,0,100));
  // Two direct strikes, not a packet mirror, force Seed feedback between hits.
  check(w->gale(source,target,12,{},100));const auto strike=*holy_damage(312,12);
  box->drain();check(w->holy_damage(source,target,strike,20,200));
  check(w->resources(2)->health==before_target-2*strike.damage+2*seed.healing);
  check(w->resources(3)->health==before_caster+2*seed.healing);
  check(w->npc_combat_snapshot(*w->effect_scope(102))->health->current==500+2*seed.healing);
  check(w->npc_combat_snapshot(*w->effect_scope(103))->health->current==500);
  check(w->resources(4)->health==0&&*w->control_epoch(fragile)==epoch+1);
  check(w->npc_combat_snapshot(npc_fragile)->health->current==0);
  const auto packets=box->drain();
  check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(102,3,seed.healing,1000))==2);
  w->tick(200);check(w->resources(4)->health==0&&*w->control_epoch(fragile)==epoch+1);
  check(w->npc_combat_snapshot(npc_fragile)->health->current==0);
 }
 std::cout<<"PASS two-strike batch admission/committed Seed gains, immune spirit, Defile, wound and death-once\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
