#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include "gw_lightning_bolt.h"
#include "gw_shock_arrow.h"
#include <compare>
namespace fw::gw::skills::effects {
struct LightningPlan {
    unsigned skill=0,rank=0;double damage=0;float range=1248.f,radius=0;
    double secondary_damage=0;unsigned max_secondary=0;
    unsigned penetration=25;bool projectile=false;
    std::uint64_t weakness_ms=0,blind_ms=0,cracked_ms=0;
    unsigned storage=0,energy_if_cracked=0;
    auto operator<=>(const LightningPlan&)const=default;
};
// Reviewed local pre-Reforged descriptions and pinned native rows. Conditional
// area membership and222's owned expiry belong to the authoritative World.
inline std::optional<LightningPlan> lightning(unsigned skill,unsigned rank=0,unsigned storage=0){
    if(rank>20||storage>20)return {};LightningPlan p;p.skill=skill;p.rank=rank;
    if(skill==205){p.damage=damage_scale(14,100,rank);p.cracked_ms=std::uint64_t(interpolate(5,20,rank))*1000;}
    else if(skill==222){p.damage=damage_scale(5,50,rank);}
    else if(skill==226||skill==2804){p.damage=damage_scale(10,50,rank);}
    else if(skill==228){p.damage=damage_scale(10,50,rank);p.radius=166;
        p.cracked_ms=p.weakness_ms=std::uint64_t(interpolate(5,20,rank))*1000;}
    else if(skill==224){p.damage=damage_scale(25,50,rank);p.weakness_ms=std::uint64_t(interpolate(5,20,rank))*1000;}
    else if(skill==1369){if(!LightningBoltImpact::begin(rank,false))return {};p.damage=damage_scale(5,50,rank);p.projectile=true;}
    else if(skill==1082){const auto shock=shock_arrow(rank,storage);if(!shock)return {};p.damage=shock->damage;p.projectile=true;p.storage=storage;p.energy_if_cracked=shock->energy_if_cracked;}
    else if(skill==229){p.damage=damage_scale(10,100,rank);p.projectile=true;}
    else if(skill==865){p.damage=damage_scale(10,100,rank);p.cracked_ms=std::uint64_t(interpolate(5,20,rank))*1000;}
    else if(skill==3396){p.damage=damage_scale(10,100,rank);} // PvP Hammer has no Cracked Armor.
    else if(skill==2059){p.damage=damage_scale(10,30,rank);p.cracked_ms=std::uint64_t(interpolate(5,20,rank))*1000;}
    else if(skill==842){const auto* row=find_skill(skill);
        if(!row||row->profession!=6||row->attribute!=8||row->type!=5||row->energy_cost()!=5||
           row->activation!=1.5f||row->aftercast!=.75f||row->recharge!=8||
           row->scale0!=5||row->scale15!=40||row->bonus0!=15||row->bonus15!=85||
           row->caster_body_resource!=2077||row->target_body_resource!=988)return {};
        p.damage=damage_scale(5,40,rank);p.secondary_damage=damage_scale(15,85,rank);
        p.radius=252;p.max_secondary=2;}
    else if(skill==1664){const auto* row=find_skill(skill);
        if(!row||row->campaign!=3||row->profession!=6||row->attribute!=8||
           row->type!=5||row->energy_cost()!=10||row->activation!=1.f||
           row->aftercast!=.75f||row->recharge!=6||row->overcast!=5||
           row->scale0!=10||row->scale15!=90||row->caster_body_resource!=2077||
           row->target_body_resource!=1640)return {};
        p.damage=damage_scale(10,90,rank);p.secondary_damage=p.damage;
        p.radius=322;p.max_secondary=2;}
    else if(skill==232){p.damage=damage_scale(10,60,rank);p.range=144;p.radius=166;
        p.blind_ms=std::uint64_t(interpolate(1,4,rank))*1000;p.cracked_ms=std::uint64_t(interpolate(1,10,rank))*1000;}
    else return {};return p;
}
inline bool valid_lightning(const LightningPlan& p){const auto expected=lightning(p.skill,p.rank,p.storage);return expected&&*expected==p;}
struct LightningResult {
    bool accepted=false;combat::Actor actor;combat::conditions::State conditions;
    combat::Hit hit;std::vector<combat::conditions::Kind> applied_conditions;
    std::optional<combat::DamageStep> projection;
};
inline LightningResult lightning_target(const LightningPlan& plan,combat::Actor actor,
        combat::conditions::State conditions,unsigned level,std::uint32_t caster,std::uint64_t now,
        std::optional<double> secondary_damage={},const combat::DamageProjection& projection={}){
    if(!valid_lightning(plan)||!caster||!actor.lightning_armor||
       (secondary_damage&&((plan.skill!=842&&plan.skill!=1664)||
                           *secondary_damage!=plan.secondary_damage)))return {};
    actor.lightning_armor=conditions.armor(*actor.lightning_armor,now);
    const auto step=combat::project_damage(actor,{combat::Kind::lightning,secondary_damage.value_or(plan.damage),level,plan.skill,plan.penetration},projection);
    if(!step)return {};
    LightningResult result{true,actor,conditions,step->incoming,{}};
    if(projection)result.projection=*step;
    if(actor.health->current<=0)return result;
    const std::array<std::pair<combat::conditions::Kind,std::uint64_t>,3> additions{{
        {combat::conditions::Kind::weakness,plan.weakness_ms},{combat::conditions::Kind::blind,plan.blind_ms},
        {combat::conditions::Kind::cracked_armor,plan.cracked_ms}}};
    auto ordered=additions;
    if(plan.skill==228)std::swap(ordered[0],ordered[2]); // Weakness covers Cracked Armor.
    for(auto [kind,duration]:ordered)if(duration){
        if(!result.conditions.apply(kind,caster,now,duration))return {};
        result.applied_conditions.push_back(kind);
    }
    return result;
}
}
