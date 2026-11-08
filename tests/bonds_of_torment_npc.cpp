#include "gw_bonds_of_torment_npc.h"
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bonds of Torment line "+std::to_string(at.line()));
}
int main(){
    using namespace skills::effects;
    const auto plan=*bonds_of_torment_npc();
    const Scope caster{7,154,1}, target{7,39,1}, another{7,683,1};
    BondsOfTorment bonds;
    const BondsOfTorment::Link first{caster,target,101,1000,31000};
    const BondsOfTorment::Link second{caster,another,102,2000,32000};
    check(bonds.add(plan,first));
    check(!bonds.add(plan,first));
    check(bonds.add(plan,second));
    auto early=bonds.propagate(caster,1500,1,42);
    check(early&&early->size()==1&&early->front().target==target&&
          early->front().kind==combat::Kind::cold&&early->front().cold_damage==42);
    auto both=bonds.propagate(caster,2000,2,75);
    check(both&&both->size()==2&&(*both)[0].effect==101&&(*both)[1].effect==102);
    check(!bonds.propagate(caster,2000,2,75));
    check(!bonds.propagate(caster,2000,1,75));
    check(!bonds.propagate(caster,2000,3,-1));
    check(!bonds.propagate(caster,2000,3,std::numeric_limits<double>::infinity()));
    check(!bonds.erase(caster,target,999));
    check(bonds.erase(caster,target,101));
    auto remaining=bonds.propagate(caster,31000,3,10);
    check(remaining&&remaining->size()==1&&remaining->front().target==another);
    check(bonds.expire(32000)==1&&bonds.size()==0);
    check(bonds.propagate(caster,32000,4,10)->empty());
    check(bonds.add(plan,{caster,target,104,32000,62000}));
    check(!bonds.propagate(caster,32000,4,10));
    check(!bonds.propagate(caster,31999,5,10));
    check(bonds.propagate(caster,32000,5,10)->size()==1);
    check(bonds.erase(caster,target,104));
    check(!bonds.add(plan,{caster,target,103,UINT64_MAX,0}));
    check(!bonds.add(plan,{caster,{8,39,1},103,0,30000}));
    auto forged=plan;++forged.duration;
    check(!bonds.add(forged,{caster,target,103,0,30000}));
    check(bonds.add(plan,first));
    bonds.discard(caster);
    check(bonds.size()==0);
    check(bonds.add(plan,first));
    bonds.discard(target);
    check(bonds.size()==0);

    Registry registry(target);
    const auto prepared=prepare_bonds_of_torment_application(registry,bonds,caster,target,200,plan,5000);
    check(prepared&&prepared->change.accepted&&prepared->ledger.size()==1);
    check(!registry.lookup(target,200)&&bonds.size()==0);
    check(prepared->registry.lookup(target,200)->skill==1881);
    check(prepared->entry.expires==35000);
    const auto replaced=prepare_bonds_of_torment_application(prepared->registry,
        prepared->ledger,caster,target,201,plan,6000);
    check(replaced&&replaced->change.removed.size()==1&&
          replaced->change.removed.front().instance==200&&replaced->ledger.size()==1);
    check(!replaced->registry.lookup(target,200));
    check(replaced->registry.lookup(target,201)->expires==36000);
    check(!prepare_bonds_of_torment_application(prepared->registry,prepared->ledger,
        caster,target,200,plan,6000));
    check(!prepare_bonds_of_torment_application(prepared->registry,prepared->ledger,
        caster,target,201,forged,6000));
}
