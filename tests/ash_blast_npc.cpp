#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
using namespace combat::npcs;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Ash Blast NPC line "+std::to_string(at.line()));
}
Definition profile(bool armor=true,unsigned health=500){
    Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
    d.relation=Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),500};
    d.damageable=d.conditions_allowed=true;
    if(armor)d.earth_armor=60;
    return d;
}
int main()try{
    const Lifetime life{1,2,3,146,100};
    const skills::effects::Scope caster{1,1,1};
    const auto plan=*skills::effects::ash_blast(0);
    {
        LiveActor split(profile(),life);
        check(split.stage_shield_absorption(life,12,1,77,100));
        check(split.apply_condition(life,combat::conditions::Kind::burning,1,200,3000));
        const auto damage=split.apply_damage(life,{combat::Kind::earth,35,20,1085},200);
        check(damage.reason==combat::Reason::accepted&&damage.applied==30);
        check(split.remove_condition(life,combat::conditions::Kind::burning,200));
        // A reaction removed Burning after its impact-time qualification.
        const auto hex=split.apply_ash_blast_hex(life,plan,caster,80,200,true);
        check(hex&&hex->accepted&&split.actor(life)->health->current==470);
        check(split.hex_effect(life,80)&&split.ash_blast_miss_chance(life,200)==.20);
        const auto second=split.apply_damage(life,{combat::Kind::earth,35,20,1085},201);
        check(second.reason==combat::Reason::accepted&&second.applied==25);
        check(split.actor(life)->health->current==445); // Hex did not consume a damage step.
        check(!split.apply_ash_blast_hex(life,plan,caster,81,201,true));
        check(bool(split.advance_conditions(life,201)));
        const auto replacement=split.apply_ash_blast_hex(life,plan,caster,81,201,true);
        check(replacement&&replacement->removed.size()==1&&replacement->removed[0].instance==80);
        check(!split.hex_effect(life,80)&&split.hex_effect(life,81));
        auto stale=life;++stale.actor_generation;
        auto forged=plan;++forged.damage;
        check(!split.apply_ash_blast_hex(stale,plan,caster,82,201,true));
        check(!split.apply_ash_blast_hex(life,forged,caster,82,201,true));
        check(!split.apply_ash_blast_hex(life,plan,caster,82,200,true));
        check(!split.apply_ash_blast_hex(life,plan,caster,82,202,true));
        check(!split.apply_ash_blast_hex(life,plan,caster,82,201,false));
        check(!split.apply_ash_blast_hex(life,plan,{9,1,1},82,201,true));
        check(!split.apply_ash_blast_hex(life,plan,caster,81,201,true));
        check(split.actor(life)->health->current==445&&split.hex_effect(life,81));
    }
    LiveActor plain(profile(),life);
    const auto hit=plain.apply_ash_blast(life,plan,caster,1,20,0,200);
    check(hit&&hit->hit.applied==35&&!hit->hex);
    check(plain.actor(life)->health->current==465&&!plain.hex_effect(life,1));
    LiveActor burning(profile(),life);
    check(burning.apply_condition(life,combat::conditions::Kind::burning,1,200,3000));
    const auto conditional=burning.apply_ash_blast(life,plan,caster,2,20,0,200);
    check(conditional&&conditional->hit.applied==35&&conditional->hex);
    check(burning.hex_effect(life,2)&&burning.hex_effect(life,2)->skill==1085);
    check(burning.ash_blast_miss_chance(life,200)==.20);
    check(burning.ash_blast_miss_chance(life,5200)==0.);
    check(burning.actor(life)->health->current==465);
    auto high_armor=profile();high_armor.earth_armor=80;
    LiveActor cracked(high_armor,life),ordinary(high_armor,life),uncracked(high_armor,life);
    check(cracked.apply_condition(life,combat::conditions::Kind::cracked_armor,1,200,3000));
    check(ordinary.apply_condition(life,combat::conditions::Kind::cracked_armor,1,200,3000));
    const auto ordinary_hit=ordinary.apply_damage(life,
        {combat::Kind::earth,double(plan.damage),20,1085},200);
    const auto cracked_hit=cracked.apply_ash_blast(life,plan,caster,6,20,0,200);
    check(cracked_hit&&ordinary_hit.reason==combat::Reason::accepted);
    const auto uncracked_hit=uncracked.apply_ash_blast(life,plan,caster,7,20,0,200);
    check(bool(uncracked_hit));
    check(cracked_hit->hit.applied==ordinary_hit.applied&&
        cracked_hit->hit.applied>uncracked_hit->hit.applied);
    LiveActor unknown(profile(false),life);
    check(!unknown.apply_ash_blast(life,plan,caster,3,20,0,200));
    check(unknown.actor(life)->health->current==500);
    check(!burning.apply_ash_blast(life,plan,{9,1,1},4,20,0,200));
    check(!burning.apply_ash_blast(life,plan,caster,4,20,plan.radius+1,200));
    LiveActor lethal(profile(true,10),life);
    const auto killed=lethal.apply_ash_blast(life,plan,caster,5,20,0,200);
    check(killed&&killed->hit.killed&&!killed->hex&&lethal.actor(life)->health->current==0);
    std::cout<<"Ash Blast NPC damage, conditional hex, rejection and death pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
