#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Distracting Shot line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank:{0u,12u,20u})for(int armor:{0,60,120}){
  warrior_test::Fixture f(399,false);check(f.world.physical_armor(f.target,armor));
  f.weapon.damage.minimum=f.weapon.damage.maximum=200;f.weapon.mastery=0;f.weapon.customized=true;
  check(f.world.ordinary_weapon(f.source,f.weapon,20));check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::weakness,100,5000));
  check(f.world.melee_target(f.source,2,1000,399));check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,rank),1000));
  check(f.health()==1000);f.world.tick(1100);check(f.health()==999-rank);
 }
 for(unsigned mode=0;mode<7;++mode){
  warrior_test::Fixture f(399,false);auto& w=f.world;Slots bar{};bar[0]={281,0};check(w.configure_adrenaline(f.target,bar,100));
  if(mode==4)check(w.apply_condition(f.source,f.target,combat::conditions::Kind::dazed,100,5000));
  if(mode==6)check(w.physical_armor(f.target,{}));
  casting::Runtime target(2);casting::Hooks h;unsigned heals=0;
  h.resources=[&](auto id){return w.resources(id);};h.events=[](const auto&){};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.target_valid=[](auto id,float,bool){return id==2;};h.heal=[&](auto,auto,float,bool){++heals;return true;};
  h.consume_energy_at=[&](auto n,auto at){return w.consume_energy(2,n,f.target,at);};
  h.cast_slot_activity=[&](auto slot,auto begin,auto end){check(w.cast_activity(f.target,slot,begin,end));};
  h.control_epoch=[&](){return w.control_epoch(f.target);};h.control_event_time=[&](auto after){return w.control_event_time(f.target,after);};
  h.cooldown_control_source=[&](auto after){return w.cooldown_control_source(f.target,after);};
  h.settle_attack_interrupt=[&](const auto& origin,auto slot,auto base,auto now){return w.settle_pending_attack_interrupt(origin,slot,base,now);};
  h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(f.target,slot,at);};h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
  Policy p;p.primary=3;p.campaigns=255;p.learned[281]=1;attributes::Ranks ranks{};
  if(mode!=5)check(target.start(1000,{281,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
  if(mode==1)check(w.status_mutation(f.target,16,123,12,1000,1000+timing::stance(16,12)->duration,true));
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,1000,5000));
  if(mode==3){w.configure_attack_block_draws([]{return 0.;});check(w.status_mutation(f.target,448,124,12,1000,1000+*effects::escape(448,12),true));}
  const auto epoch=*w.control_epoch(f.target);check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),1200));
  if(mode==5)check(target.start(1210,{281,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
  check(f.health()==1000);w.tick(1300);target.update(2500,h);
  check(f.health()==unsigned(mode==2||mode==3?1000:987));
  const bool extra=mode==0||mode==5||mode==6;
  const auto control=w.cooldown_control_source(f.target,epoch);
  if(extra)check(control&&control->attack_disable&&control->attack_disable->attack==399&&w.skill_disabled_slot(f.target,{281,0},2500)==control->stamp->at+22000);
  else check(w.skill_disabled_slot(f.target,{281,0},2500)==0);
  check(heals==unsigned(mode==1||mode==2||mode==3)); // Dazed interrupts,without extra disable.
 }
 for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(399,true,1000,true,true,1,combat::npcs::Energy{50,50});auto& w=f.world;
  if(mode==2)check(f.condition(combat::conditions::Kind::dazed,100,5000));
  std::optional<combat::npcs::control::Token> token;
  if(mode==3)token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,1000,2000,Slot{194,1});
  else if(mode!=1){check(w.configure_npc_spellbook(f.target,{{{194,1},1248,"reviewed Flare"}}));token=w.begin_npc_spell(f.target,f.source,1000);check(token.has_value());}
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),1200));w.tick(1300);
  check(f.health()==unsigned(mode==3?1000:987));
  const auto disabled=*w.skill_disabled_slot(f.target,{194,1},1300);check((disabled>1300)==(mode==0));
  if(mode==0){check(!w.complete_npc_action(*token,token->completes));check(w.begin_npc_spell(f.target,f.source,disabled).has_value());}
 }
 for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(399,false);auto& w=f.world;std::uint64_t clock=1000;unsigned releases=0;std::vector<world::Packet> events;
  if(mode==3)check(w.status_mutation(f.source,346,123,0,1000,9000,true));
  const auto windup=*w.attack_duration(f.source,250,1000);check(mode==3?windup<250:windup==250);
  casting::Runtime source(1);casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};
  h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.consume_energy_at=[&](auto n,auto at){check(n==3||n==5);return w.consume_energy(1,n,f.source,at);};
  h.weapon_snapshot=[&](){return std::optional{f.weapon};};h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,399);};
  h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++releases;return w.melee_attack(f.source,target,gear,plan,at);};
  h.attack_activity=[&](auto until){w.attack_activity(f.source,until,clock);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};
  h.attack_duration=[&](auto base,auto at){return w.attack_duration(f.source,base,at);};
  Slots bar{};bar[0]={399,0};bar[1]={403,0};Policy p;p.primary=2;p.campaigns=255;p.learned[399]=p.learned[403]=1;attributes::Ranks ranks{};ranks[23]=12;
  check(source.start(1000,{399,0,2},bar,p,ranks,true,h)==casting::Reason::accepted&&w.resources(1)->energy==27);
  const auto release_at=1000+windup;
  if(mode==1)source.cancel(h);if(mode==2)source.interrupt(1100,h);
  clock=release_at-1;source.update(clock,h);check(!releases);clock=release_at;source.update(clock,h);
  check(releases==unsigned(mode==0||mode==3));w.tick(release_at+100);check(f.health()==unsigned(mode==0||mode==3?987:1000));
  check(std::find(events.begin(),events.end(),casting::terminal(46,1))==events.end());
  if(mode==0||mode==3){
   clock=release_at+749;check(source.start(clock,{403,0,2},bar,p,ranks,true,h)==casting::Reason::busy);
   clock=release_at+750;check(source.start(clock,{403,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);source.cancel(h);
  }
 }
 combat::conditions::State conditions;combat::weapon::AttackActionFacts easy{combat::weapon::AttackTargetAction::skill,Slot{400,0}};
 check(combat::weapon::distracting_disable_eligible(conditions,easy,100)==false);
 check(!combat::weapon::distracting_shot(21));auto forged=*combat::weapon::attack_skill(399,12);++forged.fixed_damage;check(!combat::weapon::valid(forged));
 std::cout<<"PASS Distracting Shot fixed damage/armor/Weakness,impact-time player Runtime disable,protection/miss/block,Dazed/easily-interrupted precedence,NPC tracked/idle/untracked recipients and expiry\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

