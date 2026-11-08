#include "gw_recall_end_queue.h"
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Recall end queue");}
int main(){
 const Scope a{1,1,1},b{1,2,1};RecallEndQueue queue;
 const RecallAnchor first{a,b,100,1000};check(queue.enqueue(first,2000));
 const auto pending=*queue.front();check(pending.anchor.effect==100&&pending.at==2000);
 check(!queue.enqueue(first,2000)&&queue.size()==1); // Recursive removal cannot duplicate work.
 check(!queue.acknowledge(pending.event+1)&&queue.front()->event==pending.event);
 check(queue.front()->event==pending.event); // Failed preparation leaves the same work pending.
 check(queue.enqueue({b,a,1,1000},2000));check(!queue.enqueue({a,b,101,1000},1999));
 check(queue.acknowledge(pending.event)&&queue.front()->anchor.caster==b);
 check(!queue.enqueue(first,2001)); // Completed work retains its replay watermark.
 queue.retire(a);check(queue.size()==1); // Target retirement does not erase another caster's end.
 queue.retire(b);check(!queue.front());
 check(!queue.enqueue({b,a,2,1000},1999)); // An empty queue retains its global frontier.
 auto reused=a;++reused.generation;check(queue.enqueue({reused,b,1,3000},3000));
 check(!queue.enqueue({reused,reused,2,3000},3000));
}
