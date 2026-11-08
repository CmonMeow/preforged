#include "gw_signet_humility.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok){if(!ok)throw std::runtime_error("Humility paired disable regression");}
int main()try{
 const auto plan=*signet_humility(12);check(plan.target_disable==13000&&plan.self_disable==10000&&signet_humility(20)->target_disable==21000&&!signet_humility(21));
 Slots caster{};caster[0]={62,0};caster[1]={281,0};caster[2]={3,0};caster[3]={5,0};
 Slots target{};target[0]={5,0};target[1]={259,0};target[2]={259,1};target[3]={257,0};
 DisableDeadlines current_caster{{{14,0},60000}},current_target{{{5,0},50000},{{259,0},1000}};
 const auto result=humility_deadlines(plan,current_caster,current_target,caster,target,2000);check(bool(result));
 check(result->caster.size()==3&&result->caster.at({14,0})==60000&&result->caster.at({281,0})==12000&&result->caster.at({3,0})==12000);
 check(!result->caster.contains({62,0})&&!result->caster.contains({5,0}));
 check(result->target.size()==3&&result->target.at({5,0})==50000&&result->target.at({259,0})==15000&&result->target.at({259,1})==15000&&!result->target.contains({257,0}));
 check(current_caster.size()==1&&current_target.at({259,0})==1000);
 auto invalid=target;invalid[7]={99999,0};check(!humility_deadlines(plan,current_caster,current_target,caster,invalid,2000));
 check(!humility_deadlines(plan,current_caster,current_target,caster,target,UINT64_MAX));
 auto forged=plan;forged.self_disable=0;check(!humility_deadlines(forged,current_caster,current_target,caster,target,2000));
 std::cout<<"PASS Humility paired elite/all-copy and non-Mesmer/common-skill disable selection,longer deadline preservation and atomic rejection;World/casting/retail interactions pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
