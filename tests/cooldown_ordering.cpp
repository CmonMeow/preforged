#include "gw_cooldown_ordering.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Cooldown order line "+std::to_string(at.line()));}
int main()try{
 const Scope owner{1,2,3},stale{1,2,4};using R=OrderedAttackCooldowns::Result;
 CooldownOrderClock clock(owner);const auto first=clock.next(owner,100);check(first&&first->order==1&&!clock.next(stale,100));const auto delayed=clock.next(owner,99);check(delayed&&delayed->order==2&&delayed->at==99);check(clock.next(owner,100)->order==3);
 CooldownOrderClock exhausted(owner,UINT64_MAX-1);check(exhausted.next(owner,100)->order==UINT64_MAX&&!exhausted.next(owner,101));
 // A later World operation can carry an earlier sampled time, even after a
 // checkpoint. Causal order still wins; historical queries remain rejected.
 {OrderedAttackCooldowns timeline(owner);
  check(timeline.reset_attacks({owner,1,1000},{{400,0}})==R::accepted);
  check(timeline.put({owner,2,990},{400,0},6990)==R::accepted);
  check(!timeline.until(owner,{400,0},990)&&timeline.until(owner,{400,0},1000)==6990);
  check(timeline.checkpoint({owner,3,1000},{{{owner,2,990},{400,0},6990}}));
  check(timeline.reset_attacks({owner,4,995},{{400,0}})==R::accepted);
  check(timeline.until(owner,{400,0},1000)==0);
  check(timeline.put({owner,5,994},{400,0},6994)==R::accepted);
  check(timeline.until(owner,{400,0},1000)==6994);
 }
 std::vector<unsigned> order{1,2,3,4,5,6,7};unsigned permutations=0;
 do{OrderedAttackCooldowns timeline(owner);
 for(auto event:order){const CooldownStamp stamp{owner,event,event*10};
  const auto result=event==1?timeline.put(stamp,{402,0},1000):event==2?timeline.put(stamp,{403,9},1500):event==3?timeline.reset_attacks(stamp,{{402,0}}):event==4?timeline.reset_attacks(stamp,{{403,9}}):event==5?timeline.put(stamp,{402,0},2000):event==6?timeline.put(stamp,{281,0},3000):timeline.put(stamp,{403,10},2500);
  check(result==R::accepted);
 }
 check(timeline.until(owner,{402,0},80)==2000&&timeline.until(owner,{403,9},80)==0&&timeline.until(owner,{403,10},80)==2500&&timeline.until(owner,{281,0},80)==3000);
 check(timeline.reset_attacks({owner,4,40},{{403,9}})==R::duplicate);const auto before=timeline;
 check(timeline.reset_attacks({owner,4,40},{{402,0}})==R::invalid&&timeline==before);
 check(timeline.reset_attacks({stale,8,80},{{402,0}})==R::invalid&&timeline==before);
 check(timeline.reset_attacks({owner,8,80},{{281,0}})==R::invalid&&timeline==before);
 check(timeline.reset_attacks({owner,8,80},{{402,0},{402,0}})==R::invalid&&timeline==before);
 check(!timeline.until(owner,{402,0},69));
 check(timeline.acknowledge({owner,7,70})&&timeline.acknowledge({owner,7,70}));
 check(timeline.put({owner,2,20},{403,9},9999)==R::stale&&timeline.until(owner,{403,9},80)==0);
 ++permutations;
 }while(std::next_permutation(order.begin(),order.end()));check(permutations==5040);
 {OrderedAttackCooldowns timeline(owner);check(timeline.reset_attacks({owner,2,200},{{402,0}})==R::accepted);check(timeline.put({owner,1,100},{402,0},9000)==R::accepted);check(timeline.until(owner,{402,0},200)==0);check(timeline.put({owner,3,300},{402,0},10000)==R::accepted);check(timeline.until(owner,{402,0},300)==10000);
 check(timeline.put({owner,4,250},{403,0},9000)==R::accepted);const auto before=timeline;check(timeline.reset_attacks({owner,3,300},{{402,0}})==R::invalid&&timeline==before);}
 {OrderedAttackCooldowns timeline(owner);for(unsigned event=1;event<=4096;++event){check(timeline.put({owner,event,event},{402,0},10000)==R::accepted);
  if(event==1023)check(!timeline.needs_checkpoint());if(event==1024)check(timeline.needs_checkpoint());}
 const auto before=timeline;check(timeline.put({owner,4097,4097},{402,0},10000)==R::full&&timeline==before);check(timeline.acknowledge({owner,4096,4096}));check(!timeline.needs_checkpoint());check(timeline.put({owner,4097,4097},{402,0},10000)==R::accepted);}
 {OrderedAttackCooldowns timeline(owner);for(unsigned copy=0;copy<128;++copy)check(timeline.put({owner,copy+1,copy+1},{402,copy},1000)==R::accepted);
 const auto before=timeline;check(timeline.put({owner,129,129},{402,128},1000)==R::full&&timeline==before);
 check(timeline.reset_attacks({owner,129,1001},{{402,0}})==R::accepted&&timeline.acknowledge({owner,129,1001}));
 check(timeline.put({owner,130,1002},{402,128},2000)==R::accepted);}
 {
  OrderedAttackCooldowns timeline(owner);const CooldownCheckpointSource off_bar{{owner,1,100},{400,9},1000000000};
  check(timeline.put({owner,2,200},{402,0},2000)==R::accepted);
  check(timeline.reset_attacks({owner,3,300},{{402,0}})==R::accepted);
  check(timeline.checkpoint({owner,4,400},{off_bar})); // Seed a live off-bar copied timer not previously imported.
  check(timeline.put(off_bar.stamp,off_bar.slot,off_bar.until)==R::duplicate);
  check(timeline.put({owner,2,200},{402,0},2000)==R::stale&&timeline.until(owner,{402,0},400)==0);
  check(timeline.until(owner,off_bar.slot,400)==off_bar.until);
  check(timeline.reset_attacks({owner,5,500},{off_bar.slot})==R::accepted);
  check(timeline.put(off_bar.stamp,off_bar.slot,off_bar.until)==R::duplicate&&timeline.reset_supersedes(off_bar.stamp,off_bar.slot)==true);
  const auto before=timeline;check(!timeline.checkpoint({owner,6,600},{off_bar})&&timeline==before); // Cannot seed a reset source.
  check(timeline.put({owner,6,600},off_bar.slot,2000000000)==R::accepted);
  const CooldownCheckpointSource newer{{owner,6,600},off_bar.slot,2000000000};
  check(timeline.reset_supersedes(off_bar.stamp,off_bar.slot)==true);
  check(timeline.checkpoint({owner,7,700},{newer})&&timeline.checkpoint({owner,7,700},{newer}));
  check(timeline.put(off_bar.stamp,off_bar.slot,off_bar.until)==R::stale);
  check(timeline.put(newer.stamp,newer.slot,newer.until)==R::duplicate&&timeline.reset_supersedes(newer.stamp,newer.slot)==false);
  auto wrong=newer;++wrong.until;const auto saved=timeline;
  check(!timeline.checkpoint({owner,8,800},{wrong})&&timeline==saved);
  check(!timeline.checkpoint({owner,8,800},{newer,newer})&&timeline==saved);
  check(!timeline.checkpoint({stale,8,800},{newer})&&timeline==saved);
  check(!timeline.checkpoint({owner,8,699},{newer})&&timeline==saved);
 }
 {
  OrderedAttackCooldowns timeline(owner);const CooldownCheckpointSource persistent{{owner,1,1},{400,9},1000000000};
  check(timeline.checkpoint({owner,2,2},{persistent}));
  // Exceed both old lifetime capacities while preserving an old copied timer.
  for(std::uint64_t i=0;i<6000;++i){const auto order=3+i*2;
   check(timeline.put({owner,order,order},{402,unsigned(i+1000)},order+1)==R::accepted);
   check(timeline.checkpoint({owner,order+1,order+1},{persistent}));
   check(timeline.put(persistent.stamp,persistent.slot,persistent.until)==R::duplicate);
   check(timeline.until(owner,persistent.slot,order+1)==persistent.until);
  }
 }
 {
  OrderedAttackCooldowns timeline(owner);check(timeline.put({owner,1,100},{400,0},9000)==R::accepted);
  const auto before=timeline;
  check(!timeline.checkpoint({owner,2,200},{{{owner,1,100},{402,0},9000}})&&timeline==before);
  check(timeline.checkpoint({owner,2,200},{{{owner,1,100},{400,0},9000}}));
  check(timeline.put({owner,3,300},{400,0},12000)==R::accepted&&timeline.acknowledge({owner,3,300}));
  check(timeline.put({owner,3,300},{400,0},12000)==R::duplicate&&timeline.reset_supersedes({owner,3,300},{400,0})==false);
  check(timeline.put({owner,1,100},{400,0},9000)==R::stale);
  check(timeline.reset_attacks({owner,4,400},{{400,0}})==R::accepted);
  check(timeline.put({owner,3,300},{400,0},12000)==R::duplicate&&timeline.reset_supersedes({owner,3,300},{400,0})==true);
 }
 std::cout<<"PASS reordered delivery and safe complete-state checkpoints,off-bar/copy retention,old-source suppression and6000compaction cycles\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
