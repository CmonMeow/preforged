#include "gw_healing_ring.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool x){if(!x)throw std::runtime_error("Healing Ring recipient policy");}
int main()try{
 const Scope owner{1,1,10};
 for(unsigned rank:{0u,12u,15u,20u})for(unsigned divine:{0u,12u,20u}){
  const auto plan=*healing_ring(rank,divine);check(plan.healing==30+rank*10);
  std::vector<RingCandidate> candidates{{owner,0,0,0,true,RingCreature::player},{{1,2,20},0,166,0,true,RingCreature::player},{{1,3,30},0,-166,0,true,RingCreature::creature},{{1,4,40},0,0,0,true,RingCreature::spirit},{{1,5,50},0,166.001,0,true,RingCreature::unknown},{{1,6,60},1,0,0,true,RingCreature::creature},{{1,7,70},0,0,0,false,RingCreature::creature}};
  const auto result=healing_ring_recipients(plan,owner,candidates);check(result&&result->size()==(divine?3u:2u));
  for(const auto& r:*result)check(r.healing==(r.scope==owner?plan.caster_healing:plan.healing));
  auto unknown=candidates;unknown[2].kind=RingCreature::unknown;check(!healing_ring_recipients(plan,owner,unknown));
  auto reused=candidates;reused.push_back({{1,2,99},0,0,0,true,RingCreature::player});check(!healing_ring_recipients(plan,owner,reused));
  auto stale=owner;++stale.generation;check(!healing_ring_recipients(plan,stale,candidates));
  auto invalid=plan;invalid.radius=167;check(!healing_ring_recipients(invalid,owner,candidates));
  std::reverse(candidates.begin(),candidates.end());const auto reversed=healing_ring_recipients(plan,owner,candidates);check(reversed&&reversed->size()==result->size());
 }
 check(!healing_ring(21,0)&&!healing_ring(0,21));
 std::cout<<"PASS Healing Ring rank/caster Divine Favor, mixed creature recipients, spirit/dead/plane/range exclusions and immutable admission failures; World binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
