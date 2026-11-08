#include "gw_energy_regeneration.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Energy regeneration ceiling line "+std::to_string(at.line()));
}
int main()try{
 {vitals::Resources r{100,100,0,50};std::uint32_t debt=30;energy::Regeneration regen;
  auto step=regen.advance(r,30000,4,debt,-10);check(step.delta==20&&debt==10&&r.energy==0&&r.max_energy==50&&regen.remainder()==0);
  check(!regen.advance(r,3000,4,debt,-10).changed&&debt==10);
  check(regen.advance(r,3000,1,debt,-9).delta==1&&debt==9&&r.max_energy==50);
 }
 for(int initial=-12;initial<=12;++initial)for(int ceiling=-12;ceiling<=12;++ceiling)for(int pips:{0,1,4,10}){
  vitals::Resources dense{100,100,unsigned(std::max(0,initial)),20},sparse=dense;
  std::uint32_t dense_debt=unsigned(std::max(0,-initial)),sparse_debt=dense_debt;energy::Regeneration a,b;
  for(unsigned i=0;i<120;++i)a.advance(dense,100,pips,dense_debt,ceiling);
  b.advance(sparse,12000,pips,sparse_debt,ceiling);
  check(dense.energy==sparse.energy&&dense_debt==sparse_debt&&a.remainder()==b.remainder()&&dense.max_energy==20);
  check(std::int64_t(dense.energy)-dense_debt<=std::max(initial,ceiling));
 }
 {vitals::Resources r{100,100,0,20};std::uint32_t debt=1;energy::Regeneration regen;
  check(!regen.advance(r,1000,1,debt,0).changed&&regen.remainder()==1000);
  const auto prior=regen.remainder();bool rejected=false;try{regen.advance(r,1000,1,debt,21);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected&&regen.remainder()==prior&&debt==1&&r.energy==0);
  r.health=0;check(!regen.advance(r,3000,1,debt,0).changed&&debt==1&&regen.remainder()==0);
 }
 {vitals::Resources a{100,100,2,20},b=a;std::uint32_t da=0,db=0;energy::Regeneration x,y;
  for(unsigned dt:{100u,2700u,1200u,30000u}){x.advance(a,dt,4,da);y.advance(b,dt,4,db,20);}
  check(a.energy==b.energy&&da==db&&x.remainder()==y.remainder());
 }
 std::cout<<"PASS signed regeneration ceiling, debt recovery, fixed-ceiling tick cadence, invalid-state atomicity and nominal compatibility\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
