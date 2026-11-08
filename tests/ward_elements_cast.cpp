#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Ward175 cast line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<7;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto source=*w.effect_scope(1);
  casting::Hooks h;unsigned calls=0;std::vector<world::Packet> sent;
  h.events=[&](const auto&packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){check(cost==15);return w.consume_energy(1,cost,source,now);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto id,float,bool allow_self){return id==1&&allow_self;};
  h.ward_elements=[&](const auto&plan,auto now){++calls;check(plan==*effects::ward_elements_plan(12)&&now==1100);
   std::vector<world::Packet>visuals;const bool accepted=w.apply_ward_elements(source,plan,now,&visuals);return casting::EffectCompletion{accepted,std::move(visuals)};};
  if(mode==4)h.ward_elements={};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[175]=1;
  Slots slots{};slots[0]={175,0};attributes::Ranks ranks{};ranks[9]=12;casting::Runtime runtime(1);check(casting::Runtime::effect_supported(175));
  check(runtime.start(100,{175,0,0},slots,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==4){check(w.resources(1)->energy==100&&!calls);continue;}check(w.resources(1)->energy==85);runtime.update(1099,h);check(!calls);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1099,h);if(mode==3)h.ward_elements={};
  if(mode==5)check(w.kill(source,1099));
  if(mode==6){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1100,1);w.set_vitals(1,{1000,1000,100,100},100);}
  box->drain();runtime.update(1100,h);
  if(mode){check(w.ward_elemental_armor(*w.effect_scope(1),1100)==0);continue;}
  check(calls==1&&w.ward_elemental_armor(source,1100)==24);
  const auto finish=casting::terminal(58,1),caster=casting::generic(21,1,314),ground=particles::positional_effect_2022(1,900,0,0,0);
  for(const auto&packet:{finish,caster,ground})check(std::count(sent.begin(),sent.end(),packet)==1);
  check(std::find(sent.begin(),sent.end(),finish)<std::find(sent.begin(),sent.end(),caster)&&std::find(sent.begin(),sent.end(),caster)<std::find(sent.begin(),sent.end(),ground));
  check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{175,0},20))==1);
  check(runtime.start(1850,{175,0,0},slots,policy,ranks,true,h)==casting::Reason::recharge);
  check(box->drain().empty()); // Completion packets returned to Runtime exactly once.
  w.tick(19100);check(w.ward_elemental_armor(source,19100)==0&&!box->drain().empty());
 }
 std::cout<<"Ward175 Runtime typed completion, rank/resources, native314/900/effect chronology, cancellation and lifetimes passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
