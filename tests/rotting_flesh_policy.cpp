#include "gw_rotting_flesh.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
int main()try{
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=rotting_flesh(rank);
  if(!plan||plan->rank!=rank||plan->range!=1248||plan->duration!=(10+rank)*1000)
   throw std::runtime_error("Rotting Flesh native rank plan mismatch");
 }
 if(rotting_flesh(21))throw std::runtime_error("Invalid rank accepted");
 std::cout<<"PASS Rotting Flesh pinned native fields and duration ranks;World/Runtime not yet bound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
