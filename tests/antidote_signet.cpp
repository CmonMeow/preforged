#include "gw_antidote_signet.h"
#include <iostream>
#include <bit>
#include <stdexcept>
using namespace fw::gw;
void check(bool x){if(!x)throw std::runtime_error("Antidote Signet policy");}
int main()try{using namespace combat::conditions;
 for(unsigned mask=0;mask<1024;++mask){State state;for(unsigned k=0;k<10;++k)if(mask&(1u<<k))check(state.apply(static_cast<Kind>(k),42,100,1000));
  const auto removals=skills::effects::antidote_signet_removals(state,100);State after=state;
  unsigned named=0;for(auto kind:{Kind::poison,Kind::disease,Kind::blind})named+=state.has(kind,100);
  check(removals.size()==named+(state.count(100)>named?1u:0u));
  for(auto kind:removals)check(after.remove(kind));
  for(auto kind:{Kind::poison,Kind::disease,Kind::blind})check(!after.has(kind,100));
  check(state.count(100)==unsigned(std::popcount(mask)));
  check(skills::effects::antidote_signet_removals(state,1100).empty());
 }
 State covered;check(covered.apply(Kind::bleeding,42,100,1000)&&covered.apply(Kind::weakness,42,100,1000)&&covered.apply(Kind::bleeding,42,101,1));
 check(skills::effects::antidote_signet_removals(covered,101)==std::vector{Kind::bleeding});
 std::cout<<"PASS Antidote all1024 condition sets, named removals plus newest additional, expiry, covered ordering and immutable source\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
