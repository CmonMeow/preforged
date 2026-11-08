#include "gw_martyr.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Martyr line "+std::to_string(at.line()));}
int main()try{
 const auto plan=*martyr();const Scope caster{1,1,1};combat::conditions::State own;
 check(own.apply(Kind::burning,9,100,10000));
 std::vector<MartyrAlly> allies;
 for(unsigned id=2;id<=5;++id){MartyrAlly a{{1,id,1},id==5?5001.f:5000.f,true,id!=4,{}};
  for(unsigned kind=0;kind<10;++kind)check(a.conditions.apply(Kind(kind),id,100,1000*id+kind));allies.push_back(a);}
 const auto next=prepare_martyr(plan,caster,own,allies,200);check(next&&next->allies.size()==2);
 for(unsigned kind=0;kind<10;++kind){const auto k=Kind(kind);check(next->conditions.has(k,200));
  check(next->conditions.lookup(k)->expires==(k==Kind::burning?10100:3100+kind));}
 for(const auto& ally:next->allies){check(ally.removed.size()==10&&!ally.conditions.newest(200));}
 check(allies.front().conditions.newest(200)&&!own.has(Kind::poison,200));
 std::reverse(allies.begin(),allies.end());const auto reverse=prepare_martyr(plan,caster,own,allies,200);check(reverse&&reverse->allies.front().scope.actor==2);
 allies[0].alive=false;check(prepare_martyr(plan,caster,own,allies,200).has_value());
 auto bad=allies;bad.push_back(bad.front());check(!prepare_martyr(plan,caster,own,bad,200));
 bad=allies;bad[0].scope.instance=2;check(!prepare_martyr(plan,caster,own,bad,200));
 bad=allies;bad[0].distance=std::numeric_limits<float>::infinity();check(!prepare_martyr(plan,caster,own,bad,200));
 auto forged=plan;forged.range=6000;check(!prepare_martyr(forged,caster,own,allies,200));
 const auto expired=prepare_martyr(plan,caster,own,allies,20000);check(expired&&!expired->conditions.newest(20000));
 std::cout<<"PASS Martyr copied all-condition/remaining-duration transfer,range/allegiance/identity guards and deterministic no-partial transaction\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

