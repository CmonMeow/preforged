#include "gw_extinguish.h"
#include <bit>
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool x){if(!x)throw std::runtime_error("Extinguish conditional removal");}
int main()try{using namespace combat::conditions;using namespace skills::effects;
 for(unsigned rank:{0u,12u,15u,20u})for(unsigned divine:{0u,12u,20u})for(unsigned mask=0;mask<1024;++mask){
  State state;for(unsigned k=0;k<10;++k)if(mask&(1u<<k))check(state.apply(static_cast<Kind>(k),42,100,1000));
  const auto plan=*extinguish(rank,divine);check(plan.burning_heal==10+6*rank);
  for(bool caster:{false,true}){const auto result=extinguish_recipient(plan,state,caster,100);check(result&&result->removed==state.newest(100));
   check(result->healing==(result->removed==Kind::burning?plan.burning_heal:0)+(caster?plan.caster_heal:0));
   const auto expired=extinguish_recipient(plan,state,caster,1100);check(expired&&!expired->removed&&expired->healing==(caster?plan.caster_heal:0));}
  check(state.count(100)==unsigned(std::popcount(mask)));
 }
 State state;check(state.apply(Kind::burning,42,100,1000)&&state.apply(Kind::blind,42,101,2000));const auto plan=*extinguish(12,0);
 check(extinguish_recipient(plan,state,false,102)->healing==0);check(state.apply(Kind::burning,42,103,1));
 check(extinguish_recipient(plan,state,false,103)->healing==82);
 auto forged=plan;++forged.burning_heal;check(!extinguish_recipient(forged,state,false,103));check(!extinguish(21,0)&&!extinguish(0,21));
 std::cout<<"PASS Extinguish conditional Burning removal,all1024 condition sets/ranks,covered order,caster-only Divine Favor,expiry and immutable source;World binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
