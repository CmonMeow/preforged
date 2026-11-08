#include "gw_defy_pain.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
int main()try{
 for(bool pvp:{false,true})for(unsigned rank=0;rank<=20;++rank){const auto p=defy_pain(rank,pvp);
  if(!p||p->health_bonus!=90+14*rank||p->armor!=20||p->adrenaline!=120||p->duration!=20000||p->stance!=pvp||p->skill!=(pvp?3204u:318u)||p->reduction!=(pvp?0u:unsigned(std::lround(1.+9.*rank/15))))throw std::runtime_error("Defy Pain mode/rank");}
 if(defy_pain(21)||defy_pain(UINT32_MAX,true))throw std::runtime_error("Defy Pain rank guard");
 std::cout<<"PASS42DefyPain plans;PvE skill/reduction and PvP stance/no-reduction;World/casting remain unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
