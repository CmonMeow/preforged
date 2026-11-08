// Runtime contract coverage; synthetic callback packets isolate publication ordering.
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;using namespace skills;using namespace skills::casting;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Completion family line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{290u,308u,297u,294u,259u,817u,910u,1130u,296u})
 for(bool owns_prefix:{false,true})for(bool deferred:{false,true}){
  const auto* row=find_skill(skill);check(bool(row));
  Policy policy;policy.primary=row->profession;policy.campaigns=255;policy.learned[skill]=1;
  Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};
  const unsigned selected=skill==297?42:43;const effects::Scope owner{1,42,1},target{1,43,1};
  vitals::Resources resources{100,100,100,100};std::vector<Packet> packets;unsigned completed=0,published=0;
  const auto finish=terminal(58,42),visual=generic(21,selected,65000),status=generic(21,selected,65001);
  const auto caster_body=skill_success_body_effect(42,skill);
  Hooks h;h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](auto)->std::optional<vitals::Resources>{return resources;};
  h.consume_energy=[&](unsigned cost){check(cost==row->energy_cost());resources.energy-=cost;return true;};
  h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==42?std::optional(owner):actor==43?std::optional(target):std::nullopt;};
  h.target_valid=[](auto,float,bool){return true;};h.projectile_target_valid=[](auto,float){return true;};
  h.ally_enchantment_target_valid=[](auto,unsigned,auto){return true;};
  h.before_spell_effect=[](auto,auto,auto){return true;};
  h.blessed_signet_ready=[](auto,auto){return true;};h.signet_judgment_ready=[](auto){return true;};
  h.mending_target_valid=[](auto,unsigned,auto){return true;};h.succor_target_valid=[](auto,unsigned,auto){return true;};
  const auto complete=[&](){
   ++completed;EffectCompletion result{true};
   if(owns_prefix){std::vector<Packet> prefix{finish};if(caster_body)prefix.push_back(*caster_body);prefix.push_back(visual);h.events(prefix);}
   else result.visuals={visual};
   result.completion_prefix_published=owns_prefix;result.visuals_before_publication=true;
   if(deferred)result.publication=[&](){++published;h.events(std::vector<Packet>{status});};
   return result;
  };
  h.mending=[&](auto,const auto&,unsigned,auto){return complete();};
  h.succor=[&](auto,const auto&,unsigned,auto){return complete();};
  h.blessed_signet=[&](auto,const auto&,auto){return complete();};
  h.signet_judgment=[&](auto,const auto&,auto){return complete();};
  h.shield_deflection=[&](auto,const auto&,unsigned,auto){return complete();};
  h.discord=[&](auto,const auto&,auto){return complete();};
  h.spirit_rift=[&](auto,const auto&,auto){return complete();};
  h.spear_light=[&](auto,const auto&,auto){return complete();};
  h.holy_damage=[&](auto,const auto&,auto){return complete();};
  const auto finished_at=Milliseconds(100)+Milliseconds(std::llround(row->activation*1000.0));
  Runtime runtime(42);const auto admitted=runtime.start(100,{skill,0,selected},bar,policy,ranks,true,h);
  if(admitted!=casting::Reason::accepted)throw std::runtime_error("Family skill "+std::to_string(skill)+" admission "+reason_text(admitted));
  runtime.update(finished_at-1,h);check(!completed);runtime.update(finished_at,h);
  check(completed==1&&published==unsigned(deferred));runtime.update(finished_at+1,h);check(completed==1);
  check(std::count(packets.begin(),packets.end(),finish)==1);
  check(std::count(packets.begin(),packets.end(),visual)==1);
  if(caster_body)check(std::count(packets.begin(),packets.end(),*caster_body)==1);
  const auto finish_at=std::find(packets.begin(),packets.end(),finish),visual_at=std::find(packets.begin(),packets.end(),visual);
  check(finish_at<visual_at);
  if(deferred){check(std::count(packets.begin(),packets.end(),status)==1);check(visual_at<std::find(packets.begin(),packets.end(),status));}
 }
 std::cout<<"PASS maintained/signet/ally/holy-family publication and prefix contracts\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
