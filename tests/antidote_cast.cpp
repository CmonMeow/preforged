#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Antidote cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;using combat::conditions::Kind;
 for(unsigned mode=0;mode<6;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Ranger",0,0,box,0,{}, {},100,1);w.join(2,"Other",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,20,20},100);const auto owner=*w.effect_scope(1);
  for(auto kind:{Kind::poison,Kind::disease,Kind::blind,Kind::burning,Kind::weakness})check(w.apply_condition(owner,owner,kind,100,10000));
  casting::Hooks h;unsigned calls=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy=[](unsigned cost){check(cost==0);return true;};h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
  h.antidote_signet=[&](auto scope,auto at){++calls;return w.antidote_signet(scope,at);};if(mode==4)h.antidote_signet={};if(mode==5)h.target_scope={};
  Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[427]=1;Slots bar{};bar[0]={427,0};attributes::Ranks ranks{};casting::Runtime cast(1);
  check(cast.start(100,{427,0,2},bar,policy,ranks,true,h)==(mode>=4?casting::Reason::unsupported:casting::Reason::accepted));if(mode>=4)continue;
  cast.update(1099,h);check(!calls);if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);
  if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1099,1);w.set_vitals(1,{500,500,20,20},100);}
  box->drain();cast.update(1100,h);check(calls==(mode==0?1u:0u));
  if(mode==0){for(auto kind:{Kind::poison,Kind::disease,Kind::blind,Kind::weakness})check(!w.has_condition(owner,kind,1100));check(w.has_condition(owner,Kind::burning,1100));
   const auto events=box->drain();check(std::count(events.begin(),events.end(),casting::generic(21,1,492))==1);check(std::count(sent.begin(),sent.end(),casting::generic(21,1,730))==1);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{427,0},4))==1&&w.resources(1)->energy==20);
   check(cast.start(5099,{427,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }
 }
 std::cout<<"PASS Antidote1s/zeroEnergy/4s self signet, named+extra cleanse, both captured bodies, cancel/interrupt/stale and required-hook guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
