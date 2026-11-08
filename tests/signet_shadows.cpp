#include "gw_signet_shadows.h"
#include <fstream>
#include <iostream>
#include <source_location>
#include <string>
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Signet Shadows line "+std::to_string(at.line()));
}
int main(){
    using namespace fw::gw;using namespace skills::effects;
    using combat::conditions::Kind;using combat::conditions::State;
    for(unsigned rank=0;rank<=20;++rank){
        const auto p=*signet_shadows(rank);
        check(p.damage==5+2*rank&&p.blind_bonus==15+3*rank);
        check(p.activation==1000&&p.aftercast==750&&p.recharge==30000);
        combat::Actor actor{2,7,combat::Health{480,480},{},2,true};
        State blind;check(blind.apply(Kind::blind,1,100,1000));
        const auto ordinary=signet_shadows_target(p,actor,{},20,true,500);
        const auto bonus=signet_shadows_target(p,actor,blind,20,true,500);
        check(ordinary&&bonus&&!ordinary->blinded&&bonus->blinded);
        check(ordinary->hit.damage==p.damage&&bonus->hit.damage==p.damage+p.blind_bonus);
        check(actor.health->current==480&&blind.has(Kind::blind,500));
        check(signet_shadows_target(p,actor,blind,20,true,99)->hit.damage==p.damage);
        check(signet_shadows_target(p,actor,blind,20,true,100)->blinded);
        check(!signet_shadows_target(p,actor,blind,20,true,1100)->blinded);
        check(blind.remove(Kind::blind));
        check(!signet_shadows_target(p,actor,blind,20,true,500)->blinded);
        for(unsigned condition=0;condition<unsigned(Kind::count);++condition){
            State other;check(other.apply(Kind(condition),1,100,1000));
            check(signet_shadows_target(p,actor,other,20,true,500)->blinded==(Kind(condition)==Kind::blind));
        }
        actor.incoming_damage_multiplier=2;actor.incoming_reduction=3;
        check(signet_shadows_target(p,actor,{},20,true,500)->hit.damage==2*p.damage-3);
        actor.incoming_damage_cap=4;
        check(signet_shadows_target(p,actor,{},20,true,500)->hit.damage==1);
        actor.incoming_damage_cap.reset();actor.incoming_reduction=0;actor.incoming_damage_multiplier=1;
        actor.health->current=1;
        check(signet_shadows_target(p,actor,{},20,true,500)->hit.killed);
        check(!signet_shadows_target(p,actor,{},20,false,500));
        actor.health.reset();check(!signet_shadows_target(p,actor,{},20,true,500));
        actor.health=combat::Health{0,480};check(!signet_shadows_target(p,actor,{},20,true,500));
        actor.health=combat::Health{480,480};actor.damageable=false;
        check(!signet_shadows_target(p,actor,{},20,true,500));actor.damageable=true;
        auto forged=p;++forged.blind_bonus;check(!signet_shadows_target(forged,actor,{},20,true,500));
        forged=p;forged.range=9999;check(!signet_shadows_target(forged,actor,{},20,true,500));
        check(!signet_shadows_target(p,actor,{},0,true,500));
    }
    check(!signet_shadows(21));
    auto packet=signet_shadows_impact_2022(1,2);packet[1]|=0x80;
    std::ifstream fixture(FIXTURE_FILE);std::string line;check(bool(std::getline(fixture,line)));
    std::string actual;constexpr char hex[]="0123456789abcdef";
    for(auto byte:packet){actual+=hex[byte>>4];actual+=hex[byte&15];}check(actual==line);
    for(auto actors:{std::pair{0u,2u},{1u,0u},{1u,1u},{65536u,2u},{1u,65536u}}){
        bool threw=false;try{signet_shadows_impact_2022(actors.first,actors.second);}catch(const std::invalid_argument&){threw=true;}check(threw);
    }
    std::cout<<"PASS Signet of Shadows 21 ranks, exact Blind timing, other conditions, armor bypass/protection, copy-only rejection and native impact wire\n";
}
