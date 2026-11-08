#include "gw_diversion.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Diversion policy regression");}
int main()try{
 check(diversion(0)->additional_recharge==10000&&diversion(12)->additional_recharge==47000&&diversion(20)->additional_recharge==71000&&!diversion(21));
 const Scope owner{1,2,1};Registry original(owner);
 check(original.apply(owner,{7,3,30,12,Category::hex,100,6100},Replace::same_skill).accepted);
 for(unsigned skill:{28u,67u,22u,1049u}){
  const auto result=diversion_use(original,{owner,8,{skill,1},true,false},200);
  check(result&&result->slot.skill==skill&&result->slot.copy==1&&result->consumed.instance==7&&result->event==8);
  check(result->additional_recharge==((skill==22||skill==1049)?0u:47000u));
  check(!result->registry.lookup(owner,7)&&original.lookup(owner,7));
 }
 check(diversion_use(original,{owner,8,{28,0},true,true},200)->additional_recharge==0);
 check(!diversion_use(original,{owner,8,{28,0},false,false},200));
 check(!diversion_use(original,{{1,2,2},8,{28,0},true,false},200));
 check(!diversion_use(original,{owner,0,{28,0},true,false},200));
 check(!diversion_use(original,{owner,8,{28,0},true,false},6100));
 check(diversion_recharge_deadline(200,20000,47000,0)==67200);
 check(diversion_recharge_deadline(200,20000,47000,90000)==90000);
 check(!diversion_recharge_deadline(UINT64_MAX-100,20000,47000,0));
 std::cout<<"PASS Diversion rank scale, proposed skill-use consumption, instant-reset/Inspired/Revealed exceptions and nonshortening deadlines; World/casting pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
