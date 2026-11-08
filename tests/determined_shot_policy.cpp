#include "gw_determined_shot.h"
#include "gw_determined_delivery.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace combat::weapon;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Determined policy line "+std::to_string(at.line()));}
int main()try{
 const AttackScope owner{1,2,3},target{1,4,5};const auto plan=*determined_shot(12);
 for(unsigned rank=0;rank<=20;++rank)check(determined_shot(rank)->bonus==5+rank);
 for(auto outcome:{AttackOutcome::hit,AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction,AttackOutcome::target_retired,AttackOutcome::cancelled,AttackOutcome::replaced}){
  const auto result=determined_outcome(plan,owner,{owner,target,7,100,200,outcome,{}});check(result&&result->recharge==(outcome==AttackOutcome::miss||outcome==AttackOutcome::block||outcome==AttackOutcome::dodge||outcome==AttackOutcome::obstruction));
 }
 const auto event=*determined_outcome(plan,owner,{owner,target,7,100,200,AttackOutcome::miss,{}});
 AttackCooldowns snapshot{owner,1,0,200,{{{402,0},10200,0},{{403,9},0,1200},{{400,0},200,0},{{281,0},5000,6000},{{1,0},5000,0},{{403,10},1200,0}}};
 auto world_only=snapshot;world_only.local_recharge_known=false;check(!prepare_determined_recharge(world_only,event));
 for(auto& slot:world_only.slots)slot.recharge_until=0;const auto partial=prepare_determined_recharge(world_only,event);check(partial&&partial->barriers.size()==4&&!partial->before.local_recharge_known);
 const auto proposal=prepare_determined_recharge(snapshot,event);check(proposal&&proposal->cleared==std::vector<skills::Slot>{{402,0},{403,9},{403,10}});
 check(proposal->barriers==std::vector<skills::Slot>{{402,0},{403,9},{400,0},{403,10}});
 {skills::effects::OrderedAttackCooldowns late(owner);using D=skills::effects::OrderedAttackCooldowns::Result;
 check(deliver_determined_recharge(late,*proposal,{owner,3,200})==D::accepted);
 check(late.put({owner,2,150},{400,0},1000)==D::accepted&&late.until(owner,{400,0},200)==0);}
 check(proposal->after.slots[2]==snapshot.slots[2]&&proposal->after.slots[3]==snapshot.slots[3]&&proposal->after.slots[4]==snapshot.slots[4]);
 auto current=snapshot;check(commit_determined_recharge(current,*proposal));check(!commit_determined_recharge(current,*proposal));
 {
  auto reordered=proposal->after;reordered.at=300;
  auto earlier_release=event;earlier_release.event=3;earlier_release.at=300;
  const auto next=prepare_determined_recharge(reordered,earlier_release);
  check(next&&commit_determined_recharge(reordered,*next)&&reordered.last_event==3);
  auto same_time=earlier_release;same_time.event=2;
  const auto second=prepare_determined_recharge(reordered,same_time);check(second&&commit_determined_recharge(reordered,*second));
  check(!prepare_determined_recharge(reordered,earlier_release)&&!prepare_determined_recharge(reordered,same_time));
  auto old=event;reordered.at=old.at;check(!prepare_determined_recharge(reordered,old));
  reordered=proposal->after;reordered.slots.resize(9);check(!prepare_determined_recharge(reordered,event));
 }
 auto changed=snapshot;++changed.revision;check(!commit_determined_recharge(changed,*proposal));
 changed=snapshot;changed.slots[0].recharge_until=20200;check(!commit_determined_recharge(changed,*proposal));
 auto forged=*proposal;forged.after.slots[3].recharge_until=0;check(!commit_determined_recharge(snapshot,forged));
 changed=snapshot;changed.slots.push_back(changed.slots.front());check(!prepare_determined_recharge(changed,event));
 changed=snapshot;++changed.at;check(!prepare_determined_recharge(changed,event));
 auto stale=owner;++stale.generation;check(!determined_outcome(plan,stale,{owner,target,7,100,200,AttackOutcome::miss,{}}));
 check(!determined_outcome(plan,owner,{owner,target,7,100,200,AttackOutcome::miss,150}));
 auto invalid=plan;++invalid.bonus;check(!determined_outcome(invalid,owner,{owner,target,7,100,200,AttackOutcome::miss,{}}));
 auto hit=event;hit.recharge=false;check(!prepare_determined_recharge(snapshot,hit));
 auto replay=event;replay.event=0;check(!prepare_determined_recharge(snapshot,replay));
 auto exhausted=snapshot;exhausted.revision=UINT64_MAX;check(!prepare_determined_recharge(exhausted,event));
 {skills::effects::OrderedAttackCooldowns delivery(owner);using Delivery=skills::effects::OrderedAttackCooldowns::Result;
 check(delivery.put({owner,1,100},{402,0},10200)==Delivery::accepted);
 check(delivery.put({owner,4,300},{402,0},20200)==Delivery::accepted);
 check(deliver_determined_recharge(delivery,*proposal,{owner,3,200})==Delivery::accepted);
 check(delivery.put({owner,2,150},{403,9},1200)==Delivery::accepted);
 check(delivery.until(owner,{402,0},300)==20200&&delivery.until(owner,{403,9},300)==0);
 check(deliver_determined_recharge(delivery,*proposal,{owner,3,200})==Delivery::duplicate);
 const auto before=delivery;check(deliver_determined_recharge(delivery,forged,{owner,5,200})==Delivery::invalid&&delivery==before);
 check(deliver_determined_recharge(delivery,*proposal,{stale,5,200})==Delivery::invalid&&delivery==before);
 check(deliver_determined_recharge(delivery,*proposal,{owner,5,201})==Delivery::invalid&&delivery==before);
 }
 {
  DeterminedDelivery retained(owner);using R=DeterminedDelivery::Result;
  const DeterminedNotice notice{{owner,1,200},*proposal};
  check(retained.publish(notice)==R::accepted&&retained.publish(notice)==R::duplicate);
  check(retained.acknowledged_order(owner)==0&&!retained.acknowledged_order(stale));
  check(retained.pending(owner)->size()==1&&retained.pending(owner)->size()==1&&!retained.pending(stale));
  auto conflict=notice;conflict.recharge.trigger.event=8;conflict.recharge=*prepare_determined_recharge(snapshot,conflict.recharge.trigger);
  const auto saved=retained;check(retained.publish(conflict)==R::invalid&&retained==saved);
  check(!retained.acknowledge({owner,1,201})&&!retained.acknowledge({stale,1,200})&&retained==saved);
  auto repeated=notice;repeated.stamp.order=2;check(retained.publish(repeated)==R::invalid);
  for(std::uint64_t order=2;order<=64;++order){auto next=notice;next.stamp.order=order;auto trigger=event;trigger.event=100+order;
   next.recharge=*prepare_determined_recharge(snapshot,trigger);check(retained.publish(next)==R::accepted);}
  auto full=notice;full.stamp.order=65;auto trigger=event;trigger.event=165;full.recharge=*prepare_determined_recharge(snapshot,trigger);
  check(retained.publish(full)==R::full&&retained.pending(owner)->size()==64);
  check(!retained.acknowledge({owner,65,200}));
  check(retained.acknowledge({owner,32,200})&&retained.pending(owner)->size()==32);
  check(retained.acknowledged_order(owner)==32);
  check(retained.acknowledge({owner,32,200})&&!retained.acknowledge({owner,31,200}));
  check(retained.publish(notice)==R::stale&&retained.publish(full)==R::accepted);
  check(retained.acknowledge(full.stamp)&&retained.pending(owner)->empty());
  auto backdated=notice;backdated.stamp={owner,66,199};backdated.recharge.before.at=199;backdated.recharge.trigger.at=199;
  backdated.recharge=*prepare_determined_recharge(backdated.recharge.before,backdated.recharge.trigger);
  check(retained.publish(backdated)==R::invalid);
 }
 std::cout<<"PASS Determined exact failed-hit policy,impact snapshot,attack-only/copied timers,atomic revision commit and stale/forged/replayed rejection;World/Runtime unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
