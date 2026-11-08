#include "gw_recall.h"
#include "gw_recall_wire.h"
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Recall policy");}
int main(){
 const auto p=recall_shadow();check(bool(p));const Scope caster{1,1,1},target{1,2,1};
 const auto anchor=prepare_recall_anchor(*p,caster,target,100,1000,true,true,true,1248);check(bool(anchor));
 check(!prepare_recall_anchor(*p,caster,caster,100,1000,true,true,true,0));
 check(!prepare_recall_anchor(*p,caster,target,100,1000,true,true,false,100));
 check(!prepare_recall_anchor(*p,caster,target,100,1000,true,true,true,1248.01));
 const auto end=recall_end_action(*p,*anchor,caster,target,true,true,2000);check(end&&end->step&&end->disabled_until==12000);
 auto reused=target;++reused.generation;
 for(auto destination:{target,reused})for(bool alive:{false,true}){
  const auto action=recall_end_action(*p,*anchor,caster,destination,true,alive,2000);
  check(action&&action->step==(destination==target&&alive));
  check(action->disabled_until==(action->step?12000:0));
 }
 check(!recall_end_action(*p,*anchor,caster,target,true,true,999));
 check(!recall_end_action(*p,*anchor,caster,target,true,true,UINT64_MAX));
 check(!recall_end_action(*p,*anchor,reused,target,true,true,2000));
 check(!recall_end_action(*p,*anchor,caster,target,false,true,2000)->step);
 auto forged=*p;forged.disable=0;check(!recall_end_action(forged,*anchor,caster,target,true,true,2000));
 const RecallAnchor captured{{1,25,1},{1,28,1},82,1000};
 const Entry entry{82,25,925,0,Category::enchantment,1000,{}};
 const auto wire=recall_wire(captured,entry);check(bool(wire));
 const fw::gw::combat::Packet expected{62,0,25,0,0,0,25,0,0,0,157,3,0,0,0,0,82,0,0,0};
 check(wire->upkeep==expected&&wire->effect.size()==20&&wire->effect[0]==64);
 const auto removed=recall_end_wire(captured,entry);check(removed&&removed->upkeep==fw::gw::combat::Packet{63,0,25,0,0,0,82,0,0,0});
 auto invalid=entry;invalid.cause=28;check(!recall_wire(captured,invalid));
}
