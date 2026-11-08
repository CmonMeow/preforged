#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Energy hex departure line "+std::to_string(at.line()));}
int main()try{
 for(bool npc:{false,true})for(unsigned skills:{1u,2u,3u})for(bool sparse:{false,true})for(unsigned mode=0;mode<3;++mode){
  world::Instance w;const unsigned id=npc?100:2;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.spawn_energy=combat::npcs::Energy{1,20};d.energy_pips=0;d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Original",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc){w.join(2,"Target",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,1,20},100);check(w.energy_capacity(2,1,0,100,{0,-2}));}
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(id);
  if(skills&1)check(w.stage_energy_degeneration_hex(caster,target,*skills::effects::energy_degeneration_hex(125,12),20,1000));
  if(skills&2)check(w.stage_energy_degeneration_hex(caster,target,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  w.tick(2000);w.leave(1);
  if(mode){w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},2000,1);w.set_vitals(1,{777,777,100,100},100);check(w.effect_scope(1)!=caster);}
  if(mode==2){const auto replacement=*w.effect_scope(1);if(skills&1)check(w.stage_energy_degeneration_hex(replacement,target,*skills::effects::energy_degeneration_hex(125,12),20,2100));if(skills&2)check(w.stage_energy_degeneration_hex(replacement,target,*skills::effects::energy_degeneration_hex(140,12),20,2100));}
  if(!sparse)for(unsigned at=2100;at<7000;at+=100)w.tick(at);w.tick(7000);
  check(!w.pending_energy_zero_snapshot(target));
  const auto energy=npc?w.npc_precise_energy_snapshot(target):std::optional<std::int64_t>(overcast::precise_current(*w.overcast_energy_snapshot(target)));
  check(energy&&*energy==0); // Zero base pips: ending the hex must not invent Energy.
  if(npc)check(!(w.npc_combat_snapshot(target)->effects&0x800));
  if(!npc){const auto active=w.status_snapshot(target,7000);check(active&&std::none_of(active->begin(),active->end(),[](const auto& e){return e.skill==125||e.skill==140;}));}
  if(mode==1)check(w.resources(1)->health==777); // Old lifetime cannot charge the replacement.
  if(mode==2)check(w.resources(1)->health==(skills==2?774u:777u));
 }
 std::cout<<"PASS36 Energy-hex source departure cases:player/NPC,Wither/Malaise/both,dense/sparse,no source/reused ID/renewal,no stalled zero or inherited timer\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

