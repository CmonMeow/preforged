#include "gw_whirling_defense.h"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace fw::gw::skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Whirling Defense policy");}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){auto p=whirling_defense(rank);check(p&&p->block==.75&&p->radius==166.f);}
 check(whirling_defense(0)->duration==8000&&whirling_defense(12)->duration==18000&&whirling_defense(15)->duration==20000);
 check(whirling_defense(0)->retaliation==5&&whirling_defense(12)->retaliation==10&&whirling_defense(15)->retaliation==11);
 check(!whirling_defense(21));
 const Scope owner{1,25,2},foe{1,26,3};Registry registry(owner);
 check(registry.apply(owner,{100,25,450,12,Category::stance,100,18100},Replace::category).accepted);
 using Outcome=fw::gw::combat::weapon::AttackOutcome;
 auto reaction=whirling_retaliation(registry,owner,Outcome::block,true,200);
 check(reaction&&reaction->damage==10&&reaction->source==owner);
 fw::gw::combat::weapon::AttackOutcomes ledger(foe);
 using Delivery=fw::gw::combat::weapon::AttackDelivery;
 check(ledger.release(foe,owner,1,100,Delivery::ranged));
 const auto resolved=ledger.finish(foe,owner,1,Outcome::block,200);
 check(resolved&&whirling_resolved_retaliation(registry,owner,*resolved).has_value());
 check(!ledger.finish(foe,owner,1,Outcome::block,200));
 auto unknown=*resolved;unknown.delivery=Delivery::unknown;
 check(!whirling_resolved_retaliation(registry,owner,unknown));
 unknown.delivery=Delivery::melee;check(!whirling_resolved_retaliation(registry,owner,unknown));
 unknown=*resolved;unknown.event=0;check(!whirling_resolved_retaliation(registry,owner,unknown));
 unknown=*resolved;unknown.target.generation++;check(!whirling_resolved_retaliation(registry,owner,unknown));
 check(!whirling_retaliation(registry,owner,Outcome::hit,true,200));
 check(!whirling_retaliation(registry,owner,Outcome::block,false,200));
 check(!whirling_retaliation(registry,owner,Outcome::block,true,99));
 check(!whirling_retaliation(registry,owner,Outcome::block,true,18100));
 check(!whirling_retaliation(registry,{1,25,3},Outcome::block,true,200));
 check(whirling_retaliation_target(*reaction,foe,true,true,166));
 check(!whirling_retaliation_target(*reaction,foe,true,true,166.001));
 check(!whirling_retaliation_target(*reaction,foe,true,true,-1));
 check(!whirling_retaliation_target(*reaction,foe,true,true,std::numeric_limits<double>::quiet_NaN()));
 check(!whirling_retaliation_target(*reaction,foe,false,true,1));
 check(!whirling_retaliation_target(*reaction,foe,true,false,1));
 check(!whirling_retaliation_target(*reaction,owner,true,true,1));
 check(!whirling_retaliation_target(*reaction,{2,26,3},true,true,1));
 check(registry.apply(owner,{101,25,453,0,Category::stance,300,1000},Replace::category).accepted);
 check(!whirling_retaliation(registry,owner,Outcome::block,true,300));
 std::cout<<"PASS Whirling Defense rank,projectile-only block reaction,adjacency,scope,expiry,replacement\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
