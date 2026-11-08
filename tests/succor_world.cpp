#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Succor World line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<3;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==2?500.f:0.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{500,1000,10,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),other=*w.effect_scope(3);
  check(!w.succor_target_valid(source,source,0,100));
  check(!w.complete_succor(source,other,0,100));
  std::vector<world::Packet> visuals;
  check(w.complete_succor(source,target,0,100,&visuals));check(visuals.size()==1);
  check(w.maintained_enchantment_count(source,100)==1);
  const auto status=w.status_snapshot(target,100);check(status&&status->size()==1&&status->front().skill==308&&!status->front().expires);
  const auto first=status->front().instance;
  const auto end=*mending_end_wire(source,target,status->front());box->drain();
  check(w.settle_resources(target,1100));check(w.resources(2)->health==502);
  if(mode==0){check(w.cancel_maintained_enchantment(source,first,1100));}
  if(mode==1){check(w.complete_succor(source,target,0,1100));const auto replacement=w.status_snapshot(target,1100)->front().instance;check(replacement>first);check(w.cancel_maintained_enchantment(source,replacement,1100));}
  if(mode==2){w.move(2,9000,0,1100);check(w.settle_resources(source,41100));}
  check(w.maintained_enchantment_count(source,41100)==0);
  check(!w.status_snapshot(target,41100)->size());
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),end.upkeep)==1);
  check(std::count(packets.begin(),packets.end(),end.effect)==1);
 }
 // Mending and Succor share both the recipient registry and upkeep IDs.
 // A fresh per-skill Runtime counter must never replace the existing effect.
 {
  world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==2?500.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,10,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.stage_mending(source,target,*mending(0),100));
  const auto mending_id=w.status_snapshot(target,100)->front().instance;
  check(w.complete_succor(source,target,0,100));
  const auto statuses=*w.status_snapshot(target,100);check(statuses.size()==2);
  const auto added=std::find_if(statuses.begin(),statuses.end(),[](const auto& entry){return entry.skill==308;});
  check(added!=statuses.end()&&added->instance>mending_id);
  check(w.maintained_enchantment_count(source,100)==2);
  check(w.cancel_maintained_enchantment(source,added->instance,100));
  check(w.status_snapshot(target,100)->size()==1&&w.status_snapshot(target,100)->front().skill==290);
  check(w.cancel_maintained_enchantment(source,mending_id,100));
 }
 // Replacing at the same frontier contributes one Energy pip, exactly as
 // a single application. Cancelling must restore the baseline rate.
 std::vector<unsigned> energy_after,energy_cancelled;
 for(bool replace:{false,true}){
  world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==2?500.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,10,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.complete_succor(source,target,0,100));
  if(replace)check(w.complete_succor(source,target,0,100));
  check(w.settle_resources(target,10100));energy_after.push_back(w.resources(2)->energy);
  const auto effect=w.status_snapshot(target,10100)->front().instance;
  check(w.cancel_maintained_enchantment(source,effect,10100));
  check(w.settle_resources(target,20100));energy_cancelled.push_back(w.resources(2)->energy);
 }
 check(energy_after[0]==energy_after[1]&&energy_cancelled[0]==energy_cancelled[1]);
 // A recipient spell completion charges the maintainer once, while a
 // duplicate/reordered event cannot draw Energy again.
 {
  world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==2?500.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,10,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.complete_succor(source,target,0,100));
  check(w.settle_resources(source,1100));
  const auto before=w.resources(1)->energy;
  check(w.succor_spell_completed(target,281,1,1100));
  check(w.resources(1)->energy+1==before);
  check(w.succor_spell_completed(target,281,1,1100));
  check(w.resources(1)->energy+1==before);
  check(w.succor_spell_completed(target,281,2,1100));
  check(w.resources(1)->energy+2==before);
 }
 std::cout<<"Succor World passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
