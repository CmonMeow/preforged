#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ethereal Light line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank)for(unsigned divine:{0u,12u}){
  const auto h=effects::direct_heal(959);check(h&&h->self&&h->range==1248&&h->divine_multiplier==1);
  check(effects::direct_heal_amount(959,rank,divine,100,1000)==25+5*rank+unsigned(std::lround(3.2*divine)));
 }
 check(!effects::direct_heal_amount(959,21,0,100,1000)&&!effects::direct_heal_amount(959,12,21,100,1000));
 combat::conditions::State clear;
 const combat::weapon::AttackActionFacts action{combat::weapon::AttackTargetAction::skill,Slot{959,0}};
 check(combat::weapon::reviewed_hit_interrupt(clear,action,combat::weapon::AttackOutcome::hit,100)==true);
 for(auto outcome:{combat::weapon::AttackOutcome::miss,combat::weapon::AttackOutcome::block,combat::weapon::AttackOutcome::dodge,combat::weapon::AttackOutcome::obstruction,combat::weapon::AttackOutcome::cancelled})
  check(combat::weapon::reviewed_hit_interrupt(clear,action,outcome,100)==false);
 // Real landed-hit control path,including native NPC action ownership.
 for(bool npc:{false,true})for(bool missed:{false,true}){
  warrior_test::Fixture f(320,npc);auto& w=f.world;
  std::optional<combat::npcs::control::Token> token;
  if(npc){token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,200,5000,Slot{959,22});check(bool(token));}
  else check(w.cast_activity(f.target,Slot{959,22},200,5000));
  if(missed)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,5000));
  const auto epoch=npc?std::optional<std::uint64_t>{}:w.control_epoch(f.target);
  check(w.ordinary_attack(f.source,f.target.actor,1000));w.tick(2200);
  if(npc)check(w.complete_npc_action(*token,5000)==missed);
  else check((w.control_epoch(f.target)!=epoch)==!missed);
 }
 for(unsigned mode=0;mode<7;++mode){
  warrior_test::Fixture f(320,false);auto& w=f.world;w.set_vitals(2,{500,1000,30,30},100);
  unsigned heals=0;std::vector<world::Packet> wire;casting::Hooks h;
  h.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};
  h.resources=[&](auto id){return w.resources(id);};h.consume_energy_at=[&](auto cost,auto at){check(cost==5);return w.consume_energy(2,cost,f.target,at);};
  h.target_valid=[](auto id,float range,bool self){return id==2&&range==1248&&self;};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.heal_scoped=[&](auto target,auto amount,auto range,auto self,auto at){++heals;check(amount==85);return w.heal_target_scoped(f.target,target,amount,range,self,at);};
  h.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(w.cast_activity(f.target,slot,begins,ends));};
  h.control_epoch=[&](){return w.control_epoch(f.target);};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(f.target,skill,event,at);};
  Policy p;p.primary=3;p.campaigns=255;p.learned[959]=1;Slots bar{};bar[0]={959,0};attributes::Ranks ranks{};ranks[13]=12;
  casting::Runtime cast(2);check(casting::Runtime::effect_supported(959));
  check(cast.start(1000,{959,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);check(w.resources(2)->energy==25);
  if(mode==1||mode==2||mode==3||mode==6){
   if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,1100,5000));
   if(mode==3)check(w.status_mutation(f.target,17,123,12,1100,1100+timing::stance(17,12)->duration,true));
   if(mode==6)check(w.status_mutation(f.target,16,123,12,1100,1100+timing::stance(16,12)->duration,true));
   check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(320,0),1500));
  }
  if(mode==4)cast.cancel(h);if(mode==5)cast.interrupt(1500,h);
  cast.update(1999,h);check(!heals);cast.update(2000,h);check(!cast.active());
  const bool succeeds=mode==0||mode==2||mode==3||mode==6;check(heals==unsigned(succeeds));
  check(std::count(wire.begin(),wire.end(),*casting::skill_success_body_effect(2,959))==unsigned(succeeds));
  if(mode==3)check(w.resources(2)->energy==20);
  if(mode==6){const auto status=*w.status_snapshot(f.target,2000);check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==16;}));}
  if(succeeds){check(std::count(wire.begin(),wire.end(),casting::slot_event(228,2,{959,0},5))==1);check(cast.start(2100,{959,0,2},bar,p,ranks,true,h)==casting::Reason::busy);check(cast.start(2750,{959,0,2},bar,p,ranks,true,h)==casting::Reason::recharge);}
 }
 std::cout<<"PASS Ethereal Light rank healing/Divine Favor,Runtime cost/timing/body,recharge,cancel,ordinary player/NPC landed-hit interruption,miss,Resolve Energy charge and Concentration consumption\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

