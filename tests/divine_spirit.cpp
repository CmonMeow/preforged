#include "gw_divine_spirit.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;using namespace fw::gw::skills::effects;
void check(bool value){if(!value)throw std::runtime_error("Divine Spirit regression");}
int main()try{
    const Scope owner{1,42,1},other{1,43,1};
    check(divine_spirit(0)->duration==1000&&divine_spirit(12)->duration==11000&&divine_spirit(15)->duration==14000&&!divine_spirit(21));
    unsigned cases=0;
    for(unsigned rank=0;rank<=20;++rank){
        Registry registry(owner);const auto plan=*divine_spirit(rank);
        check(registry.apply(owner,{7,42,310,rank,Category::enchantment,100,100+plan.duration},Replace::same_skill).accepted);
        for(unsigned cost:{1u,5u,10u,15u,25u}){
            check(divine_spirit_cost(*find_skill(290),cost,registry,owner,100)==(cost<=5?1u:cost-5));
            check(divine_spirit_cost(*find_skill(174),cost,registry,owner,100)==cost);
            check(divine_spirit_cost(*find_skill(297),cost,registry,owner,100)==cost);
            check(divine_spirit_cost(*find_skill(290),cost,registry,owner,99)==cost);
            check(divine_spirit_cost(*find_skill(290),cost,registry,owner,100+plan.duration)==cost);++cases;
        }
        check(!divine_spirit_cost(*find_skill(290),10,registry,other,100));
        check(!divine_spirit_cost(*find_skill(290),0,registry,owner,100));
        check(registry.remove(owner,7).has_value());check(divine_spirit_cost(*find_skill(290),10,registry,owner,100)==10);
    }
    Registry foreign(owner);check(foreign.apply(owner,{1,43,310,12,Category::enchantment,100,1100},Replace::same_skill).accepted);
    check(!divine_spirit_cost(*find_skill(290),10,foreign,owner,100));
    std::cout<<"PASS "<<cases<<" rank/cost contexts,Monk spell classification,timing,removal and scope guards;World/casting/stack order pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
