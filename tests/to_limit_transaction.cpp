#include "gw_to_limit_transaction.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("To the Limit transaction line "+std::to_string(at.line()));
}
template<class... Args> ToLimitProposal require_proposal(Args&&... args){
    auto result=prepare_to_limit(std::forward<Args>(args)...);check(result.has_value());return std::move(*result);
}
int main()try{
    const Scope owner{1,1,1};skills::Slots bar{};bar[0]={335,0};bar[1]={335,1};bar[2]={336,0};
    for(unsigned rank=0;rank<=20;++rank)for(unsigned foes:{0u,1u,100u})for(unsigned mode=0;mode<5;++mode){
        TemporaryHealth health(owner,500,100);Registry registry(owner);combat::adrenaline::State pools;
        check(pools.configure(bar,100));
        // Modes:ordinary,PvE Justice,Berserker,PvP Justice,Soothing Images.
        if(mode){const auto skill=mode==1?343u:mode==2?370u:mode==3?2883u:56u;
            check(registry.apply(owner,{9,mode==4?2u:1u,skill,0,category(*skills::find_skill(skill)),100,1000},Replace::same_skill).accepted);}
        const auto plan=*to_limit(rank);const TemporaryHealthSnapshot snapshot{owner,200,400,{500,0,false,0}};
        const auto result=prepare_to_limit(health,snapshot,registry,pools,plan,10,foes,false);check(result.has_value());
        check(result->health.health.current==400+plan.health_bonus&&result->health.ledger.permanent()==500);
        check(result->registry.lookup(owner,10)->skill==316&&result->gain_forbidden==(mode==4));
        const auto halves=std::uint64_t(25)*std::min(foes,plan.maximum_foes)*(mode==1?4u:mode==2?3u:2u);
        for(const auto& pool:result->adrenaline.pools()){
            const auto exact=mode==4?0:std::min<std::uint64_t>(2ull*pool.cost,halves);
            check(pool.units==exact/2&&pool.half==exact%2);
        }
        // Preparing or discarding never changes any input authority.
        check(health.capacity()==500&&!registry.lookup(owner,10));for(const auto& p:pools.pools())check(p.units==0&&p.half==0);
        auto later=result->health.health;later.at=201;
        const auto refresh=prepare_to_limit(result->health.ledger,later,result->registry,result->adrenaline,plan,11,0,false);
        check(refresh&&refresh->health.health.current==later.current&&refresh->health.removed==10);
        check(!refresh->registry.lookup(owner,10)&&refresh->registry.lookup(owner,11));
    }
    TemporaryHealth health(owner,500,100);Registry registry(owner);combat::adrenaline::State pools;check(pools.configure(bar,100));
    const auto plan=*to_limit(0);TemporaryHealthSnapshot snapshot{owner,200,400,{500,0,false,0}};
    auto stale=snapshot;++stale.owner.generation;check(!prepare_to_limit(health,stale,registry,pools,plan,10,1,false));
    auto dead=snapshot;dead.current=0;check(!prepare_to_limit(health,dead,registry,pools,plan,10,1,false));
    auto future=pools;future.tick(201);check(!prepare_to_limit(health,snapshot,registry,future,plan,10,1,false));
    auto occupied=registry;check(occupied.apply(owner,{10,1,343,0,Category::other,100,1000},Replace::same_skill).accepted);
    check(!prepare_to_limit(health,snapshot,occupied,pools,plan,10,1,false));
    auto orphan=registry;check(orphan.apply(owner,{10,1,316,0,Category::other,100,1000},Replace::same_skill).accepted);
    check(!prepare_to_limit(health,snapshot,orphan,pools,plan,11,1,false));
    const auto first=require_proposal(health,snapshot,registry,pools,plan,10,1,false);auto later=first.health.health;later.at=201;
    check(!prepare_to_limit(first.health.ledger,later,registry,first.adrenaline,plan,11,1,false));
    {auto disabled=pools;check(disabled.disable({335,1},300,100));
        const auto result=require_proposal(health,snapshot,registry,disabled,plan,10,1,false);
        check(result.adrenaline.pools()[0].units==25&&result.adrenaline.pools()[1].units==0&&result.adrenaline.pools()[2].units==25);
        check(result.adrenaline.pools()[1].disabled_until==300);}
    {auto statuses=registry;check(statuses.apply(owner,{9,1,343,0,Category::other,100,200},Replace::same_skill).accepted);
        const auto result=require_proposal(health,snapshot,statuses,pools,plan,10,1,false);
        check(result.adrenaline.pools()[0].units==25); // Expired Justice cannot double a later grant.
        auto wounded=snapshot;wounded.current=300;wounded.capacity={500,100,0,true};
        const auto wound=require_proposal(health,wounded,registry,pools,plan,10,1,true);
        check(wound.health.health.current==310&&wound.health.health.capacity.maximum()==410);}
    {auto full=pools;check(full.gain(1000,100));const auto result=require_proposal(health,snapshot,registry,full,plan,10,1,false);
        for(const auto& pool:result.adrenaline.pools())check(pool.units==pool.cost);
        check(result.health.health.current==410);}
    std::cout<<"PASS315 full Health/status/Adrenaline proposals;rates,forbidden gain,copies,refresh and rollback guards;normal casting unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
