#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Earth Shaker cast line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills;using namespace combat::weapon;
 //0 normal,1 cancel,2 interrupt,3 recycled target,4 empty adrenaline,
 //5 wrong weapon,6 missing effect hook,7 low-rank fail,8 low-rank pass,
 //9 Blind miss,10 caster death.
 for(bool npc:{false,true})for(unsigned mode=0;mode<=10;++mode){
  if(npc&&mode==3)continue;
  warrior_test::Fixture f(355,npc);auto& w=f.world;
  Slots bar{};bar[0]={354,0};bar[1]={336,0};check(w.configure_adrenaline(f.source,bar,100));
  if(mode!=4)for(unsigned n=0;n<8;++n)check(w.melee_attack(f.source,f.target,f.weapon,*attack_skill(322,0),101+n));
  const auto before_pools=*w.adrenaline(f.source);check(before_pools.at(0).units==(mode==4?0u:200u));
  w.join(3,"Adjacent foe",226,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(3,{1000,1000,30,30},100);const auto adjacent=*w.effect_scope(3);
  // Price of Failure supplies a real miss reaction. Skill failure must not
  // damage the attacker or consume its independent accuracy RNG stream.
  if(mode==7||mode==8)check(w.apply_price_of_failure(adjacent,f.source,12,900));
  if(mode==9)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,900,10000));
  unsigned failure_draws=0,miss_draws=0;
  w.configure_earth_shaker_failure_draws([&]{++failure_draws;return mode==7?.499:.5;});
  w.configure_attack_miss_draws([&]{++miss_draws;return mode==9?0.:.99;});
  w.take_attack_outcomes(f.source);
  std::uint64_t clock=1000;unsigned commits=0,prepared=0,finalized=0,energy_spent=0;std::string phase;
  std::vector<world::Packet> events;casting::Hooks h;
  h.resources=[&](unsigned id){return w.resources(id);};
  h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
  h.diagnostic=[&](unsigned,const char* value){phase=value;};
  h.consume_energy_at=[&](unsigned amount,auto at){energy_spent+=amount;return w.consume_energy(1,amount,f.source,at);};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};
  h.weapon_snapshot=[&](){auto weapon=f.weapon;if(mode==5)weapon.item_type=27;return std::optional{weapon};};
  h.melee_target=[&](unsigned id,auto at){return w.melee_target(f.source,id,at,354);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){
   ++commits;check(plan.skill==354&&plan.rank==(mode==7||mode==8?4u:12u)&&plan.strength==0);
   return w.melee_attack(f.source,target,weapon,plan,at);
  };
  h.attack_activity=[&](auto until){w.attack_activity(f.source,until,clock);};
  h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(f.source);};
  h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(f.source,slot,at);};
  h.reconcile_adrenaline=[&](auto at){w.reconcile_adrenaline(f.source,at);};
  h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};
  h.attack_duration=[&](auto base,auto at){return w.attack_duration(f.source,base,at);};
  // Exercise the prepared skill-use/cooldown transaction, not a fake hook
  // which only reports successful damage to Runtime.
  h.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(w.cast_activity(f.source,slot,begins,ends));};
  h.control_epoch=[&](){return w.control_epoch(f.source);};
  h.control_event_time=[&](auto after){return w.control_event_time(f.source,after);};
  h.prepare_skill_use=[&](auto slot,auto event,auto at){++prepared;return w.prepare_diversion_skill_use(f.source,slot,event,at);};
  h.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool instant){++finalized;return w.finalize_diversion_skill_use(f.source,event,at,base,existing,instant);};
  h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(f.source,event);};
  h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(f.source,event);};
  h.cooldown_control_source=[&](auto after){return w.cooldown_control_source(f.source,after);};
  h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
  h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(f.source,slot,at);};
  if(mode==6)h.melee_attack={};
  skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[354]=policy.learned[336]=1;
  attributes::Ranks ranks{};ranks[19]=mode==7||mode==8?4:12;casting::Runtime runtime(1);
  const auto admission=runtime.start(clock,{354,0,f.target.actor},bar,policy,ranks,true,h);
  const auto expected=mode==4?casting::Reason::adrenaline:mode==5?casting::Reason::weapon:mode==6?casting::Reason::unsupported:casting::Reason::accepted;
  if(admission!=expected)std::cerr<<"npc="<<npc<<" mode="<<mode<<" admission="<<casting::reason_text(admission)<<'\n';
  check(admission==expected&&energy_spent==0);
  if(expected!=casting::Reason::accepted){check(!runtime.active()&&!commits&&!prepared&&!failure_draws);check(w.adrenaline(f.source)->at(0).units==before_pools.at(0).units);continue;}
  const auto paid=*w.adrenaline(f.source);check(paid.at(0).units==0);
  check(paid.at(1).units==before_pools.at(1).units-25);
  check(std::count(events.begin(),events.end(),casting::activation(1,f.target.actor,354,true))==1);
  clock=1200;
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,30,30},100);check(w.physical_armor(*w.effect_scope(2),60));}
  if(mode==10)check(w.kill(f.source,clock));
  clock=1499;runtime.update(clock,h);check(commits==0&&failure_draws==0);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(clock,h);
  clock=1500;w.tick(clock);const double before=f.health();const unsigned caster_before=w.resources(1)->health;
  f.box->drain();runtime.update(clock,h);check(!runtime.active());
  const bool committed=mode==0||mode==7||mode==8||mode==9;
  const bool hit=mode==0||mode==8;
  if(commits!=unsigned(committed)||finalized!=unsigned(committed))std::cerr<<"npc="<<npc<<" mode="<<mode<<" commits="<<commits<<" prepared="<<prepared<<" finalized="<<finalized<<" phase="<<phase<<'\n';
  check(commits==unsigned(committed)&&prepared==unsigned(committed)&&finalized==unsigned(committed));
  check(hit?f.health()<before:f.health()==before);
  check(w.knocked_down(adjacent,clock)==hit&&w.resources(3)->health==1000);
  check(w.knocked_down(f.target,clock)==hit);
  check(failure_draws==unsigned(mode==7||mode==8));
  check(miss_draws==unsigned(mode==8||mode==9));
  if(mode==7)check(w.resources(1)->health==caster_before&&w.adrenaline(f.source)->at(0).units==0);
  if(hit)check(w.adrenaline(f.source)->at(0).units==25);
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==unsigned(committed));
  if(committed)check(outcomes[0].target==f.target&&outcomes[0].outcome==(mode==7?AttackOutcome::cancelled:mode==9?AttackOutcome::miss:AttackOutcome::hit));
  check(std::count(events.begin(),events.end(),casting::generic(46,1,0))==unsigned(committed));
  check(std::count(events.begin(),events.end(),casting::generic(58,1,0))==0);
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(f.target.actor,1,354))==unsigned(committed));
 }
 std::cout<<"PASS Earth Shaker Runtime/prepared World: adrenaline, primary/area hit, independent failure, miss reactions, cancel/interrupt/recycle/death and admission guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}



