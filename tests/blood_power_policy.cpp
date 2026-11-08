#include "gw_blood_is_power.h"
#include "gw_health_sacrifice.h"
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw::skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Blood is Power policy line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto plan=effects::blood_is_power(rank);check(plan&&plan->rank==rank&&plan->duration==10000&&plan->range==1248);}
 check(!effects::blood_is_power(21));
 check(effects::blood_is_power(0)->pips==3&&effects::blood_is_power(3)->pips==4&&effects::blood_is_power(8)->pips==5&&effects::blood_is_power(13)->pips==6&&effects::blood_is_power(18)->pips==7);
 for(const auto [maximum,expected]:std::vector<std::pair<unsigned,unsigned>>{{1,0},{2,1},{49,16},{50,16},{51,17},{149,49},{150,50},{151,50},{480,158},{1000000,330000}}){
  const auto loss=sacrifice::resolve(119,maximum,maximum);check(loss&&loss->requested==expected&&loss->lost==expected&&loss->remaining==maximum-expected&&!loss->killed);
 }
 const auto lethal=sacrifice::resolve(119,10,480);check(lethal&&lethal->requested==158&&lethal->lost==11&&lethal->killed);
 const auto fractional=sacrifice::resolve(119,.25,1);check(fractional&&!fractional->lost&&fractional->remaining==.25&&!fractional->killed);
 const auto other=sacrifice::resolve(118,51,51);check(other&&other->requested==5.1);
 std::cout<<"PASS Blood is Power native plan,ranks,half-even sacrifice,1HP zero cost,lethal clamp and other-skill isolation; Energy effect not bound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
