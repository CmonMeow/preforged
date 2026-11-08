#include "gw_combat.h"
#include "gw_weapon_attack.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw::combat;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("effective armor line "+std::to_string(at.line()));
}
int main()try{
    for(int armor:{500,501,600,1000,1780}){
        const auto expected=100*std::exp2((60.-armor)/40.);
        const Damage spell{Kind::earth,100,20,174};const auto resolved=amount(spell,armor);
        check(resolved&&std::abs(*resolved-expected)<1e-12);
        weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=100;strike.location_armor=armor;strike.base_roll=.5;strike.miss_roll=.5;
        const auto attack=weapon::resolve(strike);check(attack&&!attack->missed&&std::abs(attack->damage-expected)<1e-12);
        Actor target{2,1,Health{1000,1000},armor,0,true};const auto hit=apply(target,spell,true);
        check(hit.reason==Reason::accepted&&!hit.killed&&std::abs(hit.damage-expected)<1e-12);
    }
    check(!amount({Kind::earth,100,20,174},std::nullopt));
    check(!amount({Kind::earth,100,20,174},-101));
    check(!amount({Kind::earth,100,20,174},maximum_effective_armor+1));
    const auto underflow=amount({Kind::earth,100,20,174},maximum_effective_armor);check(underflow&&*underflow==0);
    std::cout<<"PASS effective armor above500for spell/weapon resolution and actor damage;unknown/negative armor guards retained\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
