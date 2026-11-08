#include "gw_i_am_unstoppable.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("Unstoppable policy");}
int main()try{
 for(unsigned rank=0;rank<=10;++rank){
  const auto plan=i_am_unstoppable(rank);
  check(plan&&plan->norn_rank==rank&&plan->armor_after_cap==24&&plan->recharge==30000);
  check(plan->duration>=16000&&plan->duration<=20000);
 }
 check(i_am_unstoppable(0)->duration==16000&&i_am_unstoppable(10)->duration==20000);
 check(!i_am_unstoppable(11));
 const Scope owner{1,25,3};Registry effects(owner);
 check(effects.apply(owner,{1,25,2356,10,Category::other,100,20100},Replace::same_skill).accepted);
 check(!unstoppable_active(effects,{1,25,4},100));
 check(unstoppable_active(effects,owner,100)&&unstoppable_post_cap_armor(effects,owner,100)==24);
 check(unstoppable_active(effects,owner,20099)&&!unstoppable_active(effects,owner,20100));
 check(effects.apply(owner,{2,25,2356,10,Category::other,30000,35000},Replace::same_skill).accepted);
 check(!unstoppable_active(effects,owner,30001));
 std::cout<<"PASS Norn rank0..10 metadata,scoped duration and post-cap armor classification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
