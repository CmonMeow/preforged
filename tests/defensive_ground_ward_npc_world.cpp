#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
using K=combat::npcs::control::Knockdown;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ward938 NPC line "+std::to_string(at.line())+" "+context);
}
world::Instance::Npc body(unsigned mode=0,float x=100){
 world::Instance::Npc n;n.id=n.definition.id=100;n.x=x;n.plane=mode==6?1:0;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
 d.relation=mode==5?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
 d.spawn_health={1000,1000};d.damageable=d.conditions_allowed=true;d.knockdown_allowed=mode==2?false:true;
 if(mode==1||mode==9)d.knockdown_allowed.reset();else d.knockdown_evidence="authored control fixture";
 if(mode!=4){d.creature_type=mode==3?"spirit":"human";d.creature_type_evidence="authored recipient fixture";}
 n.combat_definition=d;return n;
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope owner,npc;
 Fixture(unsigned mode=0,float x=100){w.configure_npcs({body(mode,x)});
  w.join(1,"Ward owner",mode==7?900:0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  owner=*w.effect_scope(1);npc=*w.effect_scope(100);check(w.apply_defensive_ward(owner,*defensive_ground_ward_plan(938,0),100));box->drain();
 }
 void group(){auto enemy=body(5,1000);enemy.id=enemy.definition.id=101;enemy.combat_definition->binding.actor=enemy.combat_definition->binding.definition=101;
  check(w.spawn_kill_encounter(owner,75,1,{enemy},[]{return true;}));const std::array allies{npc};check(w.bind_npc_opposed_allies({owner,75,1},allies));box->drain();}
};
int main()try{unsigned cases=0;
 for(unsigned mode=0;mode<13;++mode){context="static mode="+std::to_string(mode);Fixture f(mode,mode==9?500:mode==11?322:mode==12?std::nextafter(322.f,1000.f):100);
  const auto at=mode==8?10100u:1000u;if(mode==10)f.w.leave(1);
  const bool protected_now=mode==0||mode==1||mode==2||mode==11;
  check(f.w.ward_knockdown_protection(f.npc,at)==protected_now);
  const auto result=f.w.knockdown_npc(f.npc,2000,at);const auto expected=protected_now?K::immune:mode==9?K::unknown:K::accepted;
  check(result==expected);check(f.w.knocked_down(f.npc,at)==(expected==K::accepted));
  if(mode!=10){const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::knockdown_2022(100,2.f))==int(expected==K::accepted));}
  ++cases;
 }
 {context="protected action survives";Fixture f;const auto token=f.w.begin_npc_action(f.npc,combat::npcs::control::Action::spell,1000,2000);check(bool(token));
  check(f.w.knockdown_npc(f.npc,2000,1100)==K::immune);check(f.w.complete_npc_action(*token,2000));++cases;}
 for(bool entering:{false,true}){context=entering?"entering moving NPC":"exiting moving NPC";Fixture f(0,entering?600:100);f.group();
  check(f.w.route_npc_opposed_ally({f.owner,75,1},f.npc,entering?100:600,0,0,200,0,100));
  // No World tick between these observations: query/control must materialize
  // the same retained route at their actual event timestamps.
  if(entering){check(!f.w.ward_knockdown_protection(f.npc,1100));check(f.w.ward_knockdown_protection(f.npc,1600));
   check(f.w.knockdown_npc(f.npc,2000,1600)==K::immune);check(!f.w.knocked_down(f.npc,1600));
  }else{check(f.w.ward_knockdown_protection(f.npc,1100));check(f.w.knockdown_npc(f.npc,2000,1100)==K::immune);
   check(!f.w.ward_knockdown_protection(f.npc,1300));check(f.w.knockdown_npc(f.npc,2000,1300)==K::accepted);check(f.w.knocked_down(f.npc,1300));}
  ++cases;
 }
 std::cout<<cases<<" Ward938 actual NPC control, protected action, eligibility and moving entry/exit cases passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
