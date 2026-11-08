#include "gw_dagger_chain.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
static void check(bool valid){if(!valid)throw std::runtime_error("dagger chain");}
int main()try{
    DaggerChain chain;
    const Scope first{1,20,1},other{1,21,1},stale{1,20,2};
    check(!chain.offhand(first,100)&&!chain.dual(first,100));
    check(chain.lead(first,100)&&chain.current(first,100)==1);
    check(!chain.offhand(other,101)&&!chain.offhand(stale,101));
    check(!chain.dual(first,101)&&chain.offhand(first,101));
    check(chain.current(first,101)==2&&chain.dual(first,102));
    check(chain.current(first,102)==3&&!chain.dual(first,103));
    check(chain.lead(other,104)&&chain.current(first,104)==0);
    check(chain.current(other,15104)==0&&!chain.offhand(other,15104));
    check(chain.lead(first,UINT64_MAX-15000));
    check(!chain.offhand(first,UINT64_MAX-14999));
    chain.clear();check(chain.current(first,110)==0);
    std::cout<<"PASS target-scoped dagger stages, expiry and overflow admission\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
