#include "gw_rigor_mortis.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Rigor Mortis registry regression");}
int main()try{
 const Scope target{1,2,3};Registry registry(target);const auto plan=*rigor_mortis(12);
 check(registry.apply(target,{1,2,258,12,Category::enchantment,100,6100},Replace::same_skill).accepted);
 check(reviewed_block_probability(registry.active(target,100),false,100)==.5);
 check(bool(apply_rigor_mortis(registry,target,2,1,plan,100)));
 check(reviewed_block_probability(registry.active(target,100),false,100)==0);
 using Outcome=fw::gw::combat::weapon::AttackOutcome;
 check(resolve_block_contact(Outcome::miss,*reviewed_block_probability(registry.active(target,100),false,100),0)==Outcome::miss);
 const auto replacement=apply_rigor_mortis(registry,target,3,4,plan,200);
 check(replacement&&replacement->removed.size()==1&&replacement->removed[0].instance==2);
 check(registry.active(target,200).size()==2); // Guardian plus the replacement hex.
 check(registry.remove(target,3).has_value());
 check(reviewed_block_probability(registry.active(target,200),false,200)==.5);
 const auto low=*rigor_mortis(0);check(bool(apply_rigor_mortis(registry,target,4,1,low,300)));
 check(registry.apply(target,{5,2,258,12,Category::enchantment,8000,14000},Replace::same_skill).accepted);
 check(reviewed_block_probability(registry.active(target,8299),false,8299)==0);
 check(reviewed_block_probability(registry.active(target,8300),false,8300)==.5);
 auto forged=plan;forged.duration=1;
 check(!apply_rigor_mortis(registry,target,6,1,forged,9000));
 check(!apply_rigor_mortis(registry,{1,2,4},6,1,plan,9000));
 check(!apply_rigor_mortis(registry,target,6,2,plan,9000));
 check(!apply_rigor_mortis(registry,target,6,1,plan,UINT64_MAX));
 check(registry.active(target,9000).size()==1);
 std::cout<<"PASS scoped Rigor Mortis disables Guardian blocking,retains misses,renews/removes/expires and rejects stale/forged/overflow inputs;World casting pending\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
