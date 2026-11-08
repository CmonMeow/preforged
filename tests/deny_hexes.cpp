#include "gw_deny_hexes.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::skills;
using namespace fw::gw::skills::effects;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Deny Hexes line "+std::to_string(at.line()));
}
int main()try{
    const auto plan=deny_hexes();check(plan&&plan->energy==5&&plan->recharge==12);
    const Scope source{1,1,1},target{1,2,1};Registry effects(target);
    unsigned i=0;for(unsigned skill:{99u,100u,101u,103u})
        check(effects.apply(target,{++i,3,skill,0,Category::hex,100+i,1000},Replace::same_skill).accepted);
    Slots bar{};bar[0]={991,0};bar[1]={279,0};bar[2]={300,0};bar[3]={194,0};
    std::array<Time,8> recharges{};
    const auto one=prepare_deny_hexes(*plan,source,target,effects,bar,recharges,200);
    check(one&&one->removed.size()==1&&one->removed[0].instance==4);
    recharges[0]=12000;
    const auto two=prepare_deny_hexes(*plan,source,target,effects,bar,recharges,200);
    check(two&&two->recharging_divine_favor==1&&two->removed.size()==2&&two->removed[1].instance==3);
    recharges[1]=12000;recharges[2]=12000;recharges[3]=12000;
    const auto four=prepare_deny_hexes(*plan,source,target,effects,bar,recharges,200);
    check(four&&four->recharging_divine_favor==3&&four->removed.size()==4&&four->removed[3].instance==1);
    check(effects.size()==4); // Selection never mutates authoritative effects.
    recharges[0]=200; // A completed cooldown does not count.
    const auto three=prepare_deny_hexes(*plan,source,target,effects,bar,recharges,200);
    check(three&&three->recharging_divine_favor==2&&three->removed.size()==3);
    const std::map<std::pair<unsigned,unsigned>,Time> committed{{{991,0},12000},{{279,0},12000},{{194,0},12000}};
    const auto authority=deny_hexes_committed_recharges(bar,bar,{991,0},committed,200);
    check(authority&&(*authority)[0]==12000&&(*authority)[1]==12000&&(*authority)[2]==0&&(*authority)[3]==12000);
    const auto sourced=prepare_deny_hexes(*plan,source,target,effects,bar,*authority,200);
    check(sourced&&sourced->recharging_divine_favor==2&&sourced->removed.size()==3);
    auto changed=bar;changed[1]={303,0};
    check(!deny_hexes_committed_recharges(bar,changed,{991,0},committed,200));
    check(!deny_hexes_committed_recharges(bar,bar,{991,0},{{{279,0},12000}},200));
    check(!deny_hexes_committed_recharges(bar,bar,{991,0},committed,12000));
    auto forged=*plan;forged.recharge=1;
    check(!prepare_deny_hexes(forged,source,target,effects,bar,recharges,200));
    check(!prepare_deny_hexes(*plan,source,{2,2,1},effects,bar,recharges,200));
    check(!prepare_deny_hexes(*plan,{1,1,0},target,effects,bar,recharges,200));
    std::cout<<"PASS Deny Hexes pinned row, slot cooldown count, newest-first selection and scope guards\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
