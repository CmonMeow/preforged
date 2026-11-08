#include "gw_maintained_request.h"
#include "gw_world_simulation.h"
#include "gw_outgoing_frame.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Maintained request line "+std::to_string(at.line()));}
combat::Packet request(unsigned effect,unsigned flags=0){combat::Packet packet{39,static_cast<std::uint8_t>(flags)};combat::detail::put(packet,effect);return packet;}
int main()try{
 for(unsigned flags:{0u,128u})for(unsigned mode=0;mode<9;++mode){world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{500,1000,100,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);check(w.stage_mending(caster,target,*mending(12),100));const auto effect=w.status_snapshot(target,100)->front().instance;auto packet=request(effect,flags);std::optional<Scope> actor=caster;bool ready=true;
  if(mode==1)ready=false;if(mode==2)actor.reset();if(mode==3)++actor->generation;if(mode==4)actor=target;if(mode==5)actor=*w.effect_scope(3);if(mode==6)packet=request(effect+1,flags);if(mode==7)packet=request(0,flags);if(mode==8)packet.push_back(0);
  const auto result=dispatch_drop_maintained(w,actor,ready,packet,200);const auto expected=mode==0?DropMaintainedResult::removed:mode<3?DropMaintainedResult::session_not_ready:mode<7?DropMaintainedResult::not_owned:DropMaintainedResult::invalid_frame;check(result==expected);check(w.maintained_enchantment_count(caster,200)==unsigned(mode!=0));
  if(mode==0){check(dispatch_drop_maintained(w,caster,true,packet,200)==DropMaintainedResult::not_owned);check(w.stage_mending(caster,target,*mending(12),200));const auto newer=w.status_snapshot(target,200)->front().instance;check(newer>effect&&dispatch_drop_maintained(w,caster,true,packet,200)==DropMaintainedResult::not_owned&&w.maintained_enchantment_count(caster,200)==1);}
 }
 // Native descriptor framing must wait for all six bytes and preserve a
 // coalesced second request. Each complete request is consumed once.
 for(unsigned flags:{0u,128u}){world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);const auto caster=*w.effect_scope(1);check(w.stage_mending(caster,caster,*mending(12),100));const auto effect=w.status_snapshot(caster,100)->front().instance;const auto packet=request(effect,flags);
  for(unsigned n=0;n<packet.size();++n)check(world::outgoing_frame_2022(std::span(packet).first(n)).state==world::FrameState::incomplete);
  auto combined=packet;combined.insert(combined.end(),packet.begin(),packet.end());const auto frame=world::outgoing_frame_2022(combined);check(frame.state==world::FrameState::complete&&frame.size==6);check(dispatch_drop_maintained(w,caster,true,std::span(combined).first(frame.size),200)==DropMaintainedResult::removed);combined.erase(combined.begin(),combined.begin()+frame.size);const auto next=world::outgoing_frame_2022(combined);check(next.state==world::FrameState::complete&&next.size==6&&dispatch_drop_maintained(w,caster,true,combined,200)==DropMaintainedResult::not_owned);}
 std::cout<<"PASS18 native-header session/ownership/malformed-request cases plus fragmentation/coalescing,replay and ID-reuse guards against actual World state\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
