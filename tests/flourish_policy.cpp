#include "gw_flourish.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b){if(!b)throw std::runtime_error("Flourish policy");}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto plan=flourish(rank);check(plan&&plan->energy_per_recharge==interpolate(2,7,rank));}check(!flourish(21));
 check(flourish_attack_skill(390)&&flourish_attack_skill(437)&&!flourish_attack_skill(362)&&!flourish_attack_skill(389)&&!flourish_attack_skill(281));
 FlourishCooldowns state{{1,2,3},1,0,1000,true,{{{390,0},1001,0},{{390,1},0,2000},{{437,0},3000,4000},{{340,0},0,0},{{281,0},3000,0},{{389,0},9000,0},{{399,0},1000,1000},{{362,0},5000,0}}};
 const auto plan=*flourish(12);const auto p=prepare_flourish_recharge(state,plan,1);check(p&&p->cleared.size()==3&&p->barriers.size()==5&&p->energy==18);
 check(p->after.slots[4]==state.slots[4]&&p->after.slots[5]==state.slots[5]&&p->after.slots[7]==state.slots[7]);
 auto forged=*p;forged.energy=100;auto current=state;check(!commit_flourish_cooldowns(current,forged)&&current==state);
 check(commit_flourish_cooldowns(current,*p)&&!commit_flourish_cooldowns(current,*p));
 const auto ready=prepare_flourish_recharge(current,plan,2);check(ready&&ready->energy==0&&ready->cleared.empty()&&ready->barriers.size()==5);
 auto changed=state;changed.slots[0].recharge_until=4000;check(!commit_flourish_cooldowns(changed,*p));
 changed=state;changed.owner.generation++;check(!commit_flourish_cooldowns(changed,*p));
 changed=state;changed.timers_known=false;check(!prepare_flourish_recharge(changed,plan,1));
 changed=state;changed.slots.back()=changed.slots.front();check(!prepare_flourish_recharge(changed,plan,1));
 changed=state;changed.slots[0].slot.copy=65536;check(!prepare_flourish_recharge(changed,plan,1));
 changed=state;changed.revision=UINT64_MAX;check(!prepare_flourish_recharge(changed,plan,1));
 std::cout<<"PASS staged Flourish native rank/cost/timing,attack+pet/copy slots,only future timers reward,ready/adrenaline distinction,barriers,stale/replay/forged/unknown timer guards;World/Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
