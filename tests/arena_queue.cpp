#include "gw_arena_queue.h"
#include <stdexcept>
#include <thread>
#include <vector>
#include <atomic>
void check(bool value){if(!value)throw std::runtime_error("arena queue regression");}
int main(){using fw::gw::arena::Queue;Queue q;
 auto first=q.join(2,10,1000);check(first&&first->deadline==31000&&first->remaining==30);
 auto retry=q.join(2,10,6000);check(retry&&retry->deadline==31000&&retry->remaining==25);
 auto second=q.join(2,20,11500);check(second&&second->deadline==31000&&second->remaining==20&&second->players==2);
 check(q.inspect(2,10,11500)->remaining==second->remaining);
 check(!q.join(2,30,12000)&&!q.join(3,20,12000));
 check(!q.expire(2,30999));auto match=q.expire(2,31000);check(match&&match->players[0]==10&&match->players[1]==20);
 check(!q.expire(2,31000)&&!q.inspect(2,10,31001));
 check(q.join(2,10,40000).has_value());check(q.leave(2,10));check(!q.inspect(2,10,40001));
 check(q.join(2,10,50000).has_value());check(q.join(2,20,80000)->remaining==30);check(!q.expire(2,80000));check(q.inspect(2,10,80000)->deadline==110000);check(q.leave(2,10));check(q.leave(2,20));
 check(q.join(2,10,90000).has_value());check(q.join(2,20,90001).has_value());check(q.leave(2,10));
 check(q.inspect(2,20,100000)->deadline==120000);check(q.join(2,30,100000)->remaining==20);
 auto next=q.expire(2,120000);check(next&&next->players[0]==20&&next->players[1]==30&&next->id!=match->id);
 check(q.join(4,40,200000).has_value());check(!q.expire(4,295001));check(q.inspect(4,40,295001)->deadline==320000);check(q.join(4,50,300000)->remaining==20);check(q.expire(4,320000).has_value());
 check(!q.join(0,1,0)&&!q.join(1,0,0)&&!q.join(1,1,UINT64_MAX));
 // Waiting never expires: keep membership through many windows, then cancel.
 Queue waiting;check(waiting.join(7,70,0).has_value());
 for(std::uint64_t window=1;window<=1000;++window){
  const auto now=window*30000;check(!waiting.expire(7,now));
  const auto state=waiting.inspect(7,70,now);
  check(state&&state->players==1&&state->remaining==30&&state->deadline==now+30000);
 }
 check(waiting.leave(7,70));check(!waiting.expire(7,30030000));
 check(!waiting.inspect(7,70,30030000));
 // Late scheduler delivery cannot leave a solo queue closed to new arrivals.
 Queue delayed;check(delayed.join(8,80,1000).has_value());
 check(delayed.inspect(8,80,95500)->deadline==121000);
 check(delayed.join(8,90,95500)->remaining==26);
 // A full pair still belongs to its original window until delivered once.
 check(!delayed.join(8,100,121000));
 check(delayed.inspect(8,80,150000)->deadline==121000);
 auto delayed_match=delayed.expire(8,150000);
 check(delayed_match&&delayed_match->players==std::array<Queue::Player,2>{80,90});
 check(!delayed.expire(8,150000));
 for(unsigned round=0;round<100;++round){
  Queue concurrent;std::atomic<unsigned> accepted=0;std::vector<std::thread> threads;
  for(unsigned n=1;n<=16;++n)threads.emplace_back([&,n]{if(auto s=concurrent.join(1,n,100)){check(s->deadline==30100);++accepted;}});
  for(auto& thread:threads)thread.join();check(accepted==2);
  auto pair=concurrent.expire(1,30100);check(pair&&pair->players[0]!=pair->players[1]);check(!concurrent.expire(1,30100));
 }
}
