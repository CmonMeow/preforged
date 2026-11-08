#include "gw_npc_combat_catalog.h"
#include "gw_pet_combat.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace fw::gw;
void check(bool v){if(!v)throw std::runtime_error("owned pet NPC modifier failed");}
void near(double a,double b){check(std::abs(a-b)<1e-8);}
int main()try{
    combat::npcs::Definition d;d.binding={162,100,50,128406,0,0,13,5};
    d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{180,180};d.damageable=true;
    d.healing_allowed=true;d.healing_evidence="reviewed pet healing";
    d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=35;
    const combat::npcs::Lifetime life{1,1,4,162,100};const skills::effects::Scope owner{1,1,3};
    combat::npcs::LiveActor plain(d,life),pet(d,life);
    check(!plain.owned_stalker_owner(life));
    auto stale=life;++stale.actor_generation;
    check(!pet.enroll_owned_stalker(stale,d.binding,owner,123,true,100));
    check(pet.enroll_owned_stalker(life,d.binding,owner,123,true,100));
    check(pet.owned_stalker_owner(life)==std::optional(owner)&&!pet.owned_stalker_owner(stale));
    check(pet.packed_allegiance(life)==123&&pet.actor(life)->health->current==180);
    auto foreign=owner;++foreign.generation;check(!pet.enroll_owned_stalker(life,d.binding,foreign,123,true,100));
    auto direct=pet;auto before=plain;
    const auto a=direct.apply_opposed_owned_damage(life,{combat::Kind::armor_ignoring,100,20,194},101);
    const auto b=before.apply_opposed_owned_damage(life,{combat::Kind::armor_ignoring,100,20,194},101);
    check(a.reason==combat::Reason::accepted&&b.reason==combat::Reason::accepted);
    near(a.before.current-a.after.current,67);near(b.before.current-b.after.current,100);
    auto pvp=combat::npcs::LiveActor(d,life);
    check(pvp.enroll_owned_stalker(life,d.binding,owner,123,false,100));
    const auto pvp_hit=pvp.apply_opposed_owned_damage(life,{combat::Kind::armor_ignoring,100,20,194},101);
    check(pvp_hit.reason==combat::Reason::accepted);near(pvp_hit.before.current-pvp_hit.after.current,100);
    auto enemydef=d;enemydef.binding.actor=101;enemydef.binding.definition=101;
    enemydef.relation=combat::npcs::Relation::hostile_to_players;
    const combat::npcs::Lifetime enemy_life{1,1,8,162,101};combat::npcs::LiveActor enemy(enemydef,enemy_life);
    const auto weapon=pet.apply_opposed_npc_weapon_damage(life,enemy,enemy_life,100,2,101);
    check(bool(weapon));near(weapon->before.current-weapon->after.current,67);
    // Life steal has its own loss/reduction lane. Enrollment must not install
    // a flat steal reduction or change that separate policy multiplier.
    near(*pet.life_steal_reduction(life,102),0);
    for(auto kind:{pets::combat_model::Loss::life_steal,pets::combat_model::Loss::health_loss,pets::combat_model::Loss::degeneration})
        near(*pets::combat_model::incoming_multiplier({pet.hex_scope(),owner,true},pet.hex_scope(),kind),1);
    combat::npcs::LiveActor replacement(d,{1,1,5,162,100});
    check(!replacement.owned_stalker_owner(replacement.lifetime()));
    near(replacement.actor(replacement.lifetime())->incoming_damage_multiplier,1);
    std::cout<<"Owned Stalker direct/spell/weapon scope modifiers and non-damage exclusions passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
