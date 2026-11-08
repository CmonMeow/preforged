#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace casting;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Targeted failure line "+std::to_string(at.line()));
}
int main()try{
 for(auto outcome:{SpellFailure::none,SpellFailure::failed,SpellFailure::rejected}){
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Ally",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  const auto source=*w.effect_scope(1),selected=*w.effect_scope(2);unsigned paid=0,healed=0,reactions=0,failures=0;std::uint64_t reserved=0;
  Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto at){++paid;check(cost==5);return w.consume_energy(1,cost,source,at);};
  h.target_valid=[](auto id,auto,auto){return id==2;};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.heal_scoped=[&](auto target,auto,auto,auto,auto){check(target==selected);++healed;return true;};
  h.before_spell_effect=[&](auto skill,auto event,auto){check(skill==281&&event);++reactions;reserved=event;return true;};
  h.targeted_spell_failure=[&](auto skill,auto target,auto event,auto){check(skill==281&&target==selected&&event==reserved&&reactions==1);++failures;return outcome;};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[281]=1;Slots bar{};bar[0]={281,0};attributes::Ranks ranks{};Runtime cast(1);
  const auto finish=1000+std::uint64_t(std::llround(find_skill(281)->activation*1000));
  check(cast.start(1000,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted&&paid==1&&w.resources(1)->energy==95);
  cast.update(finish,h);check(!cast.active()&&failures==1&&healed==unsigned(outcome==SpellFailure::none)&&paid==1);
  cast.update(finish,h);check(failures==1&&healed==unsigned(outcome==SpellFailure::none));
  h.targeted_spell_failure={};
  const auto retry=cast.start(finish+750,{281,0,2},bar,policy,ranks,true,h);
  check(retry==(outcome==SpellFailure::none?casting::Reason::recharge:casting::Reason::accepted));
  if(cast.active())cast.cancel(h);
  const auto payments=paid;const auto until=finish+5000;
  h.skill_disabled_slot=[&](Slot slot,auto)->std::optional<Milliseconds>{check(slot.skill==281);return until;};
  check(cast.start(until-1,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge&&paid==payments);
  check(cast.start(until,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted&&paid==payments+1);

 }
 {
  world::Instance w;
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==3?50u:500u,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto victim=*w.effect_scope(1),ally=*w.effect_scope(2),source=*w.effect_scope(3);
  check(w.stage_mark_subversion(source,victim,*effects::mark_subversion(12),1000));
  Hooks h;unsigned paid=0,healed=0;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto at){++paid;return w.consume_energy(1,cost,victim,at);};
  h.target_valid=[](auto id,auto,auto){return id==2;};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.heal_scoped=[&](auto target,auto amount,auto range,auto self,auto at){++healed;return w.heal_target(1,target.actor,amount,range,self,at);};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(victim,skill,event,at);};
  h.targeted_spell_failure=[&](auto skill,auto target,auto event,auto at){check(target==ally);return w.mark_subversion_spell_failure(victim,skill,target,event,at);};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[281]=1;Slots bar{};bar[0]={281,0};attributes::Ranks ranks{};Runtime cast(1);
  const auto duration=std::uint64_t(std::llround(find_skill(281)->activation*1000)),finish=1000+duration;
  check(cast.start(1000,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(finish,h);
  check(healed==0&&paid==1&&w.resources(1)->health==424&&w.resources(2)->health==500&&w.resources(3)->health==126);
  check(w.status_snapshot(victim,finish)->empty());cast.update(finish,h);check(w.resources(3)->health==126);
  check(cast.start(finish+750,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(finish+750+duration,h);
  check(healed==1&&paid==2&&w.resources(2)->health>500&&w.resources(3)->health==126);
 }
 std::cout<<"PASS targeted failure receives pending scoped target and reserved activation;failed spell spends cost,skips heal and normal recharge;World policy/retail wire pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
