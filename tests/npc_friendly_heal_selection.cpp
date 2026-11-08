#include "gw_npc_friendly_heal_selection.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool value){if(!value)throw std::runtime_error("authored friendly healing selection failed");}
int main()try{
    namespace f=combat::ai::friendly;
    f::HealProfile profile{286,0,0,"synthetic explicit rank profile"};
    f::Healer source{{1,36,1},{100,100},40,1000,1000,true};
    f::HealPolicy policy{800,f::HealPriority::declared_order,true,0,"synthetic authored80percent hold-position order"};
    std::vector<f::Recipient> candidates{
        {{1,35,1},{60,100},1000,100,true,true,true,true},
        {{1,37,1},{20,100},1000,100,true,true,true,true}};
    check(f::select(policy,profile,source,candidates,1000,false,true)->target==candidates[0].scope);
    policy.priority=f::HealPriority::lowest_fraction;
    check(f::select(policy,profile,source,candidates,1000,false,true)->target==candidates[1].scope);
    check(!f::select(policy,profile,source,candidates,1000,true,true));
    check(!f::select(policy,profile,source,candidates,1000,false,false));
    candidates[1].health_clock=2000;
    check(f::select(policy,profile,source,candidates,1000,false,true)->target==candidates[0].scope);
    candidates[0].distance=1400;check(!f::select(policy,profile,source,candidates,1000,false,true));
    policy.hold_position=false;policy.maximum_follow=1500;
    check(f::select(policy,profile,source,candidates,1000,false,true)->kind==f::HealDecisionKind::approach);
    candidates[0].distance=1600;check(!f::select(policy,profile,source,candidates,1000,false,true));
    candidates[0].distance=100;candidates[0].health.current=80;
    check(!f::select(policy,profile,source,candidates,1000,false,true)); //strict threshold, no invented weight
    std::cout<<"Authored friendly heal threshold, priority, activity and hold-position selection passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
