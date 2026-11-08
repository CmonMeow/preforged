#include "gw_dagger_family_conditions_detached.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace fw::gw::combat;
using namespace fw::gw::combat::weapon;
using namespace fw::gw::skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("detached remaining dagger conditions");}
int main()try{
    const Scope source{1,25,1},target{1,30,2};const QualifiedHit hit{source,target,1,90,100,AttackDelivery::melee};
    DaggerChain offhand;offhand.lead(target,10);offhand.offhand(target,20);
    DetachedDaggerContactFacts facts;facts.target_hexed=true;facts.target_knocked_down=true;
    facts.target_has_adjacent_ally=false;facts.target_health=Health{91,100};
    for(unsigned rank=0;rank<=20;++rank)for(unsigned id:{775u,776u,777u,778u,779u,783u}){
        const auto plan=detached_dagger_family_plan(id,rank);check(bool(plan));
        const auto effect=detached_dagger_family_effect(*plan,offhand,hit,facts);check(bool(effect));
        check(effect->bonus==plan->bonus+(id==783?plan->extra:0));
        check(effect->strikes==(id<=777?2u:1u));
        check(effect->condition_count==(id==776?2u:id==778?1u:0u));
        if(id==775)check(effect->adjacent_damage==plan->bonus);
        if(id==777)check(effect->knockdown_ms==2000);
        if(id==779)check(effect->energy_gain==plan->energy);
        auto bad=*plan;++bad.duration_ms;check(!detached_dagger_family_effect(bad,offhand,hit,facts));
        check(!detached_dagger_family_effect(*plan,offhand,hit,facts,27,false));
        check(!detached_dagger_family_plan(id,21));
    }
    DaggerChain empty;
    const auto spider=*detached_dagger_family_plan(778,0);
    check(bool(detached_dagger_family_effect(spider,empty,hit,facts))); // KD exception skips lead
    auto unknown=facts;unknown.target_knocked_down.reset();check(!detached_dagger_family_effect(spider,empty,hit,unknown));
    unknown.target_knocked_down=false;check(!detached_dagger_family_effect(spider,empty,hit,unknown));
    const auto twisting=*detached_dagger_family_plan(776,0);
    check(!detached_dagger_family_effect(twisting,empty,hit,facts));
    const auto twist=*detached_dagger_family_effect(twisting,offhand,hit,facts);
    std::vector<conditions::Kind> order;
    const auto projected=detached_dagger_project_conditions(twist,conditions::State{},
        [&](conditions::State& state,conditions::Kind kind,unsigned cause,std::uint64_t at,unsigned duration){
            order.push_back(kind);return state.apply(kind,cause,at,duration);
        });
    check(projected&&order==std::vector<conditions::Kind>{conditions::Kind::bleeding,conditions::Kind::deep_wound});
    check(projected->has(conditions::Kind::bleeding,100)&&projected->has(conditions::Kind::deep_wound,100));
    const auto second_hit=QualifiedHit{source,target,2,90,101,AttackDelivery::melee};
    check(bool(detached_dagger_family_effect(twisting,offhand,second_hit,facts)));
    const auto unsuspecting=*detached_dagger_family_plan(783,0);facts.target_health=Health{90,100};
    check(detached_dagger_family_effect(unsuspecting,empty,hit,facts)->bonus==19);
    facts.target_health=Health{90.01,100};check(detached_dagger_family_effect(unsuspecting,empty,hit,facts)->bonus==34);
    const auto lotus=*detached_dagger_family_plan(779,0);facts.target_hexed=false;
    check(detached_dagger_family_effect(lotus,empty,hit,facts)->energy_gain==0);
    facts.target_hexed.reset();check(!detached_dagger_family_effect(lotus,empty,hit,facts));
    const auto horns=*detached_dagger_family_plan(777,0);facts.target_has_adjacent_ally=true;
    check(detached_dagger_family_effect(horns,offhand,hit,facts)->knockdown_ms==0);
    facts.target_has_adjacent_ally.reset();check(!detached_dagger_family_effect(horns,offhand,hit,facts));
    check(!detached_dagger_family_plan(782,0));
    std::cout<<"PASS detached six-dagger plans and per-contact condition requests; dual World ownership pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
