#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Condition completion event line "+std::to_string(at.line()));
}
int main()try{
 std::uint64_t previous=0;
 for(unsigned mode=0;mode<9;++mode){
  skills::casting::Runtime runtime(1);skills::casting::Hooks h;
  vitals::Resources resources{1000,1000,100,100};
  const skills::effects::Scope target{7,2,3};std::uint64_t admitted=0,completed=0;unsigned calls=0;
  h.resources=[&](auto){return std::optional{resources};};
  h.events=[](const auto&){};
  h.consume_energy_at=[&](unsigned cost,auto){if(resources.energy<cost)return false;resources.energy-=cost;return true;};
  h.target_scope=[&](auto id)->std::optional<skills::effects::Scope>{if(id!=2)return {};return target;};
  h.projectile_target_valid=[](auto,auto){return true;};
  if(mode!=3)h.before_spell_effect=[&](auto skill,auto event,auto){check(skill==117&&event);admitted=event;return mode!=2;};
  if(mode!=4)h.condition_spell_completed=[&](auto scope,const auto& plan,auto event,auto)->skills::casting::EffectCompletion{
   check(scope==target&&plan.skill==117);check(event==admitted);if(mode!=3)check(event>previous);
   completed=event;++calls;return {mode!=7};
  };
  skills::Slots bar{};bar[0]={117,0};skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[117]=1;
  attributes::Ranks ranks{};ranks.fill(12);
  const auto reason=runtime.start(100,{117,0,2},bar,policy,ranks,true,h);
  if(mode==3||mode==4){check(reason==skills::casting::Reason::unsupported&&resources.energy==100);continue;}
  check(reason==skills::casting::Reason::accepted&&resources.energy<100);
  if(mode==1)runtime.cancel(h);
  if(mode==5)runtime.interrupt(101,h);
  if(mode==6)h.condition_spell_completed={};
  if(mode==8)h.before_spell_effect={};
  const auto finish=100+std::uint64_t(skills::find_skill(117)->activation*1000);
  runtime.update(finish,h);runtime.update(finish+10000,h);
  if(mode==1||mode==2||mode==5||mode==6||mode==8)check(calls==0);
  else check(calls==1&&completed==admitted&&(mode==3?completed==0:completed!=0));
  if(completed)previous=completed;
  if(mode==1||mode==5||mode==6||mode==8)check(admitted==0);
 }
 std::cout<<"9 condition completion event identity/cancel/interrupt/refusal/missing-or-lost-hook/no-replay cases pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
