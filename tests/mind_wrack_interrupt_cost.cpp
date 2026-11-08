#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mind Wrack interrupt cost line "+std::to_string(at.line()));}
int main()try{
 const std::string hash(64,'a');std::istringstream a("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 70 0 0 - -\n");std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/mind_wrack_interrupt_cost.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");auto pop=world::load_npc_catalog(a);world::bind_combat_catalog(pop,combat::npcs::load_catalog(m,hash),hash);
 for(bool panic:{false,true})for(unsigned hp:{50u,80u,500u})for(unsigned energy:{0u,1u,5u}){world::Instance w;w.configure_npcs(pop.for_map(146));for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:80,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==2?hp:500u,500,id==2?energy:100u,100},100);}const auto caster=*w.effect_scope(1),victim=*w.effect_scope(2),trigger=*w.effect_scope(3),npc=*w.effect_scope(100);
  check(w.energy_capacity(2,5,0,100,{80,-2})&&w.apply_mind_wrack(caster,victim,12,false,100)&&w.apply_mind_wrack(victim,npc,12,false,100));check(w.status_mutation(victim,17,900,12,100,10100,true)&&w.cast_activity(victim,82,100,2100));const auto token=w.begin_npc_action(npc,combat::npcs::control::Action::spell,100,2100);check(bool(token));
  if(panic){check(w.apply_panic(caster,trigger,0,100));check(w.panic_skill_used({trigger,82,1,100,200,true}));}
  else check(w.cry_frustration(caster,victim,*cry_frustration(12),20,200));
  const unsigned primary=panic?0:63,burst=energy?83:0;const unsigned wanted=hp>primary+burst?hp-primary-burst:0;
  check(w.resources(2)->health==wanted&&w.resources(2)->energy==0);check(w.npc_combat_snapshot(npc)->health->current==(panic?500.:437.));
  if(wanted)check(w.casting_skill(victim,200)==82);else check(!w.casting_skill(victim,200));
  // Let the short Panic expire so its unrelated hex flag cannot conceal a
  // restored Mind Wrack belonging to the dead victim in an NPC copy.
  w.tick(1100);check(bool(w.npc_combat_snapshot(npc)->effects&0x800)==bool(wanted));check(!w.complete_npc_action(*token,2100));
 }
 for(unsigned mode=0;mode<4;++mode){world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==2?80u:500u,500,id==2?5u:100u,100},100);}const auto caster=*w.effect_scope(1),victim=*w.effect_scope(2);check(w.energy_capacity(2,5,0,100,{80,-2})&&w.apply_mind_wrack(caster,victim,12,false,100)&&w.status_mutation(victim,17,900,12,100,10100,true));
  if(mode==1)check(w.attack_activity(victim,500,100));else check(w.cast_activity(victim,82,100,2100));
  if(mode==0)check(!w.interrupt_cast(victim,82,200));if(mode==1)check(!w.interrupt_attack(victim,200));if(mode==2)check(w.interrupt_skill(caster,victim,*interrupt_skill(23,0),20,200));
  if(mode==3){check(w.degeneration_hex(caster,victim,*degeneration_hex(43,0),100));check(!w.attack_activity(victim,500,200));}
  check(w.resources(2)->health==0&&w.resources(2)->energy==0);
 }
 // Power Leak's own Energy loss is observed without protection or PvE
 // per-target-loss damage; target Energy3 falls to real zero.
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,id==2?3u:100u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(2,5,0,100,{80,-2})&&w.apply_mind_wrack(a,b,12,false,100)&&w.cast_activity(b,82,100,2100));check(w.interrupt_skill(a,b,*interrupt_skill(24,0),20,200));check(w.resources(2)->health==417&&w.resources(2)->energy==0&&w.status_snapshot(b,200)->empty());}
 std::cout<<"PASS Mind Wrack protected Cry/Panic lethal/nonlethal/zero/partial Energy, no HP resurrection or restored NPC hex, direct cast/attack/Clumsiness and Power Leak loss\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
