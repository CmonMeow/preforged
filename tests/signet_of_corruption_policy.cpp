#include "../src/runtime/gw_signet_of_corruption.h"
#include <stdexcept>
#include <iostream>
using namespace fw::gw::skills::effects;
static void check(bool ok){if(!ok)throw std::runtime_error("Signet of Corruption policy");}
int main(){
 constexpr unsigned damage[]{20,21,23,25,27,28,30};
 constexpr unsigned cap[]{12,13,15,16,17,18,20};
 for(unsigned skill:{1950u,2093u})for(unsigned rank=0;rank<=12;++rank){
  auto plan=signet_of_corruption(skill,rank);check(plan.has_value());
  check(plan->damage==damage[std::min(rank,6u)]&&plan->energy_cap==cap[std::min(rank,6u)]);
  check(signet_of_corruption_energy(*plan,0)==0);
  check(signet_of_corruption_energy(*plan,1)==2);
  check(signet_of_corruption_energy(*plan,100)==plan->energy_cap);
  auto altered=*plan;++altered.damage;check(!signet_of_corruption_energy(altered,2));
  const CorruptionFoe center{1,1,0,0,true,true,true,true};
  const CorruptionFoe nearby{2,1,100,0,true,true,false,true};
  const CorruptionFoe plain{3,1,200,0,true,true,false,false};
  const CorruptionFoe far{4,1,400,0,true,true,true,false};
  const CorruptionFoe ally{5,1,50,0,false,true,true,false};
  const CorruptionFoe foes[]{center,nearby,plain,far,ally};
  const auto area=signet_of_corruption_area(*plan,center,foes);
  check(area&&area->contacts.size()==3&&area->energy==4);
  check(area->contacts[0].damage==plan->damage&&area->contacts[0].energy_qualified);
  check(!area->contacts[2].energy_qualified);
  const CorruptionFoe duplicate[]{center,center};
  check(!signet_of_corruption_area(*plan,center,duplicate));
 }
 check(!signet_of_corruption(2093,13)&&!signet_of_corruption(0,0));
 std::cout<<"PASS staged Signet of Corruption variants, rank scaling and capped conditional energy; World/Runtime pending\n";
}
