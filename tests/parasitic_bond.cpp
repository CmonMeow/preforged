#include "gw_parasitic_bond.h"
#include "gw_health_regeneration.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Parasitic Bond ledger regression line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank:{0u,12u,20u}){auto p=parasitic_bond(rank);check(p&&p->healing==30+6*rank&&p->pips==1&&p->duration==20000);}
 check(!parasitic_bond(21));Scope a{1,1,4},b{1,2,8},c{1,3,9};ParasiticBonds bonds;
 check(bonds.add({a,b,10,12,100,20100}));check(!bonds.add({a,b,10,12,100,20100}));
 check(!bonds.end(b,10,99,true)&&bonds.size()==1);check(!bonds.end({1,2,9},10,1000,true));
 check(bonds.expire(20099).empty());auto ended=bonds.expire(50100);
 check(ended.size()==1&&ended[0].caster==a&&ended[0].healing==102&&ended[0].at==20100&&bonds.size()==0);
 check(bonds.expire(UINT64_MAX).empty()&&!bonds.end(b,10,60000,true));
 check(bonds.add({a,b,11,0,100,20100}));auto removed=bonds.end(b,11,1100,true);
 check(removed&&removed->at==1100&&removed->healing==30);check(!bonds.end(b,11,1100,true));
 check(bonds.add({a,b,12,0,100,20100})&&bonds.add({a,c,13,20,200,20200}));
 check(bonds.take_caster({1,1,5}).empty()&&bonds.size()==2);const auto death=bonds.take_caster(a);
 check(death.size()==2&&bonds.size()==0&&bonds.expire(50100).empty());
 check(bonds.add({a,b,14,12,100,20100}));bonds.discard({1,2,9});check(bonds.size()==1);bonds.discard(b);check(bonds.size()==0);
 check(bonds.add({a,b,15,12,100,20100}));check(!bonds.end(b,15,1100,false)&&bonds.size()==0);
 for(const auto& bad:std::vector<ParasiticBonds::Link>{{a,a,1,0,0,20000},{a,{2,2,8},1,0,0,20000},{a,b,0,0,0,20000},{a,b,1,21,0,20000},{a,b,1,0,0,19999},{a,b,1,0,UINT64_MAX-100,19900}})check(!bonds.add(bad));
 check(bonds.add({a,b,20,0,300,20300})&&bonds.add({c,b,21,0,100,20100}));ended=bonds.expire(30000);
 check(ended.size()==2&&ended[0].caster==c&&ended[0].at==20100&&ended[1].caster==a&&ended[1].at==20300);
 {
  ParasiticBonds refresh;check(refresh.add({a,b,1,0,100,20100}));
  const auto before=refresh.lookup(b,1);check(before.has_value());
  check(!refresh.renew_same_caster(b,1,{c,b,2,12,1100,21100},1100));
  check(refresh.lookup(b,1)==before&&refresh.size()==1);
  check(!refresh.renew_same_caster(b,1,{a,b,2,12,99,20099},99));
  check(!refresh.renew_same_caster(b,1,{a,b,2,12,1100,21099},1100));
  check(refresh.lookup(b,1)==before);
  check(refresh.renew_same_caster(b,1,{a,b,2,12,1100,21100},1100));
  check(!refresh.lookup(b,1)&&refresh.lookup(b,2)->rank==12);
  check(refresh.expire(20100).empty());const auto reward=refresh.expire(21100);
  check(reward.size()==1&&reward[0].healing==102&&reward[0].at==21100);
 }
 {
  ParasiticBonds capacity;
  for(unsigned id=1;id<=4096;++id)check(capacity.add({a,b,id,0,100,20100}));
  check(!capacity.add({a,b,4097,0,100,20100})&&capacity.size()==4096);
  check(capacity.renew_same_caster(b,1,{a,b,4097,12,1100,21100},1100)&&capacity.size()==4096);
  check(!capacity.lookup(b,1)&&capacity.lookup(b,4097)->rank==12);
  check(capacity.expire(20100).size()==4095&&capacity.size()==1);
  check(capacity.expire(21100).size()==1&&capacity.size()==0);
 }
 {
  // A delayed world sample crosses three deadlines. The first callback may
  // discover that its caster died, invalidating that caster's later return.
  ParasiticBonds timeline;
  check(timeline.add({a,b,1,0,100,20100})&&timeline.add({a,c,2,12,200,20200}));
  check(timeline.add({c,b,3,20,300,20300}));
  check(timeline.next_deadline()==20100);
  std::vector<unsigned> dispatched;const auto count=timeline.dispatch_due(50000,[&](const auto& reward){
   dispatched.push_back(reward.effect);
   if(reward.effect==1){
    check(timeline.next_deadline()==20200);
    const auto cancelled=timeline.take_caster(a);check(cancelled.size()==1&&cancelled[0].effect==2);
    check(timeline.dispatch_due(50000,[](const auto&){throw std::runtime_error("nested dispatch overtook current event");})==0);
    check(!timeline.add({a,b,4,0,100,20100})); // Cannot inject a past event from a callback.
   }
  });
  check(count==2&&dispatched==std::vector<unsigned>{1,3}&&timeline.size()==0&&!timeline.next_deadline());
 }
 {
  // A callback can renew another pending hex before its expiry. The deadline
  // index must drop the old event and retain the replacement beyond this tick.
  ParasiticBonds timeline;
  check(timeline.add({a,b,1,0,100,20100})&&timeline.add({a,c,2,0,200,20200}));
  check(timeline.dispatch_due(21000,[&](const auto& reward){
   check(reward.effect==1&&timeline.renew_same_caster(c,2,{a,c,3,12,20100,40100},20100));
  })==1);
  check(timeline.next_deadline()==40100&&timeline.expire(40099).empty());
  const auto returned=timeline.expire(40100);check(returned.size()==1&&returned[0].effect==3&&returned[0].healing==102);
 }
 {
  ParasiticBonds timeline;check(timeline.add({a,b,1,0,100,20100})&&timeline.add({a,c,2,0,200,20200}));
  bool threw=false;try{timeline.dispatch_due(30000,[](const auto&){throw std::runtime_error("owner transaction failed");});}
  catch(const std::runtime_error&){threw=true;}
  check(threw&&timeline.size()==1&&timeline.next_deadline()==20200);
  check(timeline.dispatch_due(30000,[](const auto& reward){check(reward.effect==2);})==1);
 }
 {
  // Exercise the actual Health integrator, not just event order. At -3 pips,
  // advancing straight to40s kills this caster before an overdue heal can be
  // applied. Processing its20s return first leaves10HP, identically for dense
  // and sparse ticks. This proves the required caller contract, not World wiring.
  for(unsigned actor:{1u,3u})for(unsigned cadence:{100u,40000u}){
   const Scope source{1,actor,4},target{1,actor==1?3u:1u,8};
   ParasiticBonds timeline;check(timeline.add({source,target,1,0,100,20100}));
   fw::gw::skills::regeneration::State health(source,100);double value=220;
   for(Time now=100+cadence;now<=40100;now+=cadence){
    timeline.dispatch_due(now,[&](const auto& returned){
     check(health.advance(source,returned.at,value,1000,-3));
     if(value>0)value=std::min(1000.,value+returned.healing);
    });
    check(health.advance(source,now,value,1000,-3));
   }
   check(std::abs(value-10)<1e-8&&timeline.size()==0);
  }
  fw::gw::skills::regeneration::State late(a,100);double corpse=220;
  check(late.advance(a,40100,corpse,1000,-3)&&corpse==0);
 }
 std::cout<<"PASS scoped Parasitic Bond returns,atomic renewal and cancellable chronological dispatch;world behavior is not enabled\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

