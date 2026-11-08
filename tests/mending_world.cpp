#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
#include <thread>
#include <atomic>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mending World line "+std::to_string(at.line()));}
int main()try{
 // Range retirement is an event frontier,not a final-position test. Sparse
 // and dense resource polling must end upkeep/regeneration at the same time.
 for(bool reverse:{false,true})for(bool dense:{false,true}){
  world::Instance w;const unsigned source_id=reverse?2:1,target_id=reverse?1:2;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Party",id==source_id?0.f:500.f,0,id==source_id?box:std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(source_id),target=*w.effect_scope(target_id);
  check(w.stage_mending(source,target,*mending(12),100));const auto entry=w.status_snapshot(target,100)->front();const auto wire=*mending_end_wire(source,target,entry);w.move(target_id,9000,0,100);const auto outside=[&](unsigned at){const auto a=w.position(source_id,at),b=w.position(target_id,at);return std::hypot(double(a.first)-b.first,double(a.second)-b.second)>5000.;};check(!outside(100)&&outside(40100));unsigned inside=100,expected=40100;while(expected-inside>1){const auto middle=inside+(expected-inside)/2;if(outside(middle))expected=middle;else inside=middle;}box->drain();
  if(dense)for(unsigned at=237;at<expected;at+=137)check(w.settle_resources(source,at));check(w.settle_resources(source,expected-1)&&w.maintained_enchantment_count(source,expected-1)==1);check(w.settle_resources(source,expected)&&w.maintained_enchantment_count(source,expected)==0);const auto health=w.resources(target_id)->health;check(w.settle_resources(target,expected+10000)&&w.resources(target_id)->health==health);
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),wire.upkeep)==1&&std::count(packets.begin(),packets.end(),wire.effect)==1);check(w.complete_blessed_signet(source,*blessed_signet(12),expected+10000));
 }
 // A turning keyboard path retains the outer forecast horizon across an
 // earlier poison expiry;an unturned held path uses the same range retirement.
 for(unsigned mode=0;mode<3;++mode){
  const bool turning=mode!=0;const unsigned horizon=mode==2?55100:20100;
  world::Instance w;auto box=std::make_shared<world::Outbox>();for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==2?500.f:0.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{500,1000,100,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.stage_mending(source,target,*mending(12),100));const auto entry=w.status_snapshot(target,100)->front();const auto wire=*mending_end_wire(source,target,entry);w.steer(2,1,0,100);if(turning)w.rotate(2,std::numeric_limits<float>::infinity(),mode==2?288.f/(2500.f*2.094395f):.01f,100);
  const auto a=*w.maintained_range_path(source,100,horizon),b=*w.maintained_range_path(target,100,horizon);const auto exit=maintained_range_exit(a,b,100,horizon);check(exit&&exit->first_outside);if(mode==2)check(std::hypot(b.back().x,b.back().y)<5000);const auto expected=*exit->first_outside;
  check(w.apply_condition(*w.effect_scope(3),target,combat::conditions::Kind::poison,100,2000));box->drain();check(w.settle_resources(source,horizon));check(!w.maintained_enchantment_count(source,expected-1).has_value()&&w.maintained_enchantment_count(source,expected)==0);const auto health=w.resources(2)->health;check(w.settle_resources(target,horizon+10000)&&w.resources(2)->health==health);const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),wire.upkeep)==1&&std::count(packets.begin(),packets.end(),wire.effect)==1);
 }
 // Read-only range paths use the accepted movement schedule,not teleport
 // destinations,and cannot read stale lifetimes or fabricated keyboard paths.
 {world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Walker",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);const auto a=*w.effect_scope(1),b=*w.effect_scope(2);w.move(2,9000,0,100);box->drain();const auto caster=w.maintained_range_path(a,100,40100),target=w.maintained_range_path(b,100,40100);check(caster&&target&&target->size()==4&&target->front().x==500&&target->back().x==9000);const auto exit=maintained_range_exit(*caster,*target,100,40100);check(exit&&exit->first_outside);const auto outside=[&](unsigned at){const auto c=w.position(1,at),t=w.position(2,at);return std::hypot(double(c.first)-t.first,double(c.second)-t.second)>5000.;};unsigned inside=100,expected=40100;while(expected-inside>1){const auto middle=inside+(expected-inside)/2;if(outside(middle))expected=middle;else inside=middle;}check(*exit->first_outside<=expected&&expected-*exit->first_outside<=1);check(box->drain().empty());auto stale=b;++stale.generation;check(!w.maintained_range_path(stale,100,40100));check(!w.maintained_range_path(b,99,40100));w.steer(2,100,0,100);check(w.maintained_range_path(b,100,200).has_value());}
 // Both real Runtime casts share the same World ownership/Energy authority.
 // Also validate owner pip publication on application and manual cancellation.
 for(bool cancel_before_signet:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);w.set_vitals(1,{500,1000,50,100},100);const auto source=*w.effect_scope(1);
  skills::Slots bar{};bar[0]={290,0};bar[1]={297,0};skills::Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[290]=policy.learned[297]=1;attributes::Ranks ranks{};ranks[13]=ranks[16]=12;
  skills::casting::Hooks h;h.resources=[&](unsigned id){return w.resources(id);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float,bool){return id==1;};h.events=[&](const auto& packets){box->enqueue(packets);};h.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(1,cost,source,at);};h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
  h.mending_target_valid=[&](auto target,unsigned df,auto at){return w.mending_target_valid(source,target,df,at);};h.mending=[&](auto target,const auto& plan,unsigned df,auto at)->skills::casting::EffectCompletion{return {w.complete_mending(source,target,plan,df,at),{},{},{},false,true};};
  h.blessed_signet_ready=[&](auto owner,auto at){return w.blessed_signet_caster_ready(owner,at);};h.blessed_signet=[&](auto owner,const auto& plan,auto at)->skills::casting::EffectCompletion{return {w.complete_blessed_signet(owner,plan,at),{},{},{},false,true};};
  skills::casting::Runtime cast(1);check(cast.start(1000,{290,0,1},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);box->drain();cast.update(3000,h);check(w.maintained_enchantment_count(source,3000)==1);const auto state=*w.overcast_energy_snapshot(source);const auto rate=vitals::energy_regeneration_2022(1,state.curve.effective_energy_pips(2,3000),100);const auto applied=box->drain();check(std::count(applied.begin(),applied.end(),rate)>=1);
  if(cancel_before_signet){const auto status=w.status_snapshot(source,3000)->front();check(w.cancel_maintained_enchantment(source,status.instance,3000));const auto restored=vitals::energy_regeneration_2022(1,2,100);const auto removed=box->drain();check(std::count(removed.begin(),removed.end(),restored)>=1);}
  check(cast.start(3750,{297,0,999},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);check(w.settle_resources(source,5750));const auto before=*w.overcast_energy_snapshot(source);box->drain();cast.update(5750,h);cast.update(5750,h);const auto after=*w.overcast_energy_snapshot(source);
  check(overcast::precise_current(after)-overcast::precise_current(before)==(cancel_before_signet?0:3*overcast::energy_unit));const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::casting::terminal(58,1))==1);
 }
 for(bool area:{false,true}){world::Instance w;auto box=std::make_shared<world::Outbox>();for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*100),0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}const auto foe=*w.effect_scope(1),owner=*w.effect_scope(2);check(w.stage_mending(owner,owner,*mending(12),100));const auto entry=w.status_snapshot(owner,100)->front();const auto wire=*mending_end_wire(owner,owner,entry);box->drain();
  if(area)check(w.complete_chilblains(foe,owner,*chilblains(12),20,100));else check(w.enchantment_removal(foe,owner,*enchantment_removal(69,12),20,100));
  check(w.maintained_enchantment_count(owner,100)==0);const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),wire.upkeep)==1&&std::count(packets.begin(),packets.end(),wire.effect)==1);
 }
 // Normal player casts preserve ten-Energy payment even on cancellation.
 for(bool self:{false,true})for(bool secondary:{false,true})for(unsigned mode=0;mode<8;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{world::Packet{165,0,1,0,0,0,static_cast<std::uint8_t>(secondary?6:3),0}}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,1000,100,100},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto source=*w.effect_scope(1),destination=*w.effect_scope(self?1:2);skills::casting::Hooks h;unsigned applied=0;bool stale=false;
  h.resources=[&](unsigned id){return w.resources(id);};h.target_scope=[&](unsigned id){auto scope=w.effect_scope(id);if(stale&&scope)++scope->generation;return scope;};
  h.mending_target_valid=[&](auto target,unsigned df,auto now){return w.mending_target_valid(source,target,df,now);};h.target_valid=[](auto,float,bool){return false;};
  h.consume_energy_at=[&](unsigned cost,auto now){check(cost==10);return w.consume_energy(1,cost,source,now);};h.events=[&](const auto& packets){box->enqueue(packets);};
  h.before_spell_effect=[&](auto skill,auto event,auto now){return mode!=6&&w.before_spell_effect(source,skill,event,now);};
  if(mode!=1)h.mending=[&](auto target,const auto& plan,unsigned df,auto now)->skills::casting::EffectCompletion{++applied;check(df==(secondary?0u:12u));return {w.complete_mending(source,target,plan,df,now),{},{},{},false,true};};
  skills::Policy policy;policy.primary=secondary?6:3;policy.secondary=secondary?3:0;policy.campaigns=255;policy.learned[290]=1;skills::Slots bar{};bar[0]={290,0};attributes::Ranks ranks{};ranks[13]=mode==7?21:12;ranks[16]=secondary?0:12;skills::casting::Runtime cast(1);
  check(cast.start(1000,{290,0,destination.actor},bar,policy,ranks,true,h)==((mode==1||mode==7)?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));if(mode==1||mode==7){check(w.resources(1)->energy==100);continue;}check(w.resources(1)->energy==90);
  if(mode==2)cast.cancel(h);if(mode==3)cast.interrupt(2000,h);if(mode==4)stale=true;if(mode==5)h.mending={};box->drain();cast.update(2999,h);check(!applied);cast.update(3000,h);cast.update(3000,h);
  const bool success=mode==0;check(applied==unsigned(success));check(w.maintained_enchantment_count(source,3000)==unsigned(success));const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::casting::terminal(58,1))==unsigned(success));
  if(success){const auto status=*w.status_snapshot(destination,3000);check(status.size()==1&&!status.front().expires);const auto wire=*mending_wire(source,destination,status.front());check(std::count(packets.begin(),packets.end(),wire.upkeep)==1);check(w.resources(destination.actor)->health==(secondary?500u:538u));check(cast.start(3749,{290,0,destination.actor},bar,policy,ranks,true,h)==skills::casting::Reason::busy);check(cast.start(3750,{290,0,destination.actor},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);cast.cancel(h);}
 }
 // Signed Energy debt is repaid by the actual gain;it is not silently
 // converted to positive Energy or interpreted as loss of maintained buffs.
 {world::Instance w;
auto box=std::make_shared<world::Outbox>();
w.join(1,"Debt",0,0,box,0,{}, {},100,1);
w.set_vitals(1,{500,1000,0,40},100);
const auto owner=*w.effect_scope(1);
check(w.stage_mending(owner,owner,*mending(12),100));
check(w.energy_capacity(1,5,0,100,{0,0}));
const auto before=*w.overcast_energy_snapshot(owner);
check(overcast::precise_current(before)==-20*overcast::energy_unit);
box->drain();
check(w.complete_blessed_signet(owner,*blessed_signet(12),100));
const auto after=*w.overcast_energy_snapshot(owner);
check(overcast::precise_current(after)==-17*overcast::energy_unit&&after.resources.energy==0&&after.debt==17);
const auto packets=box->drain();
const auto wire=*blessed_signet_wire(1,3*overcast::energy_unit,20,overcast::energy_unit);
check(std::count(packets.begin(),packets.end(),wire.delta)==1);
}
 // Reduced fractional ceiling clips the3Energy request. Publish only the
 // settled gain and no unproven rounded integer popup.
 {world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Ceiling",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,89,100},100);const auto owner=*w.effect_scope(1);check(w.stage_mending(owner,owner,*mending(12),100)&&w.activate_overcast_energy(owner,231,0,1,100)&&w.settle_resources(owner,101));const auto before=*w.overcast_energy_snapshot(owner);box->drain();check(w.complete_blessed_signet(owner,*blessed_signet(12),101));const auto after=*w.overcast_energy_snapshot(owner);const auto gained=overcast::precise_current(after)-overcast::precise_current(before);check(gained>0&&gained<3*overcast::energy_unit&&gained%overcast::energy_unit!=0);check(std::abs(double(overcast::precise_current(after))/overcast::energy_unit-after.curve.ceiling(100))<.00001);const auto wire=*blessed_signet_wire(1,gained,100,overcast::energy_unit);check(!wire.popup);const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),wire.delta)==1);}
 for(unsigned mode=0;mode<8;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,50,100},100);const auto source=*w.effect_scope(1);
  skills::Slots bar{};bar[0]={297,0};skills::Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[297]=1;attributes::Ranks ranks{};ranks[16]=mode==7?21:12;
  skills::casting::Hooks h;unsigned commits=0;bool stale=false;h.resources=[&](unsigned id){return w.resources(id);};h.events=[&](const auto& packets){w.skill_event(1,packets);};
  h.target_scope=[&](unsigned id){return stale?std::optional<Scope>{}:w.effect_scope(id);};h.target_valid=[](unsigned id,float,bool){return id==1;};h.consume_energy_at=[&](unsigned cost,auto now){check(cost==0);return w.consume_energy(1,cost,source,now);};
  h.blessed_signet_ready=[&](Scope owner,auto now){return w.blessed_signet_caster_ready(owner,now);};
  if(mode!=1)h.blessed_signet=[&](Scope owner,const auto& plan,auto now)->skills::casting::EffectCompletion{++commits;return {w.complete_blessed_signet(owner,plan,now),{},{},{},false,true};};
  skills::casting::Runtime runtime(1);const auto result=runtime.start(1000,{297,0,999},bar,policy,ranks,true,h);check(result==((mode==1||mode==7)?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
  if(mode==1||mode==7)continue;
  box->drain();if(mode==2)runtime.cancel(h);if(mode==3)runtime.interrupt(1500,h);if(mode==4)stale=true;if(mode==5)h.blessed_signet={};
  runtime.update(2999,h);check(commits==0);runtime.update(3000,h);runtime.update(3000,h);const bool success=mode==0||mode==6;check(commits==unsigned(success));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::casting::terminal(58,1))==unsigned(success));
  if(success){check(runtime.start(12999,{297,0,0},bar,policy,ranks,true,h)==skills::casting::Reason::recharge);check(runtime.start(13000,{297,0,0},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);runtime.cancel(h);}
 }
 // Completion uses current ownership,not the recipient's buff count or a
 // stale cast-start count. Full Energy still emits finish/body without gain.
 for(unsigned mode=0;mode<6;++mode){world::Instance w;auto a=std::make_shared<world::Outbox>(),b=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Party",float(id*10),0,id==1?a:b,0,{}, {},100,1);w.set_vitals(id,{500,1000,mode==2?100u:50u,100},100);}
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),recipient=*w.effect_scope(2);
  if(mode!=0)check(w.stage_mending(mode==3?recipient:caster,mode==3?caster:recipient,*mending(12),100));
  if(mode==4){const auto effect=w.status_snapshot(recipient,100)->front().instance;check(w.cancel_maintained_enchantment(caster,effect,100));}
  a->drain();b->drain();auto source=caster;if(mode==5)++source.generation;
  check(w.complete_blessed_signet(source,*blessed_signet(12),100)==(mode!=5));
  const auto own=a->drain(),other=b->drain();const auto finish=combat::detail::integer(58,1,0),body=combat::detail::integer(21,1,523);
  check(std::count(own.begin(),own.end(),finish)==(mode!=5));check(std::count(other.begin(),other.end(),body)==(mode!=5));
  check(w.resources(1)->energy==(mode==2?100u:mode==1?53u:50u));
  const auto gain=*blessed_signet_wire(1,900000,100,300000);
  check(std::count(own.begin(),own.end(),gain.delta)==(mode==1));check(std::count(other.begin(),other.end(),gain.delta)==0);
 }
 // A delayed completion must settle the upkeep-zero boundary first. At3100
 // Mending has ended,so Blessed Signet cannot resurrect it or grant3Energy.
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Party",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,id==1?1u:100u,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.energy_capacity(1,3,0,100,{80,-2})&&w.stage_mending(source,target,*mending(12),100));check(w.complete_blessed_signet(source,*blessed_signet(12),3100));check(w.maintained_enchantment_count(source,3100)==0&&w.resources(1)->energy==0);}
 for(bool self:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto a=std::make_shared<world::Outbox>(),b=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,a,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);w.join(2,"Recipient",100,0,b,0,{}, {},100,mode==1?2:1);w.set_vitals(2,{500,1000,100,100},100);
  if(mode!=1)check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(self?1:2);auto plan=*mending(rank);if(mode==2)++plan.pips;auto selected=target;if(mode==3)++selected.generation;a->drain();b->drain();
  const bool accepted=w.stage_mending(caster,selected,plan,100);const bool reject=mode==2||mode==3||(!self&&mode==1);check(accepted==!reject);if(reject){check(w.maintained_enchantment_count(caster,100)==0);continue;}
  const auto statuses=*w.status_snapshot(target,100);check(statuses.size()==1&&statuses.front().skill==290&&!statuses.front().expires);const auto effect=statuses.front().instance;const auto wire=*mending_wire(caster,target,statuses.front());
  auto source_packets=a->drain(),target_packets=b->drain();check(std::count(source_packets.begin(),source_packets.end(),wire.upkeep)==1);check(std::count(target_packets.begin(),target_packets.end(),wire.upkeep)==0);check(std::count(source_packets.begin(),source_packets.end(),wire.effect)==1);
  if(mode==4){const auto snapshots=w.join(3,"Late observer",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);const auto snapshot=std::find_if(snapshots.begin(),snapshots.end(),[&](const auto& p){return p.id==target.actor;});check(snapshot!=snapshots.end()&&std::count(snapshot->presentation.begin(),snapshot->presentation.end(),wire.effect)==1);}
  check(w.settle_resources(target,1100));check(w.resources(target.actor)->health==500+2*plan.pips);
  auto wrong=caster;++wrong.generation;check(!w.cancel_maintained_enchantment(wrong,effect,1100));check(!w.cancel_maintained_enchantment(*w.effect_scope(2),effect,1100));
  if(mode==5){check(w.status_mutation(target,290,effect,rank,1100,{},false));}else check(w.cancel_maintained_enchantment(caster,effect,1100));
  check(w.maintained_enchantment_count(caster,1100)==0&&!w.cancel_maintained_enchantment(caster,effect,1100));check(w.settle_resources(target,2100));check(w.resources(target.actor)->health==500+2*plan.pips);
  const auto ended=*mending_end_wire(caster,target,statuses.front());source_packets=a->drain();check(std::count(source_packets.begin(),source_packets.end(),ended.upkeep)==1&&std::count(source_packets.begin(),source_packets.end(),ended.effect)==1);
 }

 {world::Instance w;auto source_box=std::make_shared<world::Outbox>();for(unsigned id:{1u,2u,3u}){w.join(id,"Party",float(id*10),0,id==1?source_box:std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100)&&w.party_action(1,158,3,100)&&w.party_action(3,154,1,100));
  const auto first=*w.effect_scope(1),target=*w.effect_scope(2),second=*w.effect_scope(3);check(w.stage_mending(first,target,*mending(12),100));const auto old=w.status_snapshot(target,100)->front();source_box->drain();check(w.stage_mending(second,target,*mending(0),1100));const auto current=w.status_snapshot(target,1100)->front();check(current.instance>old.instance&&w.maintained_enchantment_count(first,1100)==0&&w.maintained_enchantment_count(second,1100)==1);const auto packets=source_box->drain();const auto ended=*mending_end_wire(first,target,old),added=*mending_wire(second,target,current);check(std::find(packets.begin(),packets.end(),ended.effect)<std::find(packets.begin(),packets.end(),added.effect));check(!w.cancel_maintained_enchantment(first,current.instance,1100));check(w.settle_resources(target,2100)&&w.resources(2)->health==508);check(w.cancel_maintained_enchantment(second,current.instance,2100));}
 // A reaction during the captured application batch may remove the newly
 // installed effect;generic end publication must reuse the existing capture.
 {world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{1,1000,100,100},100);w.join(3,"Foe",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);check(w.soul_barbs(*w.effect_scope(3),target,*soul_barbs(12),20,100));box->drain();check(w.stage_mending(caster,target,*mending(12),100));check(w.resources(2)->health==0&&w.maintained_enchantment_count(caster,100)==0);const auto packets=box->drain();check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==63;})==1);}

 for(unsigned attempt=0;attempt<24;++attempt){world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);const auto caster=*w.effect_scope(1);check(w.stage_mending(caster,caster,*mending(12),100));const auto entry=w.status_snapshot(caster,100)->front();const auto ended=*mending_end_wire(caster,caster,entry);box->drain();std::atomic<bool> ready=false,stop=false,split=false;
  const auto inspect=[&](const auto& packets){if(std::count(packets.begin(),packets.end(),ended.upkeep)!=std::count(packets.begin(),packets.end(),ended.effect))split=true;};
  std::thread observer([&]{ready=true;while(!stop.load()){inspect(box->drain());std::this_thread::yield();}});while(!ready.load())std::this_thread::yield();const bool removed=w.cancel_maintained_enchantment(caster,entry.instance,1100);stop=true;observer.join();inspect(box->drain());check(removed&&!split.load());}

 // Settle the recipient first,under both numeric actor orders and sparse/dense
 // polling. Source Energy reaches zero at3100,ending exactly3s of regeneration.
 for(bool reverse:{false,true})for(bool dense:{false,true}){
  world::Instance w;const unsigned source_id=reverse?2:1,target_id=reverse?1:2;for(unsigned id:{1u,2u}){w.join(id,"Party",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,id==source_id?1u:100u,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));check(w.energy_capacity(source_id,3,0,100,{80,-2}));const auto caster=*w.effect_scope(source_id),target=*w.effect_scope(target_id);check(w.stage_mending(caster,target,*mending(12),100));
  if(dense)for(unsigned at=117;at<3099;at+=17)check(w.settle_resources(target,at));check(w.settle_resources(target,3099));check(w.maintained_enchantment_count(caster,3099)==1);check(w.settle_resources(target,3100));check(w.maintained_enchantment_count(caster,3100)==0);check(w.settle_resources(target,6100));check(w.resources(source_id)->energy==0&&w.resources(target_id)->health==518);
 }
 // Source degeneration death must retire the recipient's benefit at850,
 // before a distant recipient-first Health query integrates the rest of time.
 for(bool reverse:{false,true}){world::Instance w;const unsigned source_id=reverse?2:1,target_id=reverse?1:2;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==source_id?6u:500u,1000,100,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(source_id),target=*w.effect_scope(target_id);check(w.stage_mending(caster,target,*mending(12),100));check(w.apply_condition(*w.effect_scope(3),caster,combat::conditions::Kind::poison,100,10000));check(w.settle_resources(target,6100));check(w.resources(source_id)->health==0&&w.resources(target_id)->health==505&&w.maintained_enchantment_count(caster,6100)==0);}
 for(bool target_leaves:{false,true}){world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Party",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);check(w.stage_mending(caster,target,*mending(12),100));const auto effect=w.status_snapshot(target,100)->front().instance;check(w.settle_resources(target,1100));const auto departed=target_leaves?2u:1u;w.leave(departed);w.join(departed,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},1100,1);w.set_vitals(departed,{500,1000,100,100},100);check(!w.cancel_maintained_enchantment(*w.effect_scope(departed),effect,1100));if(target_leaves)check(w.maintained_enchantment_count(caster,1100)==0);else check(w.settle_resources(target,2100)&&w.resources(2)->health==506);}

 // Reciprocal maintained links settle both Energy pools before either Health
 // participant crosses a simultaneous zero boundary.
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Party",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,1,100},100);check(w.energy_capacity(id,3,0,100,{80,-2}));}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.stage_mending(a,b,*mending(12),100)&&w.stage_mending(b,a,*mending(12),100));check(w.settle_resources(a,6100));check(w.resources(1)->health==518&&w.resources(2)->health==518&&w.resources(1)->energy==0&&w.resources(2)->energy==0&&w.maintained_enchantment_count(a,6100)==0&&w.maintained_enchantment_count(b,6100)==0);}
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Party",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,100,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.stage_mending(a,b,*mending(12),100));check(w.settle_resources(b,1100));w.set_vitals(1,{500,1000,100,100},100);check(w.maintained_enchantment_count(a,1100)==0);check(w.settle_resources(b,2100)&&w.resources(2)->health==506);}

 for(bool lethal:{false,true}){world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==1&&lethal?20u:500u,1000,id==1?1u:100u,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);check(w.energy_capacity(1,3,0,100,{80,-2})&&w.stage_mending(caster,target,*mending(12),100)&&w.apply_mind_wrack(*w.effect_scope(3),caster,12,false,100));check(w.settle_resources(target,6100));check(w.resources(1)->health==(lethal?0u:500-mind_wrack(12)->zero_damage)&&w.resources(2)->health==518);}

 {world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==1?2u:500u,1000,id==1?1u:100u,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);check(w.energy_capacity(1,3,0,100,{80,-2})&&w.stage_mending(caster,target,*mending(12),100));check(w.stage_energy_degeneration_hex(caster,*w.effect_scope(3),*energy_degeneration_hex(140,12),20,100));check(w.settle_resources(target,6100));check(w.resources(1)->health==0&&w.resources(2)->health==506&&w.maintained_enchantment_count(caster,6100)==0);}

 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Party",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,id==1?1u:100u,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);check(w.energy_capacity(1,3,0,100,{80,-2})&&w.stage_mending(caster,target,*mending(12),100));check(w.consume_energy(1,1,caster,100));check(w.maintained_enchantment_count(caster,100)==0);}

 {world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Party",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{500,1000,id==1?1u:100u,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100)&&w.party_action(1,158,3,100)&&w.party_action(3,154,1,100));const auto caster=*w.effect_scope(1),older=*w.effect_scope(2),newer=*w.effect_scope(3);check(w.energy_capacity(1,3,0,100,{80,-1})&&w.stage_mending(caster,older,*mending(12),100)&&w.stage_mending(caster,newer,*mending(12),100));const auto retained=w.status_snapshot(older,100)->front().instance;check(w.settle_resources(older,6100)&&w.settle_resources(newer,6100));check(w.resources(2)->health==536&&w.resources(3)->health==518&&w.maintained_enchantment_count(caster,6100)==1);check(w.cancel_maintained_enchantment(caster,retained,6100)&&w.settle_resources(caster,9100)&&w.resources(1)->energy==1);}

 {std::array<unsigned,2> final{};for(unsigned cadence=0;cadence<2;++cadence){world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==2?1000u:500u,1000,id==1?1u:100u,100},100);}check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);check(w.energy_capacity(1,3,0,100,{80,-2})&&w.apply_condition(*w.effect_scope(3),target,combat::conditions::Kind::deep_wound,100,3000)&&w.stage_mending(caster,target,*mending(12),100));if(cadence)for(unsigned at=117;at<6100;at+=17)check(w.settle_resources(target,at));check(w.settle_resources(target,6100));final[cadence]=w.resources(2)->health;}check(final[0]==final[1]);}
 std::cout<<"PASS42 World Mending self/ally rank,scope,allegiance,indefinite regeneration,late observer,owned cancellation and generic-removal cases plus replacement/reaction-death and24 concurrent drain regressions plus15 upkeep/lifetime/shared-zero/spend/multiple-owner cases and DeepWound cadence guard;Runtime/World hook casting covered; rendered acceptance separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

