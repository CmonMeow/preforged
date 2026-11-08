#include "gw_energy_regeneration.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Energy gain line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned nominal=0;nominal<=20;++nominal)for(int signed_current=-20;signed_current<=int(nominal);++signed_current)
 for(int ceiling=-20;ceiling<=int(nominal);++ceiling)for(unsigned amount:{0u,1u,7u,40u,UINT32_MAX}){
  vitals::Resources r{100,100,unsigned(std::max(0,signed_current)),nominal};std::uint32_t debt=unsigned(std::max(0,-signed_current));
  const auto result=energy::gain(r,debt,amount,ceiling);const auto after=std::int64_t(r.energy)-debt;
  check(after>=signed_current&&after<=std::max(signed_current,ceiling));
  check(after-signed_current==result.total()&&result.total()<=amount&&!(debt&&r.energy));
  check(result.repaid<=unsigned(std::max(0,-signed_current))&&r.energy<=nominal);
  if(signed_current>=ceiling)check(after==signed_current&&result.total()==0);
 }
 vitals::Resources r{100,100,0,30};std::uint32_t debt=25;
 auto g=energy::gain(r,debt,100,-10);check(g.repaid==15&&g.restored==0&&debt==10&&r.energy==0);
 g=energy::gain(r,debt,100,-10);check(g.total()==0&&debt==10);
 g=energy::gain(r,debt,15,5);check(g.repaid==10&&g.restored==5&&debt==0&&r.energy==5);
 g=energy::gain(r,debt,100);check(g.restored==25&&r.energy==30);
 for(unsigned mode=0;mode<5;++mode){auto bad=r;auto bad_debt=debt;std::int64_t ceiling=30;
  if(mode==0)bad.energy=31;if(mode==1)bad_debt=1;if(mode==2)bad.max_energy=1000001;if(mode==3)ceiling=31;if(mode==4)ceiling=-1000001;
  const auto before=bad;const auto prior=bad_debt;bool rejected=false;
  try{energy::gain(bad,bad_debt,1,ceiling);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected&&bad.energy==before.energy&&bad.max_energy==before.max_energy&&bad_debt==prior);
 }
 std::cout<<"PASS signed Energy gain ceilings, debt repayment, nominal compatibility and invalid-input atomicity\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
