#include "gw_elemental_damage.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
using namespace skills::effects;
static void check(bool value){if(!value)throw std::runtime_error("Spirit Burn mechanic");}
int main()try{
    check(!elemental(919,21));
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=*elemental(919,rank);
        check(plan.damage==damage_scale(5,50,rank)&&plan.burning_ms==1000ull*interpolate(1,5,rank));
        combat::Actor victim{};victim.id=2;victim.generation=1;
        victim.health=combat::Health{500,500};victim.damageable=true;victim.lightning_armor=60;
        const auto hit=[&](SpiritProximity spirit,bool allow=true){return elemental_target(plan,victim,{},20,1,100,false,1,allow,{}, {},spirit);};
        auto present=hit(SpiritProximity::present),absent=hit(SpiritProximity::absent);
        check(present.accepted&&absent.accepted&&present.hits.size()==1&&absent.hits.size()==1);
        check(present.hits[0].applied==absent.hits[0].applied&&present.hits[0].applied==plan.damage);
        check(present.additions==std::vector{combat::conditions::Kind::burning}&&absent.additions.empty());
        auto burn=present.conditions.lookup(combat::conditions::Kind::burning);
        check(burn&&burn->cause==1&&burn->applied==100&&burn->expires==100+plan.burning_ms);
        check(!present.conditions.has(combat::conditions::Kind::burning,burn->expires));
        check(!hit(SpiritProximity::unknown).accepted);
        check(hit(SpiritProximity::present,false).additions.empty());
        victim.health=combat::Health{1,500};check(hit(SpiritProximity::present).additions.empty());
        victim.health=combat::Health{500,500};victim.lightning_armor.reset();check(!hit(SpiritProximity::present).accepted);
        auto forged=plan;++forged.burning_ms;check(!valid_elemental(forged));
    }
    std::cout<<"PASS Spirit Burn rank0..20 lightning, spirit census, Burning duration, immunity, lethal and unknown armor gates\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
