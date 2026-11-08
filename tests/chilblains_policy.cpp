#include "gw_chilblains.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
int main()try{
    const auto check=[](bool value){if(!value)throw std::runtime_error("Chilblains policy");};
    using K=combat::conditions::Kind;
    check(skills::effects::captured_chilblains_order==skills::effects::ChilblainsOrder::damage_then_remove);
    for(unsigned rank=0;rank<=20;++rank){
        const auto p=skills::effects::chilblains(rank);check(bool(p));
        check(p->enchantments==(rank<8?1:2));
        check(std::abs(p->damage-(10.+34.*rank/15.))<1e-9);
        check(p->energy==25&&p->activation==2000&&p->aftercast==750&&p->recharge==8000&&p->radius==322);
        combat::conditions::State original;
        const auto proposed=skills::effects::chilblains_poison(*p,original,42,1000);
        check(proposed&&proposed->has(K::poison,1000)&&proposed->has(K::poison,10999)&&!proposed->has(K::poison,11000));
        check(!original.has(K::poison,1000)&&proposed->lookup(K::poison)->cause==42);
        check(!skills::effects::chilblains_poison(*p,original,0,1000));
        check(!skills::effects::chilblains_poison(*p,original,65536,1000));
        check(!skills::effects::chilblains_poison(*p,original,42,UINT64_MAX-9999));
        auto forged=*p;++forged.enchantments;check(!skills::effects::chilblains_poison(forged,original,42,1000));
        check(original.apply(K::poison,7,100,20000));
        const auto longer=skills::effects::chilblains_poison(*p,original,42,1000);
        check(longer&&longer->lookup(K::poison)->expires==20100&&longer->lookup(K::poison)->cause==7);
    }
    check(!skills::effects::chilblains(21));
    std::cout<<"PASS Chilblains ranks0..20, native plan guards, copy-only self-poison and longer-duration preservation; World binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
