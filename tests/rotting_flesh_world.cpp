#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Rotting Flesh World line "+std::to_string(at.line()));
}
int main()try{
 using combat::conditions::Kind;using namespace skills::effects;
 for(bool npc:{false,true})for(unsigned mode=0;mode<11;++mode){
  if(!npc&&mode>=5&&mode!=8)continue;
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{mode==4?10.:1000.,1000};d.damageable=d.conditions_allowed=true;d.fleshy=mode==6||mode==7?std::nullopt:std::optional(mode!=5&&mode!=10);if(d.fleshy)d.fleshy_evidence="fixture";if(mode!=9&&mode!=10){d.disease_immune=mode==8;d.disease_immunity_evidence="fixture";}n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc){w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{mode==4?10u:1000u,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  if(!npc&&mode==8)check(w.tainted_flesh(target,target,*tainted_flesh(12),100));
  if(mode==4)check(w.degeneration_hex(source,target,*degeneration_hex(19,12),100));
  const unsigned now=150;std::vector<world::Packet> visuals;auto selected=target;if(mode==3)++selected.generation;
  const bool accepted=mode!=3&&mode!=6&&mode!=7&&mode!=9;
  check(w.rotting_flesh(source,selected,*rotting_flesh(12),now,&visuals)==accepted);
  check(visuals.size()==unsigned(accepted));
  // Player death clears conditions;NPC corpse snapshots retain their timer.
  const bool afflicted=accepted&&(mode!=4||npc)&&mode!=5&&mode!=10&&mode!=8;
  if(w.has_condition(target,Kind::disease,now)!=afflicted)std::cerr<<"npc="<<npc<<" mode="<<mode<<'\n';
  check(w.has_condition(target,Kind::disease,now)==afflicted);
  check(!w.has_condition(target,Kind::poison,now)&&!w.has_condition(target,Kind::weakness,now));
  if(afflicted){check(w.has_condition(target,Kind::disease,22149));check(!w.has_condition(target,Kind::disease,22150));}
  if(mode==4){const auto health=npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);check(health==0);}

 }
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Target",mode==1?1300.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,mode==2?1:2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);auto plan=*rotting_flesh(12);if(mode==3)++plan.duration;
  std::vector<world::Packet> visuals;
  check(!w.rotting_flesh(source,mode==0?source:target,plan,100,&visuals));
  check(visuals.empty()&&!w.has_condition(target,Kind::disease,100));
 }
 std::cout<<"PASS Rotting Flesh player/NPC,expiry,stale scope,self/ally/range/forged-plan rejection,known nonflesh/immunity,unknown facts,Fragility death and deferred visuals;rendering pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
