#include "gw_dagger_transaction_detached.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat;
using namespace fw::gw::combat::weapon;
using namespace fw::gw::skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("detached dagger transaction");}
int main()try{
    const Scope source{1,25,1},target_scope{1,30,2};
    const QualifiedHit qualified{source,target_scope,1,90,100,AttackDelivery::melee};
    const auto lead=*dagger_attack_policy(782,0),fox=*dagger_attack_policy(780,0);
    DaggerChain chain;conditions::State conditions;
    Actor actor;actor.id=30;actor.generation=2;actor.health=Health{100,100};actor.damageable=true;
    const auto condition=[](Actor&,conditions::State& state,unsigned cause,std::uint64_t now,unsigned base){
        return state.apply(conditions::Kind::bleeding,cause,now,base);
    };
    const auto damage=[&](Actor& copy,unsigned bonus)->std::optional<Hit>{
        return apply(copy,{Kind::armor_ignoring,20.+bonus,20,782},true);
    };
    const auto result=prepare_detached_dagger_transaction(lead,chain,qualified,32,false,actor,conditions,damage,condition);
    check(result&&result->target.health->current==80&&result->condition_applied&&result->publish_stage);
    check(result->conditions.has(conditions::Kind::bleeding,100)&&result->chain.stage==1);
    check(actor.health->current==100&&chain.stage==0&&!conditions.has(conditions::Kind::bleeding,100));
    unsigned called=0;
    const auto lethal=[&](Actor& copy,unsigned)->std::optional<Hit>{return apply(copy,{Kind::armor_ignoring,100,20,782},true);};
    const auto counted=[&](Actor&,conditions::State&,unsigned,std::uint64_t,unsigned){++called;return true;};
    const auto dead=prepare_detached_dagger_transaction(lead,chain,qualified,32,false,actor,conditions,lethal,counted);
    check(dead&&!dead->publish_stage&&!dead->condition_applied&&dead->chain.stage==0&&called==0);
    const auto reject=[](Actor&,conditions::State&,unsigned,std::uint64_t,unsigned){return false;};
    check(!prepare_detached_dagger_transaction(lead,chain,qualified,32,false,actor,conditions,damage,reject));
    const auto reaction_death=[](Actor& copy,conditions::State& state,unsigned cause,std::uint64_t now,unsigned base){
        if(!state.apply(conditions::Kind::bleeding,cause,now,base))return false;
        copy.health->current=0;copy.effects|=0x10;return true;
    };
    const auto reaction=prepare_detached_dagger_transaction(lead,chain,qualified,32,false,actor,conditions,damage,reaction_death);
    check(reaction&&reaction->condition_applied&&!reaction->publish_stage&&reaction->chain.stage==0);
    auto active=result->chain;
    const auto fox_damage=[](Actor& copy,unsigned bonus)->std::optional<Hit>{return apply(copy,{Kind::armor_ignoring,20.+bonus,20,780},true);};
    const auto second=prepare_detached_dagger_transaction(fox,active,qualified,32,false,actor,conditions,fox_damage,counted);
    check(second&&second->target.health->current==70&&!second->condition_applied&&second->chain.stage==2&&called==0);
    const auto forged=[&](Actor& copy,unsigned bonus)->std::optional<Hit>{auto hit=damage(copy,bonus);++hit->generation;return hit;};
    check(!prepare_detached_dagger_transaction(lead,chain,qualified,32,false,actor,conditions,forged,condition));
    actor.generation=3;check(!prepare_detached_dagger_transaction(lead,chain,qualified,32,false,actor,conditions,damage,condition));
    std::cout<<"PASS detached dagger copied damage/condition composition; no World/payment/replay authority\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
