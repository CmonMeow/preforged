#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Glyph cast line "+std::to_string(at.line()));}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope owner;
 casting::Runtime runtime{1};casting::Hooks h;attributes::Ranks ranks{};Policy policy;Slots bar{};std::uint64_t now=1000;unsigned commits=0,last_rank=0;
 Fixture(unsigned glyph){
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{1000,1000,100,100},100);w.set_vitals(2,{1000,1000,100,100},100);owner=*w.effect_scope(1);check(w.energy_capacity(1,6,0,100,{80,-2}));
  h.events=[&](const auto& batch){w.skill_event(1,batch);};h.resources=[&](auto id){return w.resources(id);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto,float,bool){return true;};h.projectile_target_valid=[](auto,float){return true;};
  h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(1,amount,owner,at);};
  h.consume_overcast_energy=[&](auto id,auto amount,auto event,auto at){return w.activate_overcast_energy(owner,id,amount,event,at);};
  h.apply_elemental_glyph=[&](unsigned effect,const auto& plan,auto at)->casting::EffectCompletion{std::vector<world::Packet> out;const bool ok=w.apply_elemental_glyph(owner,effect,plan,at,&out);return {ok,std::move(out)};};
  h.elemental_glyph_snapshot=[&](auto at){return w.elemental_glyph_snapshot(owner,at);};
  h.pay_elemental_glyph=[&](auto slot,auto cost,auto event,auto at,const auto& expected){return w.pay_elemental_glyph_use(owner,slot,cost,event,at,expected);};
  h.settle_elemental_glyph=[&](auto event,auto outcome,auto at){return w.settle_elemental_glyph_use(owner,event,outcome,at);};h.glyph_clock=[&]{return now;};
  h.elemental=[&](auto,const auto& p,auto)->casting::EffectCompletion{++commits;last_rank=p.rank;return {true,{}};};
  h.shadow_damage=[&](auto,const auto& p,auto){++commits;last_rank=p.rank;return true;};
  bar[0]={glyph,0};bar[1]={194,0};bar[2]={219,0};policy.primary=6;policy.campaigns=255;policy.learned[glyph]=policy.learned[194]=policy.learned[219]=1;ranks[10]=ranks[9]=12;
  if(glyph==203){
   h.prepare_skill_use=[&](auto slot,auto event,auto at){return w.prepare_diversion_skill_use(owner,slot,event,at);};
   h.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool instant){return w.finalize_diversion_skill_use(owner,event,at,base,existing,instant);};
   h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(owner,event);};
   h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(owner,event);};
   h.control_epoch=[&]{return w.control_epoch(owner);};h.control_event_time=[&](auto epoch){return w.control_event_time(owner,epoch);};
   h.cooldown_control_source=[&](auto after){return w.cooldown_control_source(owner,after);};
   h.cast_slot_activity=[&](auto slot,auto begin,auto end){check(w.cast_activity(owner,slot,begin,end));};
   h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(owner,slot,at);};h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
   h.finish_renewal_spell=[&](auto glyph,auto use,auto slot,auto outcome,auto at,auto base,auto existing,auto control){return w.finish_renewal_spell(owner,glyph,use,slot,outcome,at,base,existing,control);};
   check(w.configure_adrenaline(owner,bar,100));
  }
 }
 void start(unsigned skill){check(runtime.start(now,{skill,0,skill==198||skill==199||skill==203?1u:2u},bar,policy,ranks,true,h)==casting::Reason::accepted);}
 void update(std::uint64_t at){now=at;runtime.update(now,h);}
 void glyph(unsigned skill){start(skill);update(2000);check(w.elemental_glyph_snapshot(owner,2000).has_value());
  const auto packets=box->drain();const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish!=packets.end());
  const auto add=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&(p[0]==64||p[0]==65);});check(add!=packets.end()&&finish<add);
  if(skill==203){const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,1,366));check(body!=packets.end()&&finish<body&&body<add);}
 }
};
int main()try{
 for(unsigned glyph:{198u,199u}){
  Fixture f(glyph);f.glyph(glyph);f.ranks[9]+=glyph==198?2:1;f.now=3000;f.start(219);
  check(f.w.resources(1)->energy==(glyph==199?95u:90u));
  check(f.w.overcast_energy_snapshot(f.owner)->curve.burden_units()==(glyph==199?0u:5u*overcast::CeilingCurve::units_per_energy));
  check(f.w.elemental_glyph_snapshot(f.owner,3500)->remaining==(glyph==198?10u:1u));
  f.update(4500);check(f.commits==1&&f.last_rank==(glyph==198?14u:13u));
  const auto state=f.w.elemental_glyph_snapshot(f.owner,4500);check(glyph==199?!state:state&&state->remaining==9);
 }
 for(unsigned glyph:{198u,199u})for(bool interrupt:{false,true}){
  Fixture f(glyph);f.glyph(glyph);f.now=3000;f.start(194);f.now=3200;
  if(interrupt)f.runtime.interrupt(f.now,f.h);else f.runtime.cancel(f.h,f.now);
  check(!f.runtime.active()&&f.commits==0);
  const auto state=f.w.elemental_glyph_snapshot(f.owner,f.now);
  check(glyph==199?!state:state&&state->remaining==10); //198 interruption is explicitly unsupported, not a claim of consumption.
 }
 {Fixture f(199);f.glyph(199);f.now=3000;f.start(219);check(f.w.settle_resources(f.owner,3600));
  f.now=4000;f.runtime.interrupt(3200,f.h);check(!f.w.elemental_glyph_snapshot(f.owner,4000));
  check(f.runtime.start(8199,{219,0,2},f.bar,f.policy,f.ranks,true,f.h)==casting::Reason::recharge);
  f.now=8200;f.start(219);f.update(9700);check(f.commits==1);
 }
 {Fixture f(198);f.glyph(198);f.ranks[10]=14;
  for(unsigned i=0;i<10;++i){f.now=3000+i*2000;f.start(194);f.update(f.now+1000);}
  check(f.commits==10&&!f.w.elemental_glyph_snapshot(f.owner,22000));
 }
 {Fixture f(199);f.h.apply_elemental_glyph={};check(f.runtime.start(1000,{199,0,1},f.bar,f.policy,f.ranks,true,f.h)==casting::Reason::unsupported&&f.w.resources(1)->energy==100);}
 {Fixture f(199);f.glyph(199);f.h.pay_elemental_glyph={};check(f.runtime.start(3000,{194,0,2},f.bar,f.policy,f.ranks,true,f.h)==casting::Reason::unsupported&&f.w.resources(1)->energy==95);}
 {Fixture f(203);f.glyph(203);f.now=3000;f.start(219);f.update(4500);
  check(!f.w.elemental_glyph_snapshot(f.owner,4500)&&f.commits==1);
  auto state=*f.runtime.cooldown_source_snapshot(f.owner,4500,f.h);check(f.w.publish_local_cooldowns(f.owner,state));
  check(std::none_of(state.timers.begin(),state.timers.end(),[](const auto& t){return t.slot==Slot{219,0};}));
  f.now=5300;f.start(219);f.runtime.cancel(f.h,f.now);
 }
 {Fixture f(203);f.glyph(203);f.now=3000;f.start(219);f.now=3200;f.runtime.cancel(f.h,f.now);
  check(f.w.elemental_glyph_snapshot(f.owner,3200).has_value());f.now=3300;f.start(219);f.update(4800);check(!f.w.elemental_glyph_snapshot(f.owner,4800));
 }
 {Fixture f(203);f.glyph(203);check(f.w.diversion(*f.w.effect_scope(2),f.owner,*effects::diversion(12),2900));
  f.now=3000;f.start(219);f.update(4500);check(f.w.diversion_recharge_until(f.owner,{219,0},4500)==0);
  const auto statuses=*f.w.status_snapshot(f.owner,4500);check(std::none_of(statuses.begin(),statuses.end(),[](const auto& e){return e.skill==30;}));
  f.now=5300;f.start(219);f.runtime.cancel(f.h,f.now);
 }
 for(bool power_block:{false,true}){Fixture f(203);f.glyph(203);f.now=3000;f.start(219);
  if(power_block)check(f.w.interrupt_skill(*f.w.effect_scope(2),f.owner,*effects::interrupt_skill(5,12),20,3200));
  else check(f.w.interrupt_cast(f.owner,219,3200));
  check(f.w.settle_resources(f.owner,3600));f.update(4000);check(!f.runtime.active()&&!f.w.elemental_glyph_snapshot(f.owner,4000)&&f.commits==0);
  auto state=*f.runtime.cooldown_source_snapshot(f.owner,4000,f.h);check(f.w.publish_local_cooldowns(f.owner,state));
  check(std::none_of(state.timers.begin(),state.timers.end(),[](const auto& t){return t.slot==Slot{219,0};}));
  f.now=4100;if(power_block)check(f.runtime.start(f.now,{219,0,2},f.bar,f.policy,f.ranks,true,f.h)==casting::Reason::recharge);
  else{f.start(219);f.runtime.cancel(f.h,f.now);}
 }
 {Fixture f(203);f.h.finish_renewal_spell={};check(f.runtime.start(1000,{203,0,1},f.bar,f.policy,f.ranks,true,f.h)==casting::Reason::unsupported&&f.w.resources(1)->energy==100);}
 {Fixture f(203);f.glyph(203);f.now=3000;f.start(219);check(f.w.interrupt_cast(f.owner,219,3200));
  const auto finish=f.h.finish_renewal_spell;f.h.finish_renewal_spell=[](auto,auto,auto,auto,auto,auto,auto,auto)->std::optional<effects::RenewalTerminal>{return {};};
  f.update(5000);check(f.runtime.active()&&f.commits==0&&f.w.elemental_glyph_snapshot(f.owner,5000).has_value());
  f.h.finish_renewal_spell=finish;f.update(5100);check(!f.runtime.active()&&f.commits==0&&!f.w.elemental_glyph_snapshot(f.owner,5100));
 }
 {Fixture f(203);f.glyph(203);f.now=3000;f.start(219);
  const auto finish=f.h.finish_renewal_spell;
  f.h.finish_renewal_spell=[&](auto glyph,auto use,auto slot,auto outcome,auto at,auto base,auto existing,auto control){
   if(outcome==effects::GlyphOutcome::interrupted){
    // PowerBlock can arrive after Runtime's earlier disable query. Recreate
    // the captured cast activity only to deliver this deterministic race.
    check(f.w.cast_activity(f.owner,Slot{219,0},3100,6000));
    check(f.w.interrupt_skill(*f.w.effect_scope(2),f.owner,*effects::interrupt_skill(5,12),20,at));
   }
   return finish(glyph,use,slot,outcome,at,base,existing,control);
  };
  check(f.w.interrupt_cast(f.owner,219,3200));f.box->drain();f.update(4000);
  check(!f.runtime.active()&&f.commits==0&&!f.w.elemental_glyph_snapshot(f.owner,4000));
  const auto disabled=*f.w.skill_disabled_slot(f.owner,{219,0},4000);check(disabled>4000);
  const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{219,0},static_cast<unsigned>((disabled-4000+999)/1000)))!=packets.end());
 }
 for(unsigned attack:{340u,399u})for(bool delayed:{false,true})for(bool fail_first:{false,true}){
  Fixture f(203);f.glyph(203);f.now=3000;f.start(219);
  auto plan=*combat::weapon::interrupt_disable_plan(attack);if(fail_first)--plan.extra;
  unsigned settlements=0;
  f.h.finish_renewal_spell=[&](auto glyph,auto use,auto slot,auto outcome,auto at,auto base,auto existing,auto control){
   ++settlements;return f.w.finish_renewal_spell(f.owner,glyph,use,slot,outcome,at,base,existing,control,plan);
  };
  check(f.w.interrupt_cast(f.owner,219,3200));
  if(delayed)check(f.w.settle_resources(f.owner,3600));
  const auto at=delayed?4000u:3200u;f.update(at);
  if(fail_first){
   check(f.runtime.active()&&f.commits==0&&f.w.elemental_glyph_snapshot(f.owner,at).has_value());
   check(f.w.skill_disabled_slot(f.owner,{219,0},at)==0);
   plan=*combat::weapon::interrupt_disable_plan(attack);f.update(at+100);
  }
  const auto settled=at+(fail_first?100:0);
  check(!f.runtime.active()&&f.commits==0&&settlements==unsigned(fail_first?2:1));
  check(!f.w.elemental_glyph_snapshot(f.owner,settled));
  check(f.w.skill_disabled_slot(f.owner,{219,0},settled)==23200); // Original control time +20s.
  check(f.w.skill_disabled_slot(f.owner,{219,1},settled)==0);
  const auto state=*f.runtime.cooldown_source_snapshot(f.owner,settled,f.h);
  check(std::none_of(state.timers.begin(),state.timers.end(),[](const auto& timer){return timer.slot==Slot{219,0};}));
  check(f.w.publish_local_cooldowns(f.owner,state));
  f.update(settled+100);check(settlements==unsigned(fail_first?2:1)&&f.commits==0);
  check(f.runtime.start(settled+100,{219,0,2},f.bar,f.policy,f.ranks,true,f.h)==casting::Reason::recharge);
  f.update(23200);check(f.w.skill_disabled_slot(f.owner,{219,0},23200)==0);
  check(f.runtime.start(23200,{219,0,2},f.bar,f.policy,f.ranks,true,f.h)==casting::Reason::accepted);
  f.runtime.cancel(f.h,23200);
 }
 for(bool protected_cast:{false,true}){Fixture f(203);f.glyph(203);f.now=3000;f.start(219);
  const auto attacker=*f.w.effect_scope(2);combat::weapon::Owned axe{12,1,{20,20,2},12,false,false,2};
  check(f.w.ordinary_weapon(attacker,axe,20));check(f.w.physical_armor(f.owner,60));
  if(protected_cast)check(f.w.status_mutation(f.owner,16,123,12,3000,3000+timing::stance(16,12)->duration,true));
  check(f.w.melee_attack(attacker,f.owner,axe,*combat::weapon::attack_skill(340,12,0),3200));
  f.update(4500);check(!f.runtime.active()&&f.commits==unsigned(protected_cast));
  check(!f.w.elemental_glyph_snapshot(f.owner,4500));
  check(f.w.skill_disabled_slot(f.owner,{219,0},4500)==unsigned(protected_cast?0:23200));
 }
 std::cout<<"PASS glyph198/199/203 Runtime payment,charges,Renewal success/cancel/interruption,Diversion/PowerBlock,real Chop contact forwarding/protection,atomic disable,delayed clock,failure retry and native order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
