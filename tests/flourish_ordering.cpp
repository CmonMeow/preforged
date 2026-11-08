#include "gw_flourish.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw::skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Flourish ordering line "+std::to_string(at.line()));}
int main()try{
 const Scope owner{1,2,3};using R=OrderedAttackCooldowns::Result;
 const FlourishCooldowns snapshot{owner,1,0,200,true,{{{437,0},9000,0},{{390,1},8000,0},{{362,0},7000,0}}};
 const auto proposal=*prepare_flourish_recharge(snapshot,*flourish(12),1);check(proposal.energy==12&&proposal.barriers.size()==2);
 std::vector<unsigned> order{1,2,3,4,5};unsigned permutations=0;
 do{OrderedAttackCooldowns timeline(owner);
  for(unsigned e:order){const auto result=e==1?timeline.put({owner,1,100},{437,0},9000):e==2?timeline.put({owner,2,150},{390,1},8000):e==3?deliver_flourish_recharge(timeline,proposal,{owner,3,200}):e==4?timeline.put({owner,4,190},{437,0},10000):timeline.put({owner,5,250},{362,0},7000);check(result==R::accepted);}
  check(timeline.until(owner,{437,0},300)==10000&&timeline.until(owner,{390,1},300)==0&&timeline.until(owner,{362,0},300)==7000);
  check(deliver_flourish_recharge(timeline,proposal,{owner,3,200})==R::duplicate);
  const auto before=timeline;check(timeline.reset_attacks({owner,6,300},{{437,0}})==R::invalid&&timeline==before);
  check(timeline.reset_attack_skills({owner,6,300},{{362,0}})==R::invalid&&timeline==before);
  auto forged=proposal;forged.energy++;check(deliver_flourish_recharge(timeline,forged,{owner,6,200})==R::invalid&&timeline==before);
  check(deliver_flourish_recharge(timeline,proposal,{{1,2,4},6,200})==R::invalid&&timeline==before);
  ++permutations;
 }while(std::next_permutation(order.begin(),order.end()));check(permutations==120);
 {OrderedAttackCooldowns timeline(owner);check(deliver_flourish_recharge(timeline,proposal,{owner,2,200})==R::accepted);
  check(timeline.put({owner,1,100},{437,0},9000)==R::accepted&&timeline.until(owner,{437,0},200)==0);
  check(timeline.put({owner,3,250},{437,0},9500)==R::accepted);
  const std::vector<CooldownCheckpointSource> active{{{owner,3,250},{437,0},9500}};
  const auto before=timeline;check(!timeline.checkpoint({owner,4,300},active)&&timeline==before);
  check(timeline.checkpoint_attack_skills({owner,4,300},active)&&timeline.until(owner,{437,0},300)==9500);
  check(timeline.put({owner,3,250},{437,0},9500)==R::duplicate);
  check(deliver_flourish_recharge(timeline,proposal,{owner,2,200})==R::stale);
  check(timeline.reset_attack_skills({owner,5,290},{{437,0}})==R::accepted&&timeline.until(owner,{437,0},300)==0);
 }
 std::cout<<"PASS Flourish120 causal delivery orders,pet/copied barriers,later timers,forged/stale/replay and pet checkpoint;legacy14/type16 exclusions preserved;World Energy binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
