#include "gw_tainted_flesh.h"
#include <iostream>
#include <source_location>
#include <string>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Tainted Flesh policy line "+std::to_string(at.line()));
}
int main()try{
 check(!tainted_flesh(21));
 for(unsigned rank=0;rank<=20;++rank){const auto plan=tainted_flesh(rank);check(plan&&plan->rank==rank&&plan->range==1248);}
 check(tainted_flesh(0)->duration==20000&&tainted_flesh(0)->disease_duration==3000);
 check(tainted_flesh(12)->duration==39000&&tainted_flesh(12)->disease_duration==13000);
 check(tainted_flesh(20)->duration==52000&&tainted_flesh(20)->disease_duration==19000);
 const auto plan=*tainted_flesh(12);combat::conditions::State before;
 check(before.apply(Kind::bleeding,9,100,20000));
 auto result=tainted_flesh_hit(plan,before,1,1000,true,true,true,false);
 check(result.contact==TaintedContact::infected&&result.state.has(Kind::bleeding,1000));
 check(result.state.has(Kind::disease,13999)&&!result.state.has(Kind::disease,14000));
 check(result.state.lookup(Kind::disease)->cause==1&&result.state.newest(1000)==Kind::disease);
 for(unsigned mode=0;mode<6;++mode){
  auto r=tainted_flesh_hit(plan,before,1,1000,mode!=0,mode!=1,
   mode==4?std::nullopt:std::optional(mode!=2),mode==5?std::nullopt:std::optional(mode==3));
  check(r.contact==(mode<4?TaintedContact::no_effect:TaintedContact::unknown)&&!r.state.has(Kind::disease,1000));
 }
 check(tainted_flesh_hit(plan,before,1,1000,true,true,false,std::nullopt).contact==TaintedContact::no_effect);
 check(tainted_flesh_hit(plan,before,1,1000,true,true,std::nullopt,true).contact==TaintedContact::no_effect);
 check(tainted_flesh_hit(plan,before,0,1000,true,true,true,false).contact==TaintedContact::invalid);
 auto forged=plan;++forged.disease_duration;check(tainted_flesh_hit(forged,before,1,1000,true,true,true,false).contact==TaintedContact::invalid);
 check(tainted_flesh_hit(plan,before,1,UINT64_MAX,true,true,true,false).contact==TaintedContact::invalid);
 auto longer=before;check(longer.apply(Kind::disease,2,100,30000));
 result=tainted_flesh_hit(plan,longer,1,1000,true,true,true,false);
 check(result.contact==TaintedContact::infected&&result.state.lookup(Kind::disease)->expires==30100&&result.state.lookup(Kind::disease)->cause==2);
 std::cout<<"PASS Tainted Flesh native ranks and resolved-melee Disease proposal;World/Runtime and rendered acceptance are separate checks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
