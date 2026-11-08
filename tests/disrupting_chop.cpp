#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Disrupting Chop line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={281,0};check(w.configure_adrenaline(f.target,bar,100));
  casting::Runtime target(2);casting::Hooks h;unsigned heals=0;
  h.resources=[&](auto id){return w.resources(id);};h.events=[](const auto&){};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto id,float,bool){return id==2;};
  h.heal=[&](auto,auto,float,bool){++heals;return true;};h.consume_energy_at=[&](auto n,auto at){return w.consume_energy(2,n,f.target,at);};
  h.cast_slot_activity=[&](auto slot,auto begin,auto end){check(w.cast_activity(f.target,slot,begin,end));};
  h.control_epoch=[&](){return w.control_epoch(f.target);};h.control_event_time=[&](auto after){return w.control_event_time(f.target,after);};
  h.cooldown_control_source=[&](auto after){return w.cooldown_control_source(f.target,after);};
  h.settle_attack_interrupt=[&](const auto& origin,auto slot,auto base,auto now){return w.settle_pending_attack_interrupt(origin,slot,base,now);};
  h.skill_disabled_slot=[&](auto slot,auto now){return w.skill_disabled_slot(f.target,slot,now);};h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
  Policy p;p.primary=3;p.campaigns=255;p.learned[281]=1;attributes::Ranks ranks{};
  check(target.start(1000,{281,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
  if(mode==1)check(w.status_mutation(f.target,16,123,12,1000,1000+timing::stance(16,12)->duration,true));
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,1000,5000));
  if(mode==3){w.configure_attack_block_draws([]{return 0.;});check(w.status_mutation(f.target,448,124,12,1000,1000+*effects::escape(448,12),true));}
  const auto epoch=w.control_epoch(f.target);check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12,0),1200));
  check(f.health()==unsigned(mode>=2?1000:980));check((w.control_epoch(f.target)!=epoch)==(mode==0));
  target.update(2500,h);check(heals==unsigned(mode!=0));
  check(w.skill_disabled_slot(f.target,{281,0},2500)==unsigned(mode==0?23200:0));
  if(mode==0){const auto control=*w.cooldown_control_source(f.target,*epoch);check(control.attack_disable&&control.attack_disable->attack==340);}
 }
 for(bool tracked:{false,true}){
  warrior_test::Fixture f(340,true,1000,true,true,1,combat::npcs::Energy{50,50});auto& w=f.world;
  std::optional<combat::npcs::control::Token> token;
  if(tracked){check(w.configure_npc_spellbook(f.target,{{{194,1},1248,"reviewed Flare"}}));token=w.begin_npc_spell(f.target,f.source,1000);}
  else token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,1000,2000,Slot{194,1});
  check(token.has_value());
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12,0),1200)==tracked);
  check(f.health()==unsigned(tracked?980:1000));
  check(w.skill_disabled_slot(f.target,{194,1},1200)==unsigned(tracked?21200:0));
  if(tracked){check(!w.complete_npc_action(*token,2000));check(w.begin_npc_spell(f.target,f.source,21200).has_value());}
 }
 for(unsigned mode=0;mode<3;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  for(unsigned n=0;n<6;++n)check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(322,0),101+n));
  check(w.adrenaline(f.source)->front().units==150);const auto before=f.health();
  check(w.cast_activity(f.target,281,1000,3000));
  casting::Runtime source(1);casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};h.events=[](const auto&){};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.weapon_snapshot=[&](){return std::optional{f.weapon};};
  h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,340);};
  h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){return w.melee_attack(f.source,target,gear,plan,at);};
  h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};
  h.consume_energy_at=[&](auto amount,auto at){check(amount==0);return w.consume_energy(1,amount,f.source,at);};
  h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(f.source,slot,at);};h.reconcile_adrenaline=[&](auto at){check(w.reconcile_adrenaline(f.source,at));};
  Policy p;p.primary=1;p.campaigns=255;p.learned[340]=1;attributes::Ranks ranks{};ranks[18]=12;
  check(source.start(1000,{340,0,2},bar,p,ranks,true,h)==casting::Reason::accepted&&w.adrenaline(f.source)->front().units==0);
  if(mode==1)source.cancel(h);if(mode==2)source.interrupt(1200,h);
  source.update(1500,h);check((f.health()<before)==(mode==0));
  check(w.adrenaline(f.source)->front().units==unsigned(mode==0?25:0));
 }
 auto forged=*combat::weapon::attack_skill(340,12);++forged.bonus;check(!combat::weapon::valid(forged));
 std::cout<<"PASS Disrupting Chop real player contact/Runtime settlement,protection/miss/block suppression,NPC tracked spell interrupt and health-preserving commit,unknown controller rejection\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
