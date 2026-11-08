#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Virulence World line "+std::to_string(at.line()));
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
  if(mode&&mode!=7){if(npc)check(w.condition_spell(source,target,*condition_spell(117,0),100));
   else check(w.apply_condition(source,target,Kind::blind,100,mode==2?50:30000));}
  if(!npc&&mode==8)check(w.tainted_flesh(target,target,*tainted_flesh(12),100));
  if(mode==4)check(w.degeneration_hex(source,target,*degeneration_hex(19,12),100));
  const auto now=mode==2&&npc?10100u:150u;std::vector<world::Packet> visuals;
  auto selected=target;if(mode==3)++selected.generation;
  check(w.virulence(source,selected,*virulence(12),now,&visuals)==(mode!=3&&mode!=6&&mode!=9));
  check(visuals.size()==unsigned(mode!=3&&mode!=6&&mode!=9));
  if(mode==5||mode==10)check(w.has_condition(target,Kind::weakness,now)&&!w.has_condition(target,Kind::disease,now)&&!w.has_condition(target,Kind::poison,now));
  if(mode==5||mode==10)check(w.has_condition(target,Kind::weakness,10100)&&!w.has_condition(target,Kind::weakness,13150));
  if(mode==6)check(!w.has_condition(target,Kind::weakness,10100));
  if(mode==6||mode==7)check(!w.has_condition(target,Kind::disease,now)&&!w.has_condition(target,Kind::poison,now));
  if(mode==1)for(const auto kind:{Kind::disease,Kind::weakness,Kind::poison})check(w.has_condition(target,kind,now));
  if(mode==0||mode==2||mode==3)check(!w.has_condition(target,Kind::disease,now)&&!w.has_condition(target,Kind::poison,now));
  if(mode==8)check(!w.has_condition(target,Kind::disease,now)&&w.has_condition(target,Kind::poison,now)&&w.has_condition(target,Kind::weakness,now));
  if(mode==9)check(!w.has_condition(target,Kind::disease,now)&&!w.has_condition(target,Kind::poison,now)&&!w.has_condition(target,Kind::weakness,10100));
  if(mode==4){const auto health=npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);
   check(health==0&&!w.has_condition(target,Kind::poison,now));}
 }
 std::cout<<"PASS Virulence World player/NPC prerequisite,exact expiry,stale scope,three conditions and Fragility death stopping later applications; owned immunity,unknown-trait preflight and contagion-ready commits\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
