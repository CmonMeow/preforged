#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Antidote reaction line "+std::to_string(at.line()));}
int main()try{using namespace skills;using combat::conditions::Kind;
 for(bool runtime:{false,true})for(unsigned health:{16u,18u,35u,100u})for(bool retired_source:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Ranger",80,0,box,0,{}, {},100,2);
  w.set_vitals(1,{500,500,20,20},100);w.set_vitals(2,{health,500,20,20},100);const auto source=*w.effect_scope(1),owner=*w.effect_scope(2);
  check(w.apply_condition(source,owner,Kind::blind,100,10000)&&w.apply_condition(source,owner,Kind::weakness,100,10000));
  check(w.degeneration_hex(source,owner,*effects::degeneration_hex(19,12),100));
  if(retired_source){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},101,1);w.set_vitals(1,{500,500,20,20},100);}
  box->drain();const auto epoch=*w.control_epoch(owner);std::vector<world::Packet> sent;unsigned calls=0;
  if(runtime){casting::Hooks h;h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](unsigned id){return w.resources(id);};
   h.consume_energy=[](unsigned cost){check(cost==0);return true;};h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float,bool self){return id==2&&self;};
   h.control_epoch=[&]{return w.control_epoch(owner);};h.control_event_time=[&](auto previous){return w.control_event_time(owner,previous);};
   h.antidote_signet=[&](auto scope,auto at){++calls;return w.antidote_signet(scope,at);};
   Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[427]=1;Slots bar{};bar[0]={427,0};attributes::Ranks ranks{};casting::Runtime cast(2);
   check(cast.start(200,{427,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(1200,h);check(calls==1);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,2,{427,0},4))==1);
  }else check(w.antidote_signet(owner,200));
  const auto expected=health>34?health-34:0;
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),casting::generic(10,1,19))==(retired_source?0:health<=17?1:2));
  check(w.resources(2)->health==expected);check(*w.control_epoch(owner)-epoch==unsigned(expected==0));
  check(!w.has_condition(owner,Kind::blind,1200)&&!w.has_condition(owner,Kind::weakness,1200));
  w.tick(2000);check(w.resources(2)->health==expected); // No resurrection or duplicate removal reaction.
 }
 std::cout<<"PASS Antidote direct/cast Fragility lethal first/second removal, survival, single death/recharge and retired/reused caster attribution\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
