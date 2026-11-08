#include "gw_dagger_attack_effects_detached.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat;
using namespace fw::gw::combat::weapon;
using namespace fw::gw::skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("detached dagger full effects");}
int main()try{
    fw::gw::combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;
    strike.location_armor=60;strike.item_type=32;strike.base_roll=strike.miss_roll=.5;
    Actor actor;actor.id=30;actor.generation=2;actor.health=Health{500,500};actor.damageable=true;
    const Scope source{1,25,1},target{1,30,2};const QualifiedHit qualified{source,target,1,90,100,AttackDelivery::melee};
    const auto condition=[](Actor&,conditions::State& state,unsigned cause,std::uint64_t at,unsigned duration){
        return state.apply(conditions::Kind::bleeding,cause,at,duration);
    };
    for(unsigned rank=0;rank<=20;++rank){
        DaggerChain chain;conditions::State state;
        const auto lead=*detached_dagger_attack_plan(782,rank);
        check(lead.required_type==32&&lead.windup_ms==250);
        const auto first=prepare_detached_dagger_effects(lead,chain,qualified,strike,Kind::slashing,actor,state,condition);
        check(first&&first->weapon.damage==20&&first->transaction.target.health->current==480);
        check(first->transaction.conditions.has(conditions::Kind::bleeding,100));
        check(first->transaction.chain.stage==1&&chain.stage==0&&actor.health->current==500);
        const auto fox=*detached_dagger_attack_plan(780,rank,0,7);
        check(fox.required_type==32&&fox.windup_ms==250);
        const auto second=prepare_detached_dagger_effects(fox,first->transaction.chain,qualified,strike,Kind::slashing,actor,state,condition);
        check(second&&second->weapon.damage==20+interpolate(10,35,rank));
        check(second->transaction.chain.stage==2&&!second->transaction.condition_applied);
        auto weak=strike;weak.weakness=true;
        const auto weak_hit=detached_dagger_skill_hit(fox,weak);
        check(weak_hit&&std::abs(weak_hit->damage-(6.8+fox.bonus))<1e-9);
        auto bad=fox;bad.arrows=2;check(!detached_dagger_skill_hit(bad,strike));
    }
    const auto fox=*detached_dagger_attack_plan(780,0);DaggerChain chain;chain.lead(target,90);
    auto guarded=actor;guarded.incoming_reduction=5;
    const auto reduced=prepare_detached_dagger_effects(fox,chain,qualified,strike,Kind::slashing,guarded,{},condition);
    check(reduced&&reduced->transaction.damage.damage==25);
    auto elemental=actor;elemental.elemental_protection=Kind::fire;elemental.elemental_multiplier=.5;
    const auto fire=prepare_detached_dagger_effects(fox,chain,qualified,strike,Kind::fire,elemental,{},condition);
    check(fire&&fire->transaction.damage.damage==15);
    auto blind=strike;blind.blind=true;blind.miss_roll=0;
    check(!detached_dagger_skill_hit(fox,blind));
    check(!prepare_detached_dagger_effects(fox,chain,qualified,blind,Kind::slashing,actor,{},condition));
    check(!detached_dagger_attack_plan(775,0)&&!detached_dagger_attack_plan(782,21));
    check(!detached_dagger_attack_plan(782,0,21)&&!detached_dagger_attack_plan(782,0,0,65536));
    auto critical=strike;critical.critical=true;
    const auto critical_hit=detached_dagger_skill_hit(fox,critical);check(critical_hit&&critical_hit->critical&&critical_hit->critical_energy);
    const auto strong=*detached_dagger_attack_plan(780,0,20);
    const auto stronger=detached_dagger_skill_hit(strong,strike),ordinary=detached_dagger_skill_hit(fox,strike);
    check(stronger&&ordinary&&stronger->damage>ordinary->damage);
    std::cout<<"PASS detached full Jagged782/Fox780 weapon, protection, condition and chain effects; World integration pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
