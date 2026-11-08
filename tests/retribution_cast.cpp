#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;using namespace skills;using namespace skills::casting;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Retribution cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<14;++mode){
  Policy policy;policy.primary=mode==12?1:3;policy.secondary=mode==12?3:0;policy.campaigns=255;policy.learned[248]=1;
  Slots bar{};bar[0]={248,0};attributes::Ranks ranks{};ranks[14]=12;ranks[16]=10;
  vitals::Resources resources{100,100,40,40};effects::Scope target{1,43,1};
  bool valid=true;unsigned paid=0,calls=0,published=0;std::uint64_t admitted=0;std::vector<Packet> events;
  Hooks h;h.resources=[&](unsigned)->std::optional<vitals::Resources>{return resources;};
  h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
  h.consume_energy=[&](unsigned amount){check(amount==10);++paid;resources.energy-=amount;return true;};
  h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
  h.retribution_target_valid=[&](auto,unsigned divine,auto){check(divine==(mode==12?0u:10u));return valid;};
  h.before_spell_effect=[&](unsigned skill,std::uint64_t event,auto at){check(skill==248&&event&&at==2100);admitted=event;return mode!=4;};
  //248 has no caster body (sentinel2077); World returns target resource454.
  check(!skill_success_body_effect(42,248));
  const auto target_body=particles::targeted_body_2022(43,42,248);check(bool(target_body));
  const auto finish=terminal(58,42),body=*target_body;
  if(mode)h.retribution_completed=[&](auto scope,const effects::RetributionPlan& plan,unsigned divine,std::uint64_t event,auto at){
   ++calls;check(scope==target&&plan.rank==12&&divine==(mode==12?0u:10u)&&event==admitted&&at==2100);
   if(mode==5)return EffectCompletion{};
   if(mode==6||mode==12)h.events(std::vector<Packet>{finish,body});
   return EffectCompletion{true,(mode==6||mode==12)?std::vector<Packet>{}:std::vector<Packet>{body}, {},mode==12?std::function<void()>{}:std::function<void()>{[&]{++published;}},mode!=6&&mode!=12,mode==6||mode==12};
  };
  Runtime cast(42);check(!Runtime::effect_supported(248));
  check(cast.start(100,{248,0,43},bar,policy,ranks,true,h)==(mode?casting::Reason::accepted:casting::Reason::unsupported));
  check(paid==unsigned(mode!=0));if(!mode)continue;check(resources.energy==30);
  if(mode==2)cast.cancel(h,1000);
  if(mode==3)++target.generation;
  if(mode==7)valid=false;
  if(mode==8)h.retribution_completed={};
  if(mode==9)h.retribution_target_valid={};
  if(mode==10)h.before_spell_effect={};
  if(mode==11)h.target_scope={};
  if(mode==13)h.targeted_spell_failure=[](auto,auto,auto,auto){return SpellFailure::failed;};
  cast.update(2099,h);check(calls==0);cast.update(2100,h);
  check(calls==unsigned(mode==1||mode==5||mode==6||mode==12));
  check(published==unsigned(mode==1||mode==6));
  cast.update(2101,h);check(calls<=1);
  if(mode==1||mode==6||mode==12){check(std::count(events.begin(),events.end(),finish)==1);check(std::count(events.begin(),events.end(),body)==1);
   check(cast.start(2849,{248,0,43},bar,policy,ranks,true,h)==casting::Reason::busy);
   check(cast.start(2850,{248,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted);}
 }
 std::cout<<"PASS staged paid Retribution timing/ranks/cancel/lifetime/hook-loss/failure/prefix\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
