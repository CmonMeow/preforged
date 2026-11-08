#include "gw_dagger_dual_group_detached.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::weapon;
using namespace fw::gw::skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("dual group draft");}
static ResolvedAttack terminal(Scope source,Scope target,std::uint64_t event,AttackOutcome outcome){
    return {source,target,event,100,100,outcome,outcome==AttackOutcome::hit?std::optional<std::uint64_t>{100}:std::nullopt,AttackDelivery::melee};
}
int main()try{
    const Scope source{1,25,1},target{1,30,2};DaggerChain chain;chain.lead(target,10);chain.offhand(target,20);
    for(unsigned skill:{775u,776u,777u})for(auto first:{AttackOutcome::hit,AttackOutcome::miss,AttackOutcome::block,AttackOutcome::replaced})
    for(auto second:{AttackOutcome::hit,AttackOutcome::miss,AttackOutcome::block,AttackOutcome::replaced}){
        auto group=*DaggerDualGroupDraft::begin(source,target,chain,skill,1,100);
        check(!group.select_second());check(!group.release(source,target,skill,0,100));
        const auto release=group.release(source,target,skill,10,100);check(release&&!*release);
        check(!group.release(source,target,skill,11,100));
        const auto a=terminal(source,target,10,first);check(group.finish(a)&&!group.finish(a));
        check(group.select_second());check(!group.release(source,target,skill,10,100));
        const auto next=group.release(source,target,skill,11,100);check(next&&*next);
        auto foreign=terminal(source,Scope{1,30,3},11,second);check(!group.finish(foreign));
        check(group.finish(terminal(source,target,11,second))&&group.complete());
        check(group.admitted_chain().stage==2&&group.admitted_chain().target==target);
    }
    check(!DaggerDualGroupDraft::begin(source,target,chain,775,0,100));
    check(!DaggerDualGroupDraft::begin(source,target,chain,780,1,100));
    check(!DaggerDualGroupDraft::begin(source,target,chain,775,1,15020));
    auto group=*DaggerDualGroupDraft::begin(source,target,chain,775,1,100);
    check(bool(group.release(source,target,775,1,100)));
    auto false_hit=terminal(source,target,1,AttackOutcome::hit);false_hit.before_damage.reset();check(!group.finish(false_hit));
    auto false_block=terminal(source,target,1,AttackOutcome::block);false_block.before_damage=100;check(!group.finish(false_block));
    check(group.finish(terminal(source,target,1,AttackOutcome::cancelled)));group.stop();check(!group.select_second());
    check(!group.complete()&&!group.owns_event(0)&&group.owns_event(1));
    std::cout<<"PASS detached owned dual group distinct real-event terminals and retained stage2; World driver pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
