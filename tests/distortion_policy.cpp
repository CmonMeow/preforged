#include "gw_distortion.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Distortion policy regression");}
int main()try{
 check(distortion(0)->duration==1000&&distortion(12)->duration==4000&&distortion(15)->duration==5000&&distortion(20)->duration==6000&&!distortion(21));
 const Scope owner{1,2,1};Registry registry(owner);
 check(registry.apply(owner,{9,2,11,12,Category::stance,100,4100},Replace::category).accepted);
 for(unsigned energy:{0u,1u,2u,10u}){
  const auto p=distortion_block(registry,owner,energy,200);check(bool(p));
  check(p->stance_instance==9&&p->ends==(energy<2));
  check(bool(p->registry.lookup(owner,9))==(energy>=2));
  if(energy>=2)check(p->remaining_energy&&*p->remaining_energy==energy-2);
  else check(!p->remaining_energy);
  check(bool(registry.lookup(owner,9))); // Proposal never spends live state.
 }
 check(!distortion_block(registry,{1,2,2},10,200));
 check(!distortion_block(registry,owner,10,99)&&!distortion_block(registry,owner,10,4100));
 Registry foreign(owner);check(foreign.apply(owner,{10,3,11,12,Category::stance,100,4100},Replace::category).accepted);
 check(!distortion_block(foreign,owner,10,200));
 std::cout<<"PASS Distortion duration, attributed-block proposal, paid/insufficient Energy, scope/expiry and immutable original; World/casting pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
