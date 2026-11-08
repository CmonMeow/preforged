#include "gw_energy_degeneration_hexes.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Energy degeneration hex policy mismatch");}
int main()try{
 check(!energy_degeneration_hex(121,12)&&!energy_degeneration_hex(125,21)&&!energy_degeneration_hex(140,21));
 for(unsigned id:{125u,140u})for(unsigned rank=0;rank<=20;++rank){
  const auto plan=energy_degeneration_hex(id,rank);check(plan&&valid_energy_degeneration_hex(*plan));
  check(plan->duration==(5+2*rank)*1000&&plan->energy_degen==1&&plan->range==1248);
  check(plan->zero_damage==(id==125?15+4*rank:5+3*rank));
  check(id==125?plan->caster_health_degen==0:plan->target_health_degen==0&&plan->caster_health_degen==1);
  auto forged=*plan;++forged.energy_degen;check(!valid_energy_degeneration_hex(forged));
  forged=*plan;++forged.zero_damage;check(!valid_energy_degeneration_hex(forged));
 }
 check(energy_degeneration_hex(125,0)->target_health_degen==2);
 check(energy_degeneration_hex(125,12)->target_health_degen==4);
 check(energy_degeneration_hex(125,20)->target_health_degen==5);
 check(energy_degeneration_hex(140,12)->zero_damage==41);
 std::cout<<"PASS canonical Wither/Malaise native fields,rank0..20,Health/Energy degeneration and zero-Energy damage plans;World/Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
