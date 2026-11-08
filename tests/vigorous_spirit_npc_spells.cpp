#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Vigorous NPC spells line "+std::to_string(at.line()));}
int main()try{
 for(unsigned kind=0;kind<5;++kind)for(bool allowed:{false,true})for(bool cancelled:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{100,500};d.damageable=true;d.conditions_allowed=true;d.knockdown_allowed=true;d.knockdown_evidence="authored fixture";
  d.healing_allowed=allowed;d.healing_evidence="authored fixture";d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Target",100,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(100),target=*w.effect_scope(1);check(w.elemental_armor(target,60,60));
  combat::ai::FlarePolicy policy;policy.source=source;policy.level=20;policy.fire_magic=12;policy.speed=300;policy.acquire=1000;policy.follow=1500;policy.leash=1500;policy.flight={100,32};policy.evidence="authored fixture";
  const unsigned skill=kind<=1?194:kind==2?191:kind==3?186:214;
  if(!kind)check(w.configure_npc_flare_ai(policy,1000));
  else {combat::ai::elemental::Plan plan=skill==214?combat::ai::elemental::Plan{*water_projectile(214,12)}:combat::ai::elemental::Plan{*elemental(skill,12)};
   check(w.configure_npc_elemental_ai(policy,{{{static_cast<std::uint16_t>(skill),0},plan,"authored fixture"}},4,1000));check(w.observe_npc_overcast(source,0,1000,"authored zero Overcast"));}
  check(w.stage_npc_vigorous_spirit(source,*vigorous_spirit(12),1000));box->drain();w.tick(1000);
  std::uint64_t completes=0;
  if(!kind){auto state=w.npc_flare_ai_snapshot(source);check(state&&state->cast);completes=state->cast->completes;}
  else {auto state=w.npc_elemental_ai_snapshot(source);check(state&&state->pending);completes=state->pending->token.completes;}
  check(w.npc_combat_snapshot(source)->health->current==100);
  if(cancelled)check(w.gale(target,source,12,{},completes-1));w.tick(completes);
  const unsigned heals=allowed&&!cancelled?1:0;check(w.npc_combat_snapshot(source)->health->current==100+17*heals);
  const auto heal=skills::particles::health_change_2022(100,100,17,500);auto packets=box->drain();check(std::count(packets.begin(),packets.end(),heal)==heals);
  w.tick(completes);w.tick(completes+100);check(w.npc_combat_snapshot(source)->health->current==100+17*heals);packets=box->drain();check(std::count(packets.begin(),packets.end(),heal)==0);
 }
 std::cout<<"PASS20 Vigorous NPC Flare/elemental spell completion without Spiteful,healing immunity,pre-impact release,knockdown cancellation and no repeated heal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
