#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("World attack disable line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{340u,399u})for(unsigned mode=0;mode<5;++mode){
  warrior_test::Fixture f(390,false);auto& w=f.world;
  check(w.cast_activity(f.target,skills::Slot{281,1},100,1000));
  const auto before=*w.disable_reset_revision(f.target);const auto epoch=*w.control_epoch(f.target);
  if(mode!=1)check(w.interrupt_cast(f.target,281,500));
  const auto plan=*combat::weapon::interrupt_disable_plan(skill);
  combat::weapon::InterruptedActionReceipt receipt{f.target,{281,1},epoch+1,100,1000,500,8500};
  if(mode==2)receipt.slot.copy=0;if(mode==3)++receipt.action;if(mode==4)++receipt.target.generation;
  check(w.settle_attack_interrupt_disable(f.target,plan,receipt)==(mode==0));
  if(mode){check(w.skill_disabled_slot(f.target,{281,1},500)==0);continue;}
  check(w.skill_disabled_slot(f.target,{281,1},500)==28500);
  check(w.skill_disabled_slot(f.target,{281,0},500)==0&&w.skill_disabled_until(f.target,281,500)==0);
  check(!w.settle_attack_interrupt_disable(f.target,plan,receipt));
  check(!w.reset_skill_disables(f.target,{{281,1}},before,600));
  const auto current=*w.disable_reset_revision(f.target);check(current[4]!=before[4]);
  check(w.reset_skill_disables(f.target,{{281,0}},current,600));
  check(w.skill_disabled_slot(f.target,{281,1},600)==28500);
  check(w.reset_skill_disables(f.target,{{281,1}},*w.disable_reset_revision(f.target),700));
  check(w.skill_disabled_slot(f.target,{281,1},700)==0);
  w.leave(2);w.join(2,"New lifetime",60,0,std::make_shared<world::Outbox>(),0,{}, {},800,2);
  const auto next=*w.effect_scope(2);check(next!=f.target);
  check(!w.skill_disabled_slot(f.target,{281,1},800));check(w.skill_disabled_slot(next,{281,1},800)==0);
  check(!w.settle_attack_interrupt_disable(next,plan,receipt));
 }
 for(unsigned attack:{340u,399u}){
  warrior_test::Fixture f(390,true,1000,true,true,1,combat::npcs::Energy{50,50});auto& w=f.world;
  check(w.configure_npc_spellbook(f.target,{{{194,1},1248,"authored reviewed Flare"}}));
  const auto token=w.begin_npc_spell(f.target,f.source,1000);check(token.has_value());
  const auto plan=*combat::weapon::interrupt_disable_plan(attack);auto invalid=plan;--invalid.extra;
  check(!w.interrupt_npc_skill_with_disable(f.target,*token,invalid,1200));
  check(w.interrupt_npc_skill_with_disable(f.target,*token,plan,1200));
  check(!w.interrupt_npc_skill_with_disable(f.target,*token,plan,1200));
  check(w.skill_disabled_slot(f.target,{194,1},1200)==21200&&w.skill_disabled_slot(f.target,{194,0},1200)==0);
  check(!w.complete_npc_action(*token,token->completes));
  check(!w.begin_npc_spell(f.target,f.source,1201)&&!w.begin_npc_spell(f.target,f.source,21199));
  const auto next=w.begin_npc_spell(f.target,f.source,21200);check(next&&next->skill==skills::Slot{194,1});
 }
 std::cout<<"PASS World player/NPC exact action/copy receipts,disable read,AI admission/expiry,reset revision race,selective reset and reconnect retirement;contact producers unbound\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
