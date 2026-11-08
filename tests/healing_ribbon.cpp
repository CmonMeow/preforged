#include "gw_healing_ribbon.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace fw::gw::skills::effects;
static void check(bool yes){if(!yes)throw std::runtime_error("Healing Ribbon plan");}

int main()try{
    check(healing_ribbon(0)->primary_heal==20);
    check(healing_ribbon(0)->secondary_heal==10);
    check(healing_ribbon(10)->primary_heal==80);
    check(healing_ribbon(10)->secondary_heal==70);
    check(healing_ribbon(15)->primary_heal==110);
    check(healing_ribbon(15)->secondary_heal==100);
    check(!healing_ribbon(21));
    const auto plan=*healing_ribbon(10);
    check(plan.near==252);
    const Scope caster{1,1,1},primary{1,2,1},near{1,3,1},far{1,4,1};
    std::vector<HealingRibbonCandidate> candidates{
        {primary,0,true,true},{far,321,true,true},{near,20,true,true},
        {caster,30,true,true},{near,20,true,true},
        {{1,5,1},5,true,false},{{1,6,1},10,false,true},
        {{2,7,1},10,true,true},{{1,8,1},323,true,true}};
    const auto result=healing_ribbon_recipients(plan,caster,primary,candidates);
    check(result&&result->size()==3);
    check((*result)[0]==HealingRibbonRecipient{primary,80,true});
    check((*result)[1]==HealingRibbonRecipient{near,70,false});
    check((*result)[2]==HealingRibbonRecipient{caster,70,false});
    check(!healing_ribbon_recipients(plan,caster,caster,candidates));
    auto forged=plan;++forged.primary_heal;
    check(!healing_ribbon_recipients(forged,caster,primary,candidates));
    const auto boundary=healing_ribbon_recipients(plan,caster,primary,
        {{primary,0,true,true},{near,252,true,true},{far,253,true,true}});
    check(boundary&&boundary->size()==2&&(*boundary)[1].scope==near);
    candidates[0].alive=false;
    check(!healing_ribbon_recipients(plan,caster,primary,candidates));
    candidates[0].alive=true;
    candidates.push_back({{1,9,1},std::numeric_limits<float>::quiet_NaN(),true,true});
    check(healing_ribbon_recipients(plan,caster,primary,candidates)->size()==3);
    std::cout<<"PASS Healing Ribbon pinned plan and 3-recipient selection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
