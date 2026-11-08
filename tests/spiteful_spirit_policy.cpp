#include "gw_spiteful_spirit.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=spiteful_spirit(rank);
  if(!p||p->rank!=rank||p->range!=1248||p->radius!=166||p->damage!=5+rank*2)
   throw std::runtime_error("Spiteful Spirit canonical plan mismatch");
 }
 if(spiteful_spirit(21)||spiteful_spirit(0)->duration!=8000||spiteful_spirit(12)->duration!=18000||spiteful_spirit(20)->duration!=24000)
  throw std::runtime_error("Spiteful Spirit rank/duration mismatch");
 std::cout<<"PASS Spiteful Spirit pinned native fields,rank damage/duration and adjacent radius;World/Runtime are separate checks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
