#include "gw_lightning_strike.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location where=std::source_location::current()){
 if(!ok)throw std::runtime_error("Lightning Strike policy line "+std::to_string(where.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto plan=lightning_strike(rank);check(plan&&plan->damage==5+3*rank&&plan->penetration==25&&plan->duration==3000);}
 check(!lightning_strike(21));
 using Timers=LightningStrikeTimers;const Scope a{1,1,1},b{1,2,1},c{1,3,1};
 const Timers::Link first{a,b,1,12,20,1000,4000};
 {
  Timers timers(1);check(timers.add(first)&&!timers.add(first));check(timers.next_deadline()==4000);
  check(timers.advance(3999).empty()&&timers.size()==1);
  const auto due=timers.advance(4000);check(due.size()==1&&due[0]==first&&!timers.next_deadline());
  check(timers.advance(4000).empty()&&timers.advance(5000).empty());
 }
 {
  Timers timers(1);check(timers.add(first));check(!timers.cancel(b,2,2000)&&!timers.cancel(b,1,999));
  check(timers.cancel(b,1,3999)&&timers.advance(9000).empty());
 }
 {
  Timers timers(1);check(timers.add(first));check(!timers.cancel(b,1,4000));
  check(timers.advance(10000)==std::vector<Timers::Link>{first}); // Sparse ticks retain the true expiry time.
  auto future=first;future.effect=2;future.applied=10000;future.expires=13000;
  check(timers.add(future));check(timers.advance(9000).empty()&&timers.next_deadline()==13000);
 }
 {
  Timers timers(1);check(timers.add(first));auto replacement=first;
  replacement.caster=c;replacement.effect=2;replacement.applied=2000;replacement.expires=5000;replacement.rank=20;
  auto invalid=replacement;invalid.expires=5001;
  check(!timers.replace(b,1,invalid,2000)&&timers.next_deadline()==4000);
  check(timers.replace(b,1,replacement,2000)&&timers.advance(4000).empty());
  check(!timers.cancel(b,1,4500)&&timers.advance(5000)==std::vector<Timers::Link>{replacement});
 }
 {
  Timers timers(1);auto same_time=first;same_time.effect=2;
  check(timers.add(first)&&timers.replace(b,1,same_time,1000));check(timers.advance(4000)==std::vector<Timers::Link>{same_time});
 }
 {
  Timers timers(1);auto recycled=first;recycled.target.generation=2;recycled.effect=2;
  check(timers.add(first)&&timers.add(recycled));check(timers.cancel(b,1,2000));
  check(timers.advance(4000)==std::vector<Timers::Link>{recycled});
 }
 for(unsigned mode=0;mode<9;++mode){
  Timers timers(1);auto invalid=first;
  if(mode==0)invalid.target.instance=2;if(mode==1)invalid.caster.generation=0;if(mode==2)invalid.target.actor=65536;
  if(mode==3)invalid.level=256;if(mode==4)invalid.rank=21;if(mode==5)invalid.effect=0;
  if(mode==6)invalid.target=a;if(mode==7)invalid.expires=3999;
  if(mode==8){invalid.applied=UINT64_MAX-2;invalid.expires=UINT64_MAX;}
  check(!timers.add(invalid)&&timers.size()==0);
 }
 {
  Timers timers(1);auto later=first;later.target=c;later.effect=2;later.applied=1500;later.expires=4500;
  check(timers.add(first)&&timers.add(later));check(timers.advance(5000)==std::vector<Timers::Link>({first,later}));
  check(!timers.add(first));
 }
 {
  Timers timers(1);check(timers.add(first)&&timers.cancel(b,1,2000));
  check(!timers.add(first)); // A stale application cannot resurrect a cancelled obligation.
 }
 std::cout<<"PASS Lightning Strike rank policy and scoped natural-expiry timers,early removal,renewal,sparse chronology and generation reuse\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
