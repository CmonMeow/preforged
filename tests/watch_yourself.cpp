#include "gw_watch_yourself.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw::skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Watch Yourself line "+std::to_string(at.line()));}
int main()try{
 const Scope source{1,1,1},target{1,2,1};
 for(bool pvp:{false,true})for(unsigned rank=0;rank<=20;++rank){const auto plan=*watch_yourself(rank,pvp);auto owned=*prepare_watch_yourself(plan,source,target,1,100);Registry registry(target);
  check(registry.apply(target,{1,1,plan.skill,rank,Category::other,100,10100},Replace::same_skill).accepted);
  check(watch_yourself_armor(registry,owned,100)==interpolate(5,25,rank));
  std::uint64_t event=0;
  for(unsigned mode=0;mode<4;++mode){const auto next=consume_watch_yourself(registry,owned,{target,++event,100,mode!=1,mode!=2,(mode==0||mode==3)?0.:20.});check(next.has_value());
   check(!next->consumed&&next->owned.remaining==plan.charges);owned=next->owned;}
  for(unsigned n=0;n<plan.charges;++n){check(watch_yourself_armor(registry,owned,100)==plan.armor);
   const WatchYourselfDamage hit{target,++event,100,true,true,20};const auto next=consume_watch_yourself(registry,owned,hit);check(next&&next->consumed&&next->ended==(n+1==plan.charges));
   check(!consume_watch_yourself(registry,next->owned,hit));owned=next->owned;}
  check(!watch_yourself_armor(registry,owned,100));
  const auto fresh=*prepare_watch_yourself(plan,source,target,2,200);check(fresh.remaining==plan.charges&&fresh.last_damage==0);
  check(!watch_yourself_armor(registry,fresh,200));
 }
 const auto plan=*watch_yourself(12);auto owned=*prepare_watch_yourself(plan,source,target,1,100);Registry registry(target);check(registry.apply(target,{1,1,348,12,Category::other,100,10100},Replace::same_skill).accepted);
 check(!watch_yourself_armor(registry,owned,10100));auto wrong=target;++wrong.generation;check(!consume_watch_yourself(registry,owned,{wrong,1,100,true,true,20}));
 check(!consume_watch_yourself(registry,owned,{target,1,99,true,true,20}));check(!consume_watch_yourself(registry,owned,{target,1,100,true,true,-1}));
 auto forged=plan;++forged.charges;check(!prepare_watch_yourself(forged,source,target,1,100));check(!watch_yourself(21));
 check(!prepare_watch_yourself(plan,source,{2,2,1},1,100));check(!prepare_watch_yourself(plan,source,target,1,UINT64_MAX));
 auto corrupt=owned;corrupt.source.instance=2;check(!watch_yourself_armor(registry,corrupt,100));
 corrupt=owned;corrupt.remaining=11;check(!watch_yourself_armor(registry,corrupt,100));
 corrupt=owned;++corrupt.ends;check(!watch_yourself_armor(registry,corrupt,100));
 check(!consume_watch_yourself(registry,owned,{target,1,100,true,true,std::numeric_limits<double>::infinity()}));
 check(registry.remove(target,1).has_value()&&!watch_yourself_armor(registry,owned,100));
 std::cout<<"PASS Watch Yourself42rank/mode plans,documented damage counter,final-hit armor,expiry/refresh,receipt deduplication and scope guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

