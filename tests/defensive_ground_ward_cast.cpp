#include "gw_defensive_ward_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ward938 cast line "+std::to_string(at.line())+" "+context);
}
int main()try{unsigned cases=0;
 check(casting::Runtime::effect_supported(938)); // Normal producer now uses the same tested factory.
 for(unsigned rank=0;rank<=20;++rank)for(unsigned renewal:{0u,181u,2860u})for(bool pvp:{false,true})for(unsigned mode=0;mode<16;++mode){
  context="rank="+std::to_string(rank)+" renewal="+std::to_string(renewal)+" pvp="+std::to_string(pvp)+" mode="+std::to_string(mode);
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Paid ward",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,50,100},100);
  const auto source=*w.effect_scope(1);
  if(renewal){const auto r=*ether_renewal(renewal,0);check(w.status_mutation(source,renewal,80,0,100,100+r.duration,true));}
  casting::Hooks h;unsigned calls=0;h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& ps){box->enqueue(ps);};
  h.consume_energy_at=[&](auto cost,auto at){check(cost==10);return w.consume_energy(1,cost,source,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.before_spell_effect=[&](auto skill,auto event,auto at){check(skill==938&&event);w.tick(at);return w.before_spell_effect(source,skill,event,at);};
  auto native=casting::make_defensive_ward_completion(w,[&]{return w.effect_scope(1);});
  h.defensive_ward_completed=[&,native](const auto& plan,auto event,auto at)mutable{
   ++calls;check(plan.rank==rank);return native(plan,event,at);};
  if(mode==8)h.defensive_ward_completed={};if(mode==9)h.before_spell_effect={};if(mode==10)h.target_scope={};if(mode==11)h.target_valid={};
  Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[938]=1;policy.pvp_match=pvp;
  Slots bar{};bar[0]={938,0};attributes::Ranks ranks{};ranks[9]=rank;casting::Runtime runtime(1);
  const bool admitted=mode<8||mode>=12;
  const auto reason=runtime.start(100,{938,0,mode==15?999u:0u},bar,policy,ranks,true,h);
  check((reason==casting::Reason::accepted)==admitted);check(w.resources(1)->energy==(admitted?40:50));
  if(!admitted){check(!calls&&!w.ward_knockdown_protection(source,1100));++cases;continue;}
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(200,h);if(mode==3)h.defensive_ward_completed={};if(mode==4)h.before_spell_effect={};
  if(mode==5)h.target_scope={};if(mode==6)h.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(scope)++scope->generation;return scope;};
  if(mode==7)h.target_valid=[](auto,float,bool){return false;};if(mode==12)check(w.kill(source,300));
  if(mode==13)ranks[9]=0; // Pending cast retains its admitted effective Earth rank.
  if(mode==14){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},300,1);w.set_vitals(1,{500,1000,50,100},100);}
  runtime.update(1099,h);check(!calls);box->drain();runtime.update(1100,h);runtime.update(1100,h);
  const bool success=mode==0||mode==13||mode==15;
  check(calls==unsigned(success));const auto current=*w.effect_scope(1);
  check(w.ward_knockdown_protection(current,1100)==success);const auto packets=box->drain();
  if(success){
   check(w.resources(1)->health==500+(renewal?5:0)&&w.resources(1)->energy==40+(renewal?1:0));
   const auto finish=casting::terminal(58,1),body=casting::generic(21,1,1187),ground=particles::ward_stability_ground_2022(0,0,0),recharge=casting::slot_event(228,1,{938,0},30);
   for(const auto& p:{finish,body,ground,recharge})check(std::count(packets.begin(),packets.end(),p)==1);
   const auto f=std::find(packets.begin(),packets.end(),finish),b=std::find(packets.begin(),packets.end(),body),g=std::find(packets.begin(),packets.end(),ground),r=std::find(packets.begin(),packets.end(),recharge);
   check(r<f&&f<b&&b<g);
   check(runtime.start(1101,{938,0,0},bar,policy,ranks,true,h)==casting::Reason::busy);
   const auto expiry=1100+(10+rank)*1000;
   check(w.ward_knockdown_protection(source,expiry-1)&&!w.ward_knockdown_protection(source,expiry));
   check(runtime.start(1850,{938,0,0},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }else check(std::find(packets.begin(),packets.end(),particles::ward_stability_ground_2022(0,0,0))==packets.end());
  ++cases;
 }
 for(unsigned renewal:{181u,2860u})for(bool pvp:{false,true}){
  context="paid reward death renewal="+std::to_string(renewal)+" pvp="+std::to_string(pvp);
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Dying ward",0,0,box,0,{}, {},100,1);
  w.set_vitals(1,{50,1000,50,100},100);const auto source=*w.effect_scope(1);
  const auto r=*ether_renewal(renewal,0);check(w.status_mutation(source,renewal,80,0,100,100+r.duration,true));
  check(w.apply_condition(source,source,combat::conditions::Kind::deep_wound,100,60000));
  casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};h.events=[&](const auto& ps){box->enqueue(ps);};
  h.consume_energy_at=[&](auto cost,auto at){return w.consume_energy(1,cost,source,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
  auto native=casting::make_defensive_ward_completion(w,[&]{return w.effect_scope(1);});unsigned calls=0;
  h.defensive_ward_completed=[&,native](const auto& plan,auto event,auto at)mutable{++calls;return native(plan,event,at);};
  Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[938]=1;policy.pvp_match=pvp;
  Slots bar{};bar[0]={938,0};attributes::Ranks ranks{};casting::Runtime runtime(1);
  check(runtime.start(100,{938,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);box->drain();runtime.update(1100,h);
  check(calls==1&&w.resources(1)->health==0&&!w.ward_knockdown_protection(source,1100));
  const auto packets=box->drain();
  check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{938,0},30))==1);
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
  check(std::find(packets.begin(),packets.end(),casting::generic(21,1,1187))==packets.end());
  check(std::find(packets.begin(),packets.end(),particles::ward_stability_ground_2022(0,0,0))==packets.end());++cases;
 }
 std::cout<<cases<<" paid Ward938 all-rank PvE/PvP cost, captured recharge/finish/body/ground order, retained rank, cancellation/interruption, hook/target/lifetime guards passed; production factory is source-bound; rendered QA pending\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
