#include "gw_ether_renewal.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Ether Renewal line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned skill:{181u,2860u})for(unsigned rank=0;rank<=20;++rank){
        const auto p=ether_renewal(skill,rank);check(bool(p));
        check(p->healing_per_enchantment==5+rank);
        check(p->duration==(skill==181?1000ull*(5+rank):7000ull));
    }
    check(!ether_renewal(181,21)&&!ether_renewal(180,12));
    const Scope owner{7,25,3},stale{7,25,4};Registry empty(owner);
    check(!ether_renewal_reward(empty,owner,174,100));
    auto first=prepare_ether_renewal(empty,owner,80,*ether_renewal(181,15),100);
    check(bool(first)&&first->replaced.empty());
    check(!empty.newest_skill(owner,181,100));
    auto one=ether_renewal_reward(first->registry,owner,174,101);
    check(one&&one->enchantments==1&&one->energy==4&&one->healing==20);
    check(!ether_renewal_reward(first->registry,stale,174,101));
    check(!ether_renewal_reward(first->registry,owner,322,101)); // Attack.
    check(!ether_renewal_reward(first->registry,owner,200,101)); // Glyph.
    check(!ether_renewal_reward(first->registry,owner,2,101)); // Signet.
    check(first->registry.apply(owner,{81,25,180,0,Category::enchantment,100,60100},Replace::same_skill).accepted);
    check(first->registry.apply(owner,{82,25,200,0,Category::glyph,100,15100},Replace::same_skill).accepted);
    auto two=ether_renewal_reward(first->registry,owner,174,101);
    check(two&&two->enchantments==2&&two->energy==8&&two->healing==40);
    check(!ether_renewal_reward(first->registry,owner,174,20100));
    auto replacement=prepare_ether_renewal(first->registry,owner,83,*ether_renewal(2860,15),200);
    check(replacement&&replacement->replaced.size()==1&&replacement->replaced[0].instance==80);
    check(!replacement->registry.lookup(owner,80));
    auto pvp=ether_renewal_reward(replacement->registry,owner,174,201);
    check(pvp&&pvp->enchantments==2&&pvp->energy==6&&pvp->healing==40);
    check(!ether_renewal_reward(replacement->registry,owner,174,7200));
    auto forged=*ether_renewal(181,15);forged.energy_per_enchantment=99;
    check(!prepare_ether_renewal(empty,owner,80,forged,100));
    check(!prepare_ether_renewal(empty,stale,80,*ether_renewal(181,15),100));
    std::cout<<"PASS Ether Renewal PvE/PvP plans, scoped enchantment rewards and renewal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
