#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Watchful NPC cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<11;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  n.definition.level=20;combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,1000};
  d.spawn_energy=combat::npcs::Energy{mode==1?4u:40u,40};d.energy_pips=0;
  d.damageable=d.conditions_allowed=true;d.cold_armor=60;n.combat_definition=d;w.configure_npcs({n});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Player",0,0,box,0,{}, {},100,1);
  w.set_vitals(1,{1000,1000,100,100},100);const auto player=*w.effect_scope(1),source=*w.effect_scope(100);
  if(mode!=2)check(w.configure_npc_spellbook(source,{{{1392,0},1248,"authored native self-cast fixture"}}));
  auto plan=*skills::effects::watchful_healing(12);if(mode==3)++plan.gain;
  auto caller=source;if(mode==4)++caller.generation;
  box->drain();const auto token=w.begin_npc_watchful(caller,plan,1000);
  check(bool(token)==(mode==0||mode>=5));if(!token)continue;
  check(token->skill->skill==1392&&token->completes==2000);
  check(w.npc_energy_snapshot(source)->current==35);
  check(!w.begin_npc_watchful(source,plan,1000));check(!w.complete_npc_watchful(*token,1999));
  if(mode==5){check(w.interrupt_skill(player,source,*skills::effects::interrupt_skill(23,0),20,1500));check(!w.complete_npc_watchful(*token,2000));continue;}
  if(mode==9){check(w.damage_npc(player,source,{combat::Kind::armor_ignoring,1000,20,133},1248,1500).killed);
   check(!w.complete_npc_watchful(*token,2000));continue;}
  if(mode==10)check(!w.complete_npc_action(*token,2000)); // Generic completion cannot bypass the typed effect.
  if(mode==6){auto forged=*token;++forged.sequence;check(!w.complete_npc_watchful(forged,2000));}
  box->drain();check(w.complete_npc_watchful(*token,2000));check(!w.complete_npc_watchful(*token,2000));
  auto packets=box->drain();
  const auto finish=combat::detail::integer(58,100,0);
  const auto visual=*skills::particles::targeted_body_2022(100,100,1392);
  const auto finished=std::find(packets.begin(),packets.end(),finish),targeted=std::find(packets.begin(),packets.end(),visual);
  const auto added=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>2&&p[0]==65;});
  check(finished!=packets.end()&&targeted!=packets.end()&&added!=packets.end()&&finished<targeted&&targeted<added);
  if(mode==7){check(!w.begin_npc_watchful(source,plan,2750));w.tick(12000);check(w.npc_combat_snapshot(source)->health->current==160);
   const auto again=w.begin_npc_watchful(source,plan,12000);check(bool(again)&&w.npc_energy_snapshot(source)->current==30);
   check(w.complete_npc_watchful(*again,13000));continue;}
  if(mode==8){std::vector<world::Packet> visuals;
   check(w.stage_chilblains(player,source,*skills::effects::chilblains(12),20,2000,&visuals));
   check(std::abs(w.npc_combat_snapshot(source)->health->current-(202.-skills::effects::chilblains(12)->damage))<1e-9);
  }else{
   check(w.enchantment_removal(player,source,*skills::effects::enchantment_removal(68,0),20,2000));
   check(w.npc_combat_snapshot(source)->health->current==202);
  }
  packets=box->drain();const auto gain=skills::particles::health_change_2022(100,100,102,1000);
  const auto gained=std::find(packets.begin(),packets.end(),gain);
  const auto removed=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67;});
  check(gained!=packets.end()&&removed!=packets.end()&&gained<removed);
  if(mode==8){const auto damage=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>6&&p[0]==162&&p[2]==16;});check(damage!=packets.end()&&damage<gained);}
  check(!w.begin_npc_watchful(source,plan,2750)); // Recharge, aftercast already elapsed.
  w.tick(3000);packets=box->drain();check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67;}));
 }
 std::cout<<"PASS staged NPC Watchful paid cast,scope,capability,cancel,recharge,expiry,World drain and Chilblains end feedback\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
