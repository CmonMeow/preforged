#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Watchful scheduler line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<8;++mode){
  world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id=100;id<(mode==5?102u:101u);++id){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{mode==6?1.:100.,1000};
   d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;d.damageable=d.conditions_allowed=true;d.cold_armor=60;
   n.combat_definition=d;npcs.push_back(n);
  }
  w.configure_npcs(npcs);auto box=std::make_shared<world::Outbox>();w.join(1,"Player",0,0,box,0,{}, {},100,1);
  w.set_vitals(1,{1000,1000,100,100},100);const auto player=*w.effect_scope(1),source=*w.effect_scope(100);
  const auto plan=*skills::effects::watchful_healing(12);
  check(w.configure_npc_spellbook(source,{{{1392,0},1248,"authored scheduler fixture"}}));
  const auto token=w.begin_npc_watchful(source,plan,1000);check(bool(token));
  if(mode==5){const auto other=*w.effect_scope(101);check(w.configure_npc_spellbook(other,{{{1392,0},1248,"authored second caster"}}));check(bool(w.begin_npc_watchful(other,plan,1100)));}
  box->drain();
  if(mode==6)check(w.condition_spell(player,source,*skills::effects::condition_spell(131,0),1100));
  if(mode==2)check(w.interrupt_skill(player,source,*skills::effects::interrupt_skill(23,0),20,1500));
  if(mode==3)check(w.damage_npc(player,source,{combat::Kind::armor_ignoring,1000,20,133},1248,1500).killed);
  if(mode==4){check(w.damage_npc(player,source,{combat::Kind::armor_ignoring,20,20,133},1248,2050).reason==combat::Reason::accepted);}
  if(mode==7)check(w.complete_npc_watchful(*token,13000)); // Explicit late completion also retains the original deadline.
  if(mode==0)for(unsigned at=1100;at<=13000;at+=100)w.tick(at);
  else w.tick(13000);
  const auto packets=box->drain();
  const auto count=[&](unsigned opcode){return std::count_if(packets.begin(),packets.end(),[&](const auto& p){return p.size()>2&&p[0]==opcode&&(opcode!=65||(p.size()>=8&&p[6]==(1392&255)&&p[7]==(1392>>8)));});};
  check(count(65)==(mode==2||mode==3||mode==6?0:mode==5?2:1));
  check(count(67)==(mode==2||mode==3||mode==6?0:mode==5?2:1));
  const auto actual=w.npc_combat_snapshot(source)->health->current;
  const double expected=mode==2?100.-skills::effects::interrupt_skill(23,0)->damage:mode==3||mode==6?0:mode==4?140:160;
  if(std::abs(actual-expected)>=1e-9)std::cerr<<"mode "<<mode<<" Health "<<actual<<" expected "<<expected<<'\n';
  check(std::abs(actual-expected)<1e-9);
  check(!w.complete_npc_watchful(*token,13000)); // Scheduler already consumed it (or its interruption/death).
  if(mode==5)check(std::abs(w.npc_combat_snapshot(*w.effect_scope(101))->health->current-160)<1e-9);
  if(mode==0||mode==1){check(w.npc_energy_snapshot(source)->current==35);
   const auto next=w.begin_npc_watchful(source,plan,13000);check(bool(next));w.tick(14000);
   check(w.npc_energy_snapshot(source)->current==30);const auto repeated=box->drain();
   check(std::count_if(repeated.begin(),repeated.end(),[](const auto& p){return p.size()>2&&p[0]==65;})==1);
  }
 }
 std::cout<<"PASS Watchful automatic deadline,dense/coarse ticks,interruption,death,intervening damage,multiple casters and recharge reuse\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
