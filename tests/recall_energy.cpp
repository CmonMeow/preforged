#include "gw_recall_energy.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Recall Energy transaction");}
int main()try{
 const Scope owner{1,42,1};energy::Regeneration regen;vitals::Resources initial{500,500,70,100};RecallReward reward{owner,1,10,20100};
 const auto normal=prepare_recall_energy(reward,owner,initial,regen,0,100,2);check(normal&&normal->resources.energy==93&&normal->restored==10&&normal->cursor==20100&&initial.energy==70&&regen.remainder()==0);
 check(!prepare_recall_energy(reward,owner,initial,regen,0,20101,2));
 auto near_full=initial;near_full.energy=99;const auto capped=prepare_recall_energy(reward,owner,near_full,regen,0,100,2);check(capped&&capped->resources.energy==100&&!capped->restored);
 auto empty=initial;empty.energy=0;empty.max_energy=20;reward.energy=22;
 const auto debt=prepare_recall_energy(reward,owner,empty,regen,20,100,2);check(debt&&debt->before_reward.delta==13&&debt->repaid==7&&debt->restored==15&&!debt->debt&&debt->resources.energy==15);
 reward.at=100;const auto full_debt=prepare_recall_energy(reward,owner,empty,regen,30,100,2);check(full_debt&&full_debt->repaid==22&&full_debt->debt==8&&!full_debt->restored);
 const auto negative=prepare_recall_energy(reward,owner,empty,regen,30,100,2,-10);
 check(negative&&negative->repaid==20&&negative->debt==10&&!negative->restored&&negative->resources.max_energy==20&&negative->regeneration.remainder()==0);
 check(!prepare_recall_energy(reward,owner,empty,regen,30,100,2,21));
 auto stale=owner;++stale.generation;check(!prepare_recall_energy(reward,stale,initial,regen,0,100,2));
 auto dead=initial;dead.health=0;check(!prepare_recall_energy(reward,owner,dead,regen,0,100,2));
 check(!prepare_recall_energy(reward,owner,initial,regen,1,100,2));check(!prepare_recall_energy(reward,owner,initial,regen,0,100,-1));
 auto fraction=initial;fraction.energy=99;energy::Regeneration remainder;remainder.advance(fraction,1000,1);check(remainder.remainder()==1000);
 const auto filled=prepare_recall_energy(reward,owner,fraction,remainder,0,100,0);check(filled&&filled->resources.energy==100&&filled->regeneration.remainder()==0&&remainder.remainder()==1000);
 overcast::CeilingCurve curve(owner,100,33);check(curve.apply(owner,10,1,100));
 vitals::Resources burdened{500,500,20,30};reward={owner,1,22,601};
 const auto exact=prepare_recall_energy(reward,owner,burdened,regen,0,curve,0);
 check(exact&&exact->gained_units==49599&&overcast::precise_current(exact->state)==6049599&&curve.cursor()==100);
 auto wrong_reward=reward;wrong_reward.owner=stale;check(!prepare_recall_energy(wrong_reward,owner,burdened,regen,0,curve,0));
 wrong_reward=reward;wrong_reward.effect=0;check(!prepare_recall_energy(wrong_reward,owner,burdened,regen,0,curve,0));
 wrong_reward=reward;wrong_reward.energy=31;check(!prepare_recall_energy(wrong_reward,owner,burdened,regen,0,curve,0));
 wrong_reward=reward;wrong_reward.at=99;check(!prepare_recall_energy(wrong_reward,owner,burdened,regen,0,curve,0));
 check(!prepare_recall_energy(reward,stale,burdened,regen,0,curve,0));
 burdened.health=0;check(!prepare_recall_energy(reward,owner,burdened,regen,0,curve,0));
 std::cout<<"PASS Recall ordered regeneration/debt/capacity settlement, immutable original, full-cap remainder and late-cursor/dead/stale rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
