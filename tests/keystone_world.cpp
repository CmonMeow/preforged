#include "gw_world_simulation.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Keystone World line "+std::to_string(at.line()));
}
int main()try{
 world::Instance w;auto box=std::make_shared<world::Outbox>();
 w.join(1,"Owner",0,0,box,0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 for(auto id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
 const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);const auto before=*w.disable_reset_revision(owner);
 check(w.cast_activity(owner,66,100,3100)&&w.interrupt_skill(foe,owner,*interrupt_skill(5,12),20,200));
 box->drain();
 check(!w.keystone_signet(owner,*keystone_signet(12),{{62,1}},before,1,300));
 check(!w.keystone_snapshot(owner,300)&&w.status_snapshot(owner,300)->empty());
 const auto permit=*w.disable_reset_revision(owner);
 check(!w.keystone_signet(owner,*keystone_signet(12),{{66,0}},permit,1,300));
 check(w.keystone_signet(owner,*keystone_signet(12),{{62,1}},permit,1,300));
 auto effect=w.keystone_snapshot(owner,300);check(effect&&effect->remaining==5&&effect->expires==20300);
 check(w.skill_disabled_slot(owner,{62,1},300)==0&&w.skill_disabled_slot(owner,{62,0},300)==10200);
 const auto statuses=*w.status_snapshot(owner,300);check(statuses.size()==1&&statuses[0].instance==effect->effect&&statuses[0].kind==Category::other);
 world::Packet body;world::u16(body,158);world::u32(body,21);world::u32(body,1);world::u32(body,112);
 const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),body)==1);
 const auto renewed=*w.disable_reset_revision(owner);
 check(w.keystone_signet(owner,*keystone_signet(20),{},renewed,2,400));
 const auto replacement=w.keystone_snapshot(owner,400);check(replacement&&replacement->effect!=effect->effect&&replacement->remaining==8&&replacement->expires==20400);
 const auto next=*w.disable_reset_revision(owner);
 check(!w.keystone_signet(owner,*keystone_signet(20),{},next,2,500));
 check(w.keystone_snapshot(owner,500)->effect==replacement->effect);
 check(w.settle_resources(owner,20400)&&!w.keystone_snapshot(owner,20400)&&w.status_snapshot(owner,20400)->empty());
 w.leave(1);check(!w.keystone_snapshot(owner,20400));
 w.join(1,"Replacement",0,0,box,0,{}, {},20500,1);w.set_vitals(1,{500,500,100,100},100);
 check(!w.keystone_snapshot(*w.effect_scope(1),20500));
 std::cout<<"PASS Keystone World timer/charge/reset commit, stale permit rejection, native body112, renewal/expiry and departure; reactive use remains unbound\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
