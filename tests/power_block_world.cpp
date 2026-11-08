#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Power Block World regression line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 for(bool protected_cast:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Target",60,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.cast_activity(target,Slot{28,7},100,3100));
  check(w.casting_slot(target,100)==std::optional<Slot>{Slot{28,7}});
  check(w.cast_activity(target,Slot{28,0},0,0)); // Stale/base-copy cleanup must not clear copy 7.
  check(w.casting_slot(target,200)==std::optional<Slot>{Slot{28,7}});
  check(!w.cast_activity(target,Slot{28,65536},100,3100));
  check(!w.casting_slot(target,99)&&!w.casting_slot(target,3100));

  if(protected_cast)check(w.status_mutation(target,16,3000,0,100,10000,true));
  const auto epoch=w.control_epoch(target);box->drain();
  check(w.interrupt_skill(caster,target,*effects::interrupt_skill(5,12),20,200));
  check((w.control_epoch(target)==epoch)==protected_cast);
  check(w.skill_disabled_until(target,23,200)==10200&&w.skill_disabled_until(target,28,200)==10200&&w.skill_disabled_until(target,14,200)==0);
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,5))==1);
  casting::Hooks hooks;hooks.events=[](const auto&){};
  hooks.resources=[&](unsigned id){return w.resources(id);};hooks.consume_energy=[&](unsigned amount){return w.consume_energy(2,amount,target);};
  hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};hooks.projectile_target_valid=[](unsigned id,float range){return id==1&&range==1248;};
  hooks.interrupt_skill=[&](auto selected,const auto& plan,auto now){return w.interrupt_skill(target,selected,plan,20,now);};
  hooks.skill_disabled_until=[&](unsigned skill,auto now){return w.skill_disabled_until(target,skill,now);};
  unsigned slot_starts=0,slot_stops=0,legacy_calls=0;
  hooks.cast_activity=[&](auto,auto,auto){++legacy_calls;};
  hooks.cast_slot_activity=[&](Slot slot,auto begin,auto end){check(slot==Slot{23,0});if(begin||end)++slot_starts;else ++slot_stops;};
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[23]=1;Slots bar{};bar[0]={23,0};attributes::Ranks ranks{};casting::Runtime runtime(2);
  check(runtime.start(300,{23,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge&&w.resources(2)->energy==100);
  check(runtime.start(10199,{23,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge&&w.resources(2)->energy==100);
  check(runtime.start(10200,{23,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&w.resources(2)->energy==100-find_skill(23)->energy_cost());
  check(slot_starts==1&&legacy_calls==0);runtime.cancel(hooks);check(slot_stops==1&&legacy_calls==0);
  check(!w.casting_slot(target,10200));
  w.leave(2);w.join(2,"Replacement",60,0,box,0,{}, {},10300,2);w.set_vitals(2,{500,500,100,100},100);
  check(!w.skill_disabled_until(target,23,10300)&&w.skill_disabled_until(*w.effect_scope(2),23,10300)==0);
 }
 std::cout<<"PASS Power Block actual World disable despite interrupt protection,targetbody11,Runtime rejection before Energy,expiry and actor replacement;cooldown UI/NPC/visual acceptance pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
