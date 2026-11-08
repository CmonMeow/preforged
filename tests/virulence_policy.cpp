#include "gw_virulence.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Virulence policy line "+std::to_string(at.line()));
}
int main()try{
 using combat::conditions::Kind;using combat::conditions::State;
 check(!virulence(21));
 for(unsigned rank:{0u,12u,20u}){
  const auto candidate=virulence(rank);check(bool(candidate));const auto plan=*candidate;check(plan.duration==(rank==0?3000:rank==12?13000:19000));
  State initial;auto empty=virulence_conditions(plan,initial,1,100);
  check(empty&&!empty->triggered&&empty->steps.empty());
  check(initial.apply(Kind::burning,2,100,50));
  auto result=virulence_conditions(plan,initial,1,149);check(result&&result->triggered&&result->steps.size()==3);
  const auto& steps=result->steps;
  check(steps[0].kind==Kind::disease&&steps[1].kind==Kind::weakness&&steps[2].kind==Kind::poison);
  for(unsigned i=0;i<3;++i){check(steps[i].state.count(149)==i+2);check(steps[i].state.newest(149)==steps[i].kind);
   check(steps[i].state.lookup(steps[i].kind)->expires==149+plan.duration&&steps[i].state.lookup(steps[i].kind)->cause==1);}
  check(initial.count(149)==1&&!initial.has(Kind::disease,149));
  check(steps.back().state.degeneration_pips(149)==-15&&steps.back().state.degeneration_pips(150)==-8);
  empty=virulence_conditions(plan,initial,1,150);check(empty&&!empty->triggered&&empty->steps.empty());
 }
 State initial;check(initial.apply(Kind::weakness,2,100,30000));
 const auto result=virulence_conditions(*virulence(0),initial,1,200);
 check(result&&result->steps.back().state.lookup(Kind::weakness)->expires==30100);
 check(result->steps.back().state.newest(200)==Kind::poison);
 auto forged=*virulence(12);++forged.duration;check(!virulence_conditions(forged,initial,1,200));
 check(!virulence_conditions(*virulence(12),initial,0,200));
 State late;check(late.apply(Kind::burning,2,UINT64_MAX-100,100));
 check(!virulence_conditions(*virulence(12),late,1,UINT64_MAX-50));
 check(late.count(UINT64_MAX-50)==1);
 std::cout<<"PASS Virulence prerequisite at exact expiry,rank durations,ordered covering and intermediate states,timer preservation,overflow and proposal isolation; World binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
