#include "gw_skill_effect_plan.h"
#include "gw_empathy.h"
#include <iostream>
#include <stdexcept>
int main()try{
    using namespace fw::gw::skills::effects;
    auto check=[](bool v,const char* m){if(!v)throw std::runtime_error(m);};
    auto after=*caster_earth_aoe(174,12);
    check(after.count==1&&std::abs(base_damage(after,240,false)-85.2)<1e-9,"Aftershock nearby boundary rank12 retains fractional damage");
    check(std::abs(base_damage(after,240,true)-141.6)<1e-9,"Aftershock actual knockdown bonus retains fraction");
    check(base_damage(after,240.01f,true)==0,"Aftershock outside nearby");
    auto shock=*caster_earth_aoe(937,12);
    check(base_damage(shock,166,false)==153&&base_damage(shock,252,false)==102&&base_damage(shock,322,false)==51,"Shockwave concentric strikes");
    check(shock.strikes[0].condition==Condition::weakness&&shock.strikes[1].condition==Condition::cracked_armor&&shock.strikes[2].condition==Condition::blind,"Shockwave exact ordered conditions");
    check(shock.strikes[0].duration_seconds==8,"Shockwave rank12 duration");
    check(base_damage(shock,322.01f,false)==0&&!caster_earth_aoe(171,12)&&!caster_earth_aoe(174,21),"No unsupported skill or rank inference");
    const auto pve=*empathy(26,12),pvp=*empathy(3151,12);
    check(pve.damage==46&&pve.reduction==12&&pve.duration==13000,"Empathy PvE rank12 plan");
    check(pvp.damage==39&&pvp.reduction==0&&pvp.duration==13000,"Empathy PvP variant mixed with PvE");
    check(!empathy(26,21)&&!empathy(27),"Empathy unsupported identity/rank admitted");
    Scope target{1,42,1};Registry hexes(target);
    check(hexes.apply(target,{1,7,26,12,Category::hex,100,13100},Replace::same_skill).accepted,"Empathy fixture hex rejected");
    check(empathy_attack_reaction(hexes,target,100)==46&&empathy_attack_damage(hexes,target,13099,5)==0,"Empathy active reaction/reduction");
    check(!empathy_attack_reaction(hexes,target,13100)&&empathy_attack_damage(hexes,target,13100,5)==5,"Empathy expired before impact still reduced damage");
    auto stale=target;++stale.generation;check(!empathy_attack_reaction(hexes,stale,100),"Empathy leaked to reused actor");
    check(hexes.apply(target,{2,7,3151,12,Category::hex,100,13100},Replace::same_skill).accepted&&
        empathy_attack_reaction(hexes,target,100)==39&&empathy_attack_damage(hexes,target,100,5)==5,
        "Empathy aliases stacked or same-tick application order was lost");
    hexes.clear(target);check(!empathy_attack_reaction(hexes,target,100),"Removed Empathy still reacted");
    std::cout<<"Reviewed Earth AoE and Empathy plans passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
