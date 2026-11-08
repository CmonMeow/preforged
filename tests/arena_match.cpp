#include "gw_arena_match.h"
#include "gw_arena_death_time.h"
#include "gw_arena_queue.h"
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("arena match lifecycle regression");}
int main(){using namespace fw::gw::arena;
 // Delayed observers see both corpses together. The earlier actual death,
 // even when second in participant order, owns the common return clock.
 for(bool reverse:{false,true}){
  MatchLifecycle timed({70,80});
  const std::array<std::optional<std::uint64_t>,2> deaths=reverse?
   std::array<std::optional<std::uint64_t>,2>{1000,11000}:
   std::array<std::optional<std::uint64_t>,2>{11000,1000};
  check(record_deaths(timed,{70,80},deaths)==1&&timed.return_deadline()==11000);
  check(record_deaths(timed,{70,80},deaths)==0);
  const auto result=timed.poll(20000);check(result&&result->requeue==(reverse?80u:70u));
 }
 {MatchLifecycle simultaneous({90,100});
  check(record_deaths(simultaneous,{90,100},{1000,1000})==2);
  check(!simultaneous.poll(11000)->requeue);}
 {MatchLifecycle zero({110,120});check(record_deaths(zero,{110,120},{std::nullopt,0})==1);
  check(zero.return_deadline()==10000&&zero.poll(10000)->requeue==110);}
 {MatchLifecycle none({130,140});check(record_deaths(none,{130,140},{})==0&&!none.return_deadline());}
 // One loaded connection cannot start the pair. Retries do not shift the
 // common start time, including readiness arriving with sampled clock skew.
 for(bool reverse:{false,true}){
  MatchLifecycle loading({10,20});const unsigned first=reverse?20:10,second=reverse?10:20;
  check(!loading.is_ready(first)&&!loading.started_at());
  check(!loading.mark_ready(30,50));check(loading.mark_ready(first,100));
  check(loading.is_ready(first)&&!loading.is_ready(second)&&!loading.started_at());
  check(loading.mark_ready(first,1000)&&!loading.started_at());
  check(loading.mark_ready(second,90)&&loading.started_at()==100);
  check(loading.mark_ready(second,2000)&&loading.mark_ready(first,3000)&&loading.started_at()==100);
  check(!loading.mark_ready(30,4000));
 }
 {MatchLifecycle loading({30,40});check(loading.mark_ready(30,0));
  check(loading.is_ready(30)&&!loading.started_at());check(loading.mark_ready(40,500));
  check(loading.started_at()==500);check(loading.death(40,600));
  check(!loading.mark_ready(30,700)&&!loading.mark_ready(40,700));
  check(loading.poll(10600).has_value()&&!loading.mark_ready(30,10700));}
 {MatchLifecycle aborted({50,60});check(aborted.mark_ready(50,100));check(aborted.death(50,200));
  check(!aborted.mark_ready(60,300)&&!aborted.started_at());}
 MatchLifecycle match({1,2});check(!match.poll(100000));check(!match.death(3,100));
 check(match.death(2,1000));check(!match.death(2,9000));check(match.return_deadline()==11000);
 check(!match.poll(10999));const auto result=match.poll(11000);
 check(result&&result->returning==std::array<std::uint32_t,2>{1,2}&&result->requeue==1);
 check(!match.poll(11001)&&!match.death(1,11001));
 Queue queue;const auto waiting=queue.join(7,*result->requeue,11000);
 check(waiting&&waiting->remaining==30&&waiting->deadline==41000);
 check(queue.join(7,3,21000)->remaining==20);check(queue.expire(7,41000).has_value());
 MatchLifecycle tie({4,5});check(tie.death(4,100));check(tie.death(5,101));
 check(!tie.poll(10100)->requeue);
 // Scheduler latency must not turn a completed victory into a later tie.
 for(auto late:{11000u,12000u}){
  MatchLifecycle delayed({6,7});check(delayed.death(7,1000));
  check(!delayed.death(6,late));
  const auto victory=delayed.poll(20000);
  check(victory&&victory->returning==std::array<std::uint32_t,2>{6,7}&&victory->requeue==6);
  check(!delayed.poll(20001));
 }
}
