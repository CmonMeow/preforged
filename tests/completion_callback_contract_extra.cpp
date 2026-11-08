// Detached regression target for the common EffectCompletion contract.
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;using namespace skills;using namespace skills::casting;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Completion contract extra line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{925u,876u})for(bool owns_prefix:{false,true})for(bool deferred:{false,true}){
  Policy policy;policy.primary=7;policy.campaigns=255;policy.learned[skill]=1;
  Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};effects::Scope target{1,43,1};
  vitals::Resources resources{100,100,100,100};std::vector<Packet> packets;unsigned completed=0,published=0;
  const auto finish=terminal(58,42);const auto caster_body=skill_success_body_effect(42,skill);
  const auto visual=skill==925?particles::targeted_body_2022(43,42,925):std::optional<Packet>{effects::signet_shadows_impact_2022(42,43)};
  check(bool(visual));const auto status=generic(21,43,123);
  Hooks h;h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](auto)->std::optional<vitals::Resources>{return resources;};
  h.consume_energy=[&](unsigned cost){check(cost==(skill==925?15u:0u));resources.energy-=cost;return true;};
  h.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
  h.projectile_target_valid=[](auto,float){return true;};
  h.recall_target_valid=[](auto,auto){return true;};h.before_spell_effect=[](auto,auto,auto){return true;};
  const auto complete=[&](){
   ++completed;EffectCompletion result{true};
   if(owns_prefix){std::vector<Packet> prefix{finish};if(caster_body)prefix.push_back(*caster_body);prefix.push_back(*visual);h.events(prefix);}
   else result.visuals={*visual};
   result.completion_prefix_published=owns_prefix;result.visuals_before_publication=true;
   if(deferred)result.publication=[&](){++published;h.events(std::vector<Packet>{status});};
   return result;
  };
  if(skill==925)h.recall=[&](auto,auto){return complete();};
  else h.signet_shadows=[&](auto,const auto&,auto){return complete();};
  Runtime runtime(42);check(runtime.start(100,{skill,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted);
  runtime.update(1099,h);check(!completed);runtime.update(1100,h);
  check(completed==1&&published==unsigned(deferred));runtime.update(1101,h);check(completed==1);
  check(std::count(packets.begin(),packets.end(),finish)==1);
  check(std::count(packets.begin(),packets.end(),*visual)==1);
  if(caster_body)check(std::count(packets.begin(),packets.end(),*caster_body)==1);
  const auto finish_at=std::find(packets.begin(),packets.end(),finish),visual_at=std::find(packets.begin(),packets.end(),*visual);
  check(finish_at<visual_at);
  if(deferred){check(std::count(packets.begin(),packets.end(),status)==1);check(visual_at<std::find(packets.begin(),packets.end(),status));}
 }
 std::cout<<"PASS Recall/Shadows deferred publication and owned native prefix contracts\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
