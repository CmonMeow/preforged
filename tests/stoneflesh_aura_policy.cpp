#include "gw_stoneflesh_aura.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
using namespace skills::effects;
static void check(bool ok){if(!ok)throw std::runtime_error("Stoneflesh policy");}
int main()try{
    for(unsigned rank=0;rank<=20;++rank){
        auto p=stoneflesh_aura(rank);check(bool(p));
        check(p->reduction==1+2*rank);
        check(p->duration==1000ull*interpolate(5,15,rank));
    }
    check(!stoneflesh_aura(21));
    const Scope owner{1,25,1},stale{1,25,2};Registry baseline(owner);
    const auto plan=*stoneflesh_aura(15);
    check(!prepare_stoneflesh_aura(baseline,stale,69,plan,100));
    check(!prepare_stoneflesh_aura(baseline,owner,0,plan,100));
    check(!prepare_stoneflesh_aura(baseline,owner,69,plan,UINT64_MAX));
    auto first=prepare_stoneflesh_aura(baseline,owner,69,plan,100);
    check(bool(first)&&first->replaced.empty());
    check(!baseline.newest_skill(owner,1375,100));
    check(bool(first->registry.newest_skill(owner,1375,15099)));
    check(!first->registry.newest_skill(owner,1375,15100));
    auto next=prepare_stoneflesh_aura(first->registry,owner,70,plan,500);
    check(bool(next)&&next->replaced.size()==1&&next->replaced[0].instance==69);
    combat::Actor actor;actor.id=25;actor.health=combat::Health{500,500};
    actor.damageable=true;actor.earth_armor=60;actor.physical_armor=60;
    actor.incoming_reduction=4;
    check(apply_stoneflesh_damage(actor,plan));
    check(actor.incoming_reduction==35&&actor.life_steal_reduction==4);
    for(auto kind:{combat::Kind::earth,combat::Kind::armor_ignoring,combat::Kind::slashing}){
        auto copy=actor;auto hit=combat::apply(copy,{kind,100,20,1375},true);
        check(hit.reason==combat::Reason::accepted&&hit.damage==65);
    }
    auto copy=actor;check(combat::apply(copy,{combat::Kind::armor_ignoring,10,20,1375},true).damage==0);
    combat::weapon::Strike strike;strike.level=20;strike.mastery=12;
    strike.minimum=15;strike.maximum=22;strike.location_armor=60;
    strike.base_roll=.5;strike.miss_roll=.5;strike.critical=true;
    const auto ordinary_critical=combat::weapon::resolve(strike);check(bool(ordinary_critical));
    check(apply_stoneflesh_critical(strike,plan));
    const auto protected_critical=combat::weapon::resolve(strike);
    check(bool(protected_critical)&&protected_critical->critical&&!protected_critical->critical_energy);
    strike.critical=false;const auto normal=combat::weapon::resolve(strike);
    check(normal->damage==protected_critical->damage&&ordinary_critical->damage>normal->damage);
    auto forged=plan;forged.reduction=1000;
    check(!apply_stoneflesh_damage(actor,forged)&&!apply_stoneflesh_critical(strike,forged));
    std::cout<<"PASS Stoneflesh policy, damage and preserved critical events\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
