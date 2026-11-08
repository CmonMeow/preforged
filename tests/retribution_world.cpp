#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <fstream>
using namespace fw::gw;using namespace skills::effects;
unsigned tested_case=0;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Retribution World line "+std::to_string(at.line()));
}
void write_packets(std::ostream& out,const std::vector<world::Packet>& packets){
 out<<'[';bool first=true;for(const auto& packet:packets){if(!first)out<<',';first=false;out<<'"';for(auto c:packet)out<<"0123456789abcdef"[c>>4]<<"0123456789abcdef"[c&15];out<<'"';}out<<']';
}
int main(int argc,char** argv)try{
 std::ofstream fixture;if(argc>1){fixture.open(argv[1]);check(bool(fixture));}
 unsigned cases=0;
 for(bool self:{false,true})for(bool secondary:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<9;++mode){
  ++cases;tested_case=cases;world::Instance w;auto source_box=std::make_shared<world::Outbox>(),target_box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,source_box,0,{world::Packet{165,0,1,0,0,0,static_cast<std::uint8_t>(secondary?6:3),0}}, {},100,1);
  w.join(2,"Ally",mode==6?2000.f:100.f,0,target_box,0,{}, {},100,1);
  w.join(3,"Foe",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u,3u})w.set_vitals(id,{500,1000,100,100},100);
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(1);auto target=*w.effect_scope(mode==3?3:mode==6?2:self?1:2);
  check(!w.status_mutation(source,248,123,rank,100,{},true));
  auto plan=*retribution(rank);const unsigned divine=mode==5?12:secondary?0:12;
  if(mode==1)++plan.percent;if(mode==2)++target.generation;if(mode==4)target.actor=999;
  if(mode==7)w.set_vitals(target.actor,{0,1000,100,100},100);
  source_box->drain();target_box->drain();
  const bool denied=mode==1||mode==2||mode==3||mode==4||mode==6||mode==7||mode==8||(mode==5&&secondary);
  check(w.stage_retribution(source,target,plan,mode==8?99:100,divine,true)==!denied);
  if(denied){check(w.maintained_enchantment_count(source,100)==0);continue;}
  const auto status=*w.status_snapshot(target,100);check(status.size()==1&&status[0].skill==248&&!status[0].expires&&status[0].cause==source.actor);
  const auto wire=*mending_wire(source,target,status[0]),ended=*mending_end_wire(source,target,status[0]);
  const auto applied=source_box->drain();
  const auto body=self?*skills::particles::target_body_2022(target.actor,248):*skills::particles::targeted_body_2022(target.actor,source.actor,248);
  check(std::count(applied.begin(),applied.end(),combat::detail::integer(58,1,0))==1&&std::count(applied.begin(),applied.end(),body)==1);
  check(std::count(applied.begin(),applied.end(),wire.upkeep)==1&&std::count(applied.begin(),applied.end(),wire.effect)==1);
  check(std::find(applied.begin(),applied.end(),body)<std::find(applied.begin(),applied.end(),wire.effect));
  check(w.maintained_enchantment_count(source,100)==1&&w.resources(1)->energy==100); // Trusted stage is not paid casting.
  const auto health=500u+(divine?38:0);check(w.resources(target.actor)->health==health);
  check(w.settle_resources(target,1100)&&w.resources(target.actor)->health==health); // No phantom regeneration source.
  const auto snapshots=w.join(4,"Observer",300,0,std::make_shared<world::Outbox>(),0,{}, {},1100,1);
  const auto snapshot=std::find_if(snapshots.begin(),snapshots.end(),[&](const auto& p){return p.id==target.actor;});
  check(snapshot!=snapshots.end()&&std::count(snapshot->presentation.begin(),snapshot->presentation.end(),wire.effect)==1);
  auto stale=source;++stale.generation;check(!w.cancel_maintained_enchantment(stale,status[0].instance,1100));
  check(!w.cancel_maintained_enchantment(*w.effect_scope(3),status[0].instance,1100));source_box->drain();
  check(w.cancel_maintained_enchantment(source,status[0].instance,1100));
  check(w.maintained_enchantment_count(source,1100)==0&&!w.cancel_maintained_enchantment(source,status[0].instance,1100));
  const auto packets=source_box->drain();check(std::count(packets.begin(),packets.end(),ended.upkeep)==1&&std::count(packets.begin(),packets.end(),ended.effect)==1);
  if(mode==0&&fixture.is_open()){
   fixture<<"{\"self\":"<<self<<",\"secondary\":"<<secondary<<",\"rank\":"<<rank<<",\"target\":"<<target.actor<<",\"effect\":"<<status[0].instance<<",\"application\":";
   write_packets(fixture,applied);fixture<<",\"end\":";write_packets(fixture,packets);fixture<<"}\n";
  }
 }
 // Generic removal and both actor departures retire the same ownership.
 for(unsigned mode=0;mode<3;++mode){
  ++cases;tested_case=cases;world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Ally",float(id*10),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.stage_retribution(source,target,*retribution(12),100));const auto status=w.status_snapshot(target,100)->front();const auto ended=*mending_end_wire(source,target,status);box->drain();
  if(mode==0)check(w.status_mutation(target,248,status.instance,12,1100,{},false));
  else{check(w.settle_resources(target,1100));const unsigned departed=mode==1?1:2;w.leave(departed);w.join(departed,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},1100,1);w.set_vitals(departed,{500,1000,100,100},100);check(!w.cancel_maintained_enchantment(*w.effect_scope(departed),status.instance,1100));}
  if(mode!=1)check(w.maintained_enchantment_count(source,1100)==0);
  if(mode!=2)check(w.status_snapshot(target,1100)->empty());
  if(mode==0){const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),ended.upkeep)==1&&std::count(packets.begin(),packets.end(),ended.effect)==1);}
 }
 // Shared Energy zero must end exactly at3100, regardless of actor ordering
 // or recipient-first dense/sparse polling. Retribution adds no healing pips.
 for(bool reverse:{false,true})for(bool dense:{false,true}){
  ++cases;tested_case=cases;world::Instance w;const unsigned source_id=reverse?2:1,target_id=reverse?1:2;auto box=std::make_shared<world::Outbox>(),other=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Ally",float(id*10),0,id==source_id?box:other,0,{}, {},100,1);w.set_vitals(id,{500,1000,id==source_id?1u:100u,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100)&&w.energy_capacity(source_id,3,0,100,{80,-2}));
  const auto source=*w.effect_scope(source_id),target=*w.effect_scope(target_id);check(w.stage_retribution(source,target,*retribution(12),100));
  const auto ended=*mending_end_wire(source,target,w.status_snapshot(target,100)->front());box->drain();other->drain();
  std::vector<world::Packet> packets;const auto flush=[&]{auto next=box->drain();packets.insert(packets.end(),next.begin(),next.end());other->drain();};
  if(dense)for(unsigned at=117;at<3099;at+=17){check(w.settle_resources(target,at));flush();}
  check(w.settle_resources(target,3099)&&w.maintained_enchantment_count(source,3099)==1);
  check(w.settle_resources(target,3100)&&w.maintained_enchantment_count(source,3100)==0);
  check(w.settle_resources(target,6100)&&w.resources(source_id)->energy==0&&w.resources(target_id)->health==500);
  flush();check(std::count(packets.begin(),packets.end(),ended.upkeep)==1&&std::count(packets.begin(),packets.end(),ended.effect)==1);
 }
 for(bool target_dies:{false,true})for(bool reverse:{false,true}){
  ++cases;tested_case=cases;world::Instance w;const unsigned source_id=reverse?2:1,target_id=reverse?1:2;
  auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*10),0,id==source_id?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==(target_dies?target_id:source_id)?6u:500u,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(source_id),target=*w.effect_scope(target_id);check(w.stage_retribution(source,target,*retribution(12),100));
  const auto ended=*mending_end_wire(source,target,w.status_snapshot(target,100)->front());box->drain();
  check(w.apply_condition(*w.effect_scope(3),target_dies?target:source,combat::conditions::Kind::poison,100,10000));
  check(w.settle_resources(target,6100));check(w.resources(target_dies?target_id:source_id)->health==0&&w.maintained_enchantment_count(source,6100)==0);
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),ended.upkeep)==1&&std::count(packets.begin(),packets.end(),ended.effect)==1);
 }
 // Range retirement shares the current authoritative movement forecast.
 for(bool reverse:{false,true})for(bool dense:{false,true}){
  ++cases;tested_case=cases;world::Instance w;const unsigned source_id=reverse?2:1,target_id=reverse?1:2;
  auto box=std::make_shared<world::Outbox>(),other=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Ally",id==source_id?0.f:500.f,0,id==source_id?box:other,0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(source_id),target=*w.effect_scope(target_id);
  check(w.stage_retribution(source,target,*retribution(12),100));const auto ended=*mending_end_wire(source,target,w.status_snapshot(target,100)->front());w.move(target_id,9000,0,100);
  const auto outside=[&](unsigned at){const auto a=w.position(source_id,at),b=w.position(target_id,at);return std::hypot(double(a.first)-b.first,double(a.second)-b.second)>5000.;};
  check(!outside(100)&&outside(40100));unsigned inside=100,expected=40100;
  while(expected-inside>1){const auto middle=inside+(expected-inside)/2;if(outside(middle))expected=middle;else inside=middle;}
  box->drain();other->drain();std::vector<world::Packet> packets;
  const auto flush=[&]{auto next=box->drain();packets.insert(packets.end(),next.begin(),next.end());other->drain();};
  if(dense)for(unsigned at=237;at<expected;at+=137){check(w.settle_resources(source,at));flush();}
  check(w.settle_resources(source,expected-1)&&w.maintained_enchantment_count(source,expected-1)==1);
  check(w.settle_resources(source,expected)&&w.maintained_enchantment_count(source,expected)==0);flush();
  check(std::count(packets.begin(),packets.end(),ended.upkeep)==1&&std::count(packets.begin(),packets.end(),ended.effect)==1);
 }
 // A weaker cross-caster replacement retains the stronger indefinite rank,
 // while transferring upkeep and making the previous cancellation stale.
 {
  ++cases;tested_case=cases;world::Instance w;auto first_box=std::make_shared<world::Outbox>(),second_box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Ally",float(id*10),0,id==1?first_box:id==3?second_box:std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100)&&w.party_action(1,158,3,100)&&w.party_action(3,154,1,100));
  const auto first=*w.effect_scope(1),target=*w.effect_scope(2),second=*w.effect_scope(3);
  check(w.stage_retribution(first,target,*retribution(12),100));const auto old=w.status_snapshot(target,100)->front();
  first_box->drain();second_box->drain();check(w.stage_retribution(second,target,*retribution(11),1100));const auto current=w.status_snapshot(target,1100)->front();check(current.rank==12);
  check(current.instance>old.instance&&current.cause==second.actor&&w.maintained_enchantment_count(first,1100)==0&&w.maintained_enchantment_count(second,1100)==1);
  const auto packets=first_box->drain();const auto ended=*mending_end_wire(first,target,old),added=*mending_wire(second,target,current);
  check(std::count(packets.begin(),packets.end(),ended.upkeep)==1&&std::count(packets.begin(),packets.end(),ended.effect)==1);
  check(std::find(packets.begin(),packets.end(),ended.effect)<std::find(packets.begin(),packets.end(),added.effect));
  check(!w.cancel_maintained_enchantment(first,current.instance,1100)&&w.cancel_maintained_enchantment(second,current.instance,1100));
 }
 // Adding/removing the no-regeneration status preserves a concurrent Mending.
 {
  ++cases;tested_case=cases;world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Ally",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.stage_mending(source,target,*mending(12),100)&&w.stage_retribution(source,target,*retribution(12),100));
  const auto states=*w.status_snapshot(target,100);const auto at=std::find_if(states.begin(),states.end(),[](const auto& e){return e.skill==248;});check(at!=states.end());
  check(w.maintained_enchantment_count(source,100)==2&&w.settle_resources(target,1100)&&w.resources(2)->health==506);
  check(w.cancel_maintained_enchantment(source,at->instance,1100)&&w.maintained_enchantment_count(source,1100)==1);
  check(w.settle_resources(target,2100)&&w.resources(2)->health==512);
 }
 // A lethal application reaction must retire the just-added lease exactly once.
 {
  ++cases;tested_case=cases;world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*10),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==2?1u:500u,1000,100,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.soul_barbs(*w.effect_scope(3),target,*soul_barbs(12),20,100));box->drain();check(w.stage_retribution(source,target,*retribution(12),100));
  check(w.resources(2)->health==0&&w.maintained_enchantment_count(source,100)==0);
  const auto packets=box->drain();check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==63;})==1);
 }
 std::cout<<"PASS "<<cases<<" staged Retribution World application/cancel/generic end/lifetime/late observer and exact upkeep-zero cases; no attack consumer or paid Runtime binding\n";
}catch(const std::exception& e){std::cerr<<"case "<<tested_case<<": "<<e.what()<<'\n';return 1;}
