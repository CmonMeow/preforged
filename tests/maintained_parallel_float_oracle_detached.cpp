// Detached negative-control: sparse endpoint paths can hide float-only range exits.
#include "gw_maintained_range.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
int main()try{
 const std::array<MaintainedRangePoint,2> a{{{0,0,0},{1000,100,0}}};
 const std::array<MaintainedRangePoint,2> b{{{0,5000,0},{1000,5100,0}}};
 const auto sparse=maintained_range_exit(a,b,0,1000);if(!sparse||sparse->first_outside)throw std::runtime_error("unexpected sparse result");
 std::optional<std::uint64_t> first;
 for(std::uint64_t at=0;at<=1000;++at){const float x=float(double(at)*.1),y=float(5000.+double(at)*.1);
  if(double(y)-double(x)>5000.){first=at;break;}}
 if(first!=1)throw std::runtime_error("float oracle must expose1ms hidden exit");
 std::cout<<"PASS negative control: nominal sparse exit absent; exact float exit1ms\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
