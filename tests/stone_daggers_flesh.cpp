#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Stone Daggers flesh line "+std::to_string(at.line()));
}
unsigned launches(const std::vector<world::Packet>& packets){return unsigned(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==163;}));}
int main()try{
 for(unsigned mode=0;mode<7;++mode){
  const bool overcast=mode<3||mode==4,conditions=mode!=4;
  const std::optional<bool> flesh=mode==0||mode==6?std::optional(true):mode==1||mode==5?std::optional(false):std::nullopt;
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.spawn_health={1000,1000};d.damageable=true;d.conditions_allowed=conditions;d.relation=combat::npcs::Relation::hostile_to_players;
  d.earth_armor=60;d.evidence.armor="synthetic Earth armor";d.fleshy=flesh;if(flesh)d.fleshy_evidence="synthetic body trait";
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  check(w.degeneration_hex(source,target,*effects::degeneration_hex(2998,12),100,20));
  if(overcast)check(w.activate_overcast_energy(source,219,0,1,200));box->drain();
  const bool accepted=w.stone_daggers(source,target,*effects::stone_daggers(15),20,1000);
  if(mode==2){
   check(!accepted&&launches(box->drain())==0);w.tick(4000);
   check(w.npc_combat_snapshot(target)->health->current==1000&&launches(box->drain())==0);continue;
  }
  check(accepted&&launches(box->drain())==1);w.tick(1234);check(w.npc_combat_snapshot(target)->health->current==1000);
  w.tick(1334);const bool bleed=mode==0;const unsigned fragility=effects::fragility(2998,12)->damage;
  check(w.npc_combat_snapshot(target)->health->current==967-(bleed?fragility:0));
  check(w.has_condition(target,combat::conditions::Kind::bleeding,1334)==bleed);
  w.tick(1534);check(launches(box->drain())==1);w.tick(1868);
  if(!bleed)check(w.npc_combat_snapshot(target)->health->current==934&&!w.has_condition(target,combat::conditions::Kind::bleeding,1868));
  const auto hp=w.npc_combat_snapshot(target)->health->current;w.tick(1868);check(w.npc_combat_snapshot(target)->health->current==hp&&launches(box->drain())==0);
 }
 std::cout<<"7Stone Daggers overcast/flesh/unknown/condition-disabled paired-projectile cases pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
