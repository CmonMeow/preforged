#include "../src/runtime/gw_mind_burn.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
using namespace skills::effects;
void check(bool yes,const char* reason){if(!yes)throw std::runtime_error(reason);}
combat::Actor body(unsigned id=2,double health=1000){
    combat::Actor actor;actor.id=id;actor.generation=1;actor.health=combat::Health{health,1000};
    actor.damageable=true;actor.fire_armor=60;return actor;
}
int main()try{
    constexpr std::int64_t unit=300000;
    const auto plan=*mind_burn(185,12);check(plan.damage==51&&plan.burning_ms==8000&&plan.radius==166,"Mind Burn rank12 values");
    check(mind_burn(185,0)->burning_ms==1000&&mind_burn(185,15)->damage==60&&mind_burn(185,20)->burning_ms==13000,"Mind Burn rank endpoints");
    check(!mind_burn(185,21)&&!mind_burn(2803,12)&&!mind_burn(2804,12),"Mind Burn invented split or invalid rank");
    check(skills::find_skill(185)->pvp_skill==3433&&skills::find_skill(185)->overcast==5,"Mind Burn pinned mode/Overcast metadata");
    auto resolve=[&](std::int64_t caster,std::int64_t target,combat::Actor actor=body(),std::optional<bool> allowed=true){
        return mind_burn_target(plan,actor,{},20,1,caster,target,allowed,1000);
    };
    const auto tied=resolve(20*unit,20*unit);check(tied.accepted&&!tied.energy_advantage&&tied.hits.size()==1&&tied.actor.health->current==949&&!tied.burning_applied,"equal Energy incorrectly triggered bonus");
    const auto fractional=resolve(20*unit+1,20*unit);check(fractional.accepted&&fractional.energy_advantage&&fractional.hits.size()==2&&fractional.actor.health->current==898&&fractional.burning_applied,"strict fractional Energy advantage lost");
    check(fractional.conditions.has(combat::conditions::Kind::burning,8999)&&!fractional.conditions.has(combat::conditions::Kind::burning,9000)&&fractional.conditions.lookup(combat::conditions::Kind::burning)->cause==1,"burn timer/caster attribution");
    check(resolve(-1,-2).energy_advantage&&!resolve(-2,-1).energy_advantage,"signed Energy debt comparison");
    check(!mind_burn_target(plan,body(),{},20,1,{},0,true,1000).accepted&&
        !mind_burn_target(plan,body(),{},20,1,0,{},true,1000).accepted,"unknown Energy degraded to base hit");
    check(!resolve(1000000ll*unit+1,0).accepted&&!resolve(0,-1000000ll*unit-1).accepted,"Energy defensive bound");
    auto modified=plan;++modified.damage;check(!mind_burn_target(modified,body(),{},20,1,1,0,true,1000).accepted,"modified plan accepted");
    auto armorless=body();armorless.fire_armor.reset();check(!resolve(1,0,armorless).accepted,"unknown fire armor fabricated");
    auto reduced=body();reduced.incoming_reduction=5;
    const auto protected_hits=resolve(1,0,reduced);check(protected_hits.hits.size()==2&&protected_hits.hits[0].applied==46&&protected_hits.hits[1].applied==46,"two damage packets collapsed across flat reduction");
    const auto base_kill=resolve(1,0,body(2,30));check(base_kill.accepted&&base_kill.hits.size()==1&&base_kill.hits[0].killed&&!base_kill.burning_applied,"lethal base hit acquired bonus/burning");
    const auto bonus_kill=resolve(1,0,body(2,75));check(bonus_kill.accepted&&bonus_kill.hits.size()==2&&bonus_kill.hits[1].killed&&!bonus_kill.burning_applied,"lethal conditional hit acquired burning");
    const auto immune=resolve(1,0,body(),false);check(immune.accepted&&immune.hits.size()==2&&!immune.burning_applied,"condition immunity suppressed damage");
    check(!resolve(1,0,body(),{}).accepted&&resolve(0,0,body(),{}).accepted&&resolve(1,0,body(2,30),{}).accepted,"unknown condition admission not gated by actual surviving burning attempt");
    check(!mind_burn_target(plan,body(),{},20,1,1,0,true,UINT64_MAX-1000).accepted&&
        mind_burn_target(plan,body(),{},20,1,0,0,{},UINT64_MAX-1000).accepted,"burn deadline overflow handling");
    combat::conditions::State existing;check(existing.apply(combat::conditions::Kind::burning,55,0,30000),"burn refresh fixture");
    auto refreshed=mind_burn_target(plan,body(),existing,20,1,1,0,true,1000);
    check(refreshed.accepted&&refreshed.conditions.lookup(combat::conditions::Kind::burning)->expires==30000&&
        refreshed.conditions.lookup(combat::conditions::Kind::burning)->cause==55,"Mind Burn shortened/reassigned retained burning");
    combat::conditions::State cracked;check(cracked.apply(combat::conditions::Kind::cracked_armor,1,0,10000),"armor fixture");
    auto armored=body();armored.fire_armor=100;
    const auto normal=mind_burn_target(plan,armored,{},20,1,1,0,true,1000),crack=mind_burn_target(plan,armored,cracked,20,1,1,0,true,1000);
    check(crack.accepted&&crack.hits[0].damage>normal.hits[0].damage,"Cracked Armor omitted from fire damage");
    const Scope caster{1,1,1},primary{1,2,1};
    auto contact=[](unsigned id,float x,std::optional<std::int64_t> energy){return MindBurnContact{{1,id,1},body(id),{},energy,x,100,0,true,true};};
    std::vector<MindBurnContact> contacts{contact(2,100,20*unit),contact(3,266,10*unit),contact(4,266.01f,{})};
    auto other_plane=contact(5,100,{});other_plane.plane=1;contacts.push_back(other_plane);
    auto ally=contact(6,100,{});ally.hostile=false;contacts.push_back(ally);
    auto dead=contact(7,100,{});dead.actor.health->current=0;contacts.push_back(dead);
    const auto mixed=mind_burn_area(plan,caster,primary,20,15*unit,contacts,1000);
    check(mixed.accepted&&mixed.recipients.size()==2&&!mixed.recipients[0].result.energy_advantage&&
        mixed.recipients[1].result.energy_advantage&&mixed.recipients[0].result.hits.size()==1&&mixed.recipients[1].result.hits.size()==2,
        "adjacent Energy comparison copied primary predicate or166 boundary incorrect");
    check(!mixed.recipients[0].result.burning_applied&&mixed.recipients[1].result.burning_applied,"independent secondary burning predicate");
    auto reverse=contacts;reverse[0].energy=10*unit;reverse[1].energy=20*unit;
    const auto reverse_result=mind_burn_area(plan,caster,primary,20,15*unit,reverse,1000);
    check(reverse_result.accepted&&reverse_result.recipients[0].result.energy_advantage&&!reverse_result.recipients[1].result.energy_advantage,"primary advantage leaked into splash");
    auto missing=contacts;missing[1].energy.reset();const auto rejected=mind_burn_area(plan,caster,primary,20,15*unit,missing,1000);
    check(!rejected.accepted&&rejected.recipients.empty()&&contacts[0].actor.health->current==1000,"unknown secondary Energy partially committed area");
    auto unknown_armor=contacts;unknown_armor[1].actor.fire_armor.reset();check(!mind_burn_area(plan,caster,primary,20,15*unit,unknown_armor,1000).accepted,"unknown secondary fire armor partially accepted");
    auto duplicate=contacts;duplicate.push_back(duplicate[1]);check(!mind_burn_area(plan,caster,primary,20,15*unit,duplicate,1000).accepted,"duplicate secondary double hit");
    duplicate=contacts;duplicate.push_back(duplicate[0]);check(!mind_burn_area(plan,caster,primary,20,15*unit,duplicate,1000).accepted,"ambiguous primary accepted");
    auto stale=primary;++stale.generation;check(!mind_burn_area(plan,caster,stale,20,15*unit,contacts,1000).accepted,"recycled selected generation inherited area");
    auto mismatch=contacts;++mismatch[1].actor.generation;check(!mind_burn_area(plan,caster,primary,20,15*unit,mismatch,1000).accepted,"secondary actor/scope generation mismatch accepted");
    auto lethal=contacts;lethal[0].actor.health->current=30;lethal[0].energy=0;
    const auto lethal_area=mind_burn_area(plan,caster,primary,20,15*unit,lethal,1000);
    check(lethal_area.accepted&&lethal_area.recipients[0].result.hits.size()==1&&lethal_area.recipients[1].result.hits.size()==2,"primary death cancelled independently eligible splash");
    std::cout<<"PASS Mind Burn pinned plan, precise per-recipient Energy, two hits, burning, area scope/geometry/atomicity\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
