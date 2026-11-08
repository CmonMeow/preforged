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
   h.cast_slot_activity=[&](auto slot,auto begin,auto end){const bool changed=w.cast_activity(owner,slot,begin,end);if(w.effect_scope(owner.actor)==owner)check(changed);else check(!changed);};
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
 for(unsigned glyph:{198u,199u,203u})for(bool retire:{false,true}){
  Fixture f(glyph);f.glyph(glyph);f.now=3000;f.start(219);check(f.runtime.active());
  check(!f.w.reserve_elemental_glyph_use(f.owner,{219,0},5,0,999,3000));
  const auto scope=f.h.target_scope;const auto before=*f.w.resources(1);f.box->drain();f.h.target_scope={};
  f.update(4500);check(f.runtime.active()&&f.commits==0&&f.box->drain().empty());
  check(f.w.elemental_glyph_snapshot(f.owner,4500).has_value());
  check(!f.w.reserve_elemental_glyph_use(f.owner,{219,0},5,0,999,4500));
  const auto after=*f.w.resources(1);check(after.health==before.health&&after.energy==before.energy);
  // Even explicit cancellation cannot infer retirement from a missing query.
  f.runtime.cancel(f.h,4500);check(f.runtime.active()&&f.box->drain().empty());
  if(!retire){f.h.target_scope=scope;f.update(4500);check(!f.runtime.active()&&f.commits==1);}
  else{
   f.w.leave(1);f.w.join(1,"NewCaster",0,0,f.box,0,{}, {},4500,1);f.w.set_vitals(1,{1000,1000,100,100},100);
   const auto replacement=*f.w.effect_scope(1);check(replacement!=f.owner);f.box->drain();const auto fresh=*f.w.resources(1);
   f.h.target_scope=scope;f.update(4600);check(!f.runtime.active()&&f.commits==0&&f.box->drain().empty());
   const auto kept=*f.w.resources(1);check(kept.health==fresh.health&&kept.energy==fresh.energy);
   check(!f.w.elemental_glyph_snapshot(f.owner,4600));
   check(f.w.apply_elemental_glyph(replacement,1,*effects::elemental_glyph(glyph,0),4600));
   check(f.w.reserve_elemental_glyph_use(replacement,{219,0},5,0,999,4600).has_value());
  }
 }
 std::cout<<"PASS actual World glyph198/199/203 lease missing-authority retry versus proven retired scope\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
