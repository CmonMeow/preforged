#include "gw_endure_pain.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=endure_pain(rank);
  if(!p||p->rank!=rank||p->health_bonus!=90+14*rank||p->energy!=5||p->duration!=std::uint64_t(std::lround(7.+11.*rank/15))*1000||p->recharge!=30000)throw std::runtime_error("Endure Pain pinned plan");}
 if(endure_pain(21)||endure_pain(UINT32_MAX))throw std::runtime_error("Endure Pain rank guard");
 std::cout<<"PASS Endure Pain21pinned rank plans;temporary Health integration and normal cast remain unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
