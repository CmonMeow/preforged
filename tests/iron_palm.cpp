#include "gw_iron_palm.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw::skills::effects;
static void check(bool yes){if(!yes)throw std::runtime_error("Iron Palm plan/effect");}
int main()try{
    check(iron_palm(0)->damage==5);
    check(iron_palm(12)->damage==41);
    check(iron_palm(15)->damage==50);
    check(!iron_palm(21));
    const auto plan=*iron_palm(12);
    fw::gw::combat::Actor target;
    target.id=30;target.generation=1;target.health=fw::gw::combat::Health{500,500};
    target.damageable=true;
    fw::gw::combat::conditions::State conditions;
    const auto plain=iron_palm_target(plan,target,conditions,false,20,1000);
    check(plain&&plain->hit.applied==41&&!plain->knockdown_requested);
    check(plain->target.health->current==459);
    check(!iron_palm_target(plan,target,conditions,std::nullopt,20,1000));
    const auto hexed=iron_palm_target(plan,target,conditions,true,20,1000);
    check(hexed&&hexed->hit.applied==41&&hexed->knockdown_requested);
    check(conditions.apply(fw::gw::combat::conditions::Kind::poison,480,900,200));
    check(iron_palm_target(plan,target,conditions,false,20,1000)->knockdown_requested);
    check(!iron_palm_target(plan,target,conditions,false,20,1100)->knockdown_requested);
    auto forged=plan;forged.knockdown_ms=1000;
    check(!iron_palm_target(forged,target,conditions,true,20,1000));
    target.health=fw::gw::combat::Health{1,500};
    const auto lethal=iron_palm_target(plan,target,conditions,true,20,1000);
    check(lethal&&lethal->hit.killed&&!lethal->knockdown_requested);
    target.damageable=false;
    check(!iron_palm_target(plan,target,conditions,true,20,1000));
    std::cout<<"PASS Iron Palm rank damage and completion-time status gate; World binding pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
