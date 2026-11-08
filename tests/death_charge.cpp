#include "gw_death_charge.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Death's Charge stage");}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){auto p=death_charge(rank);check(p&&p->healing==65+9*rank);
  check(death_charge_healing(*p,{100,1000},{101,200})==p->healing);
  check(death_charge_healing(*p,{100,1000},{100,200})==0);
  check(death_charge_healing(*p,{100,1000},{99,200})==0);
  // Different percentages do not silently substitute for current Health.
  check(death_charge_healing(*p,{500,1000},{375,500})==0);
  check(death_charge_healing(*p,{100,100},{200,1000})==p->healing); // A raw quote is not a capped heal.
  auto bad=*p;++bad.healing;check(!death_charge_healing(bad,{100,1000},{200,1000}));
 }
 check(!death_charge(21));check(!death_charge_healing(*death_charge(0),{0,1000},{200,1000}));
 const auto effects=skills::particles::death_charge_trail_2022(42,0,0,1,100,200,2);check(effects.size()==2&&effects[0].size()==20&&effects[1].size()==20);
 check(effects[0][12]==42&&effects[1][12]==0&&effects[0][16]==146&&effects[0][17]==4&&effects[1][16]==146);
 std::cout<<"PASS Death's Charge21rank quote/guard contexts and origin/destination trails;not live\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
