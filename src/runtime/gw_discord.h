#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
namespace fw::gw::skills::effects {
struct Discord {unsigned skill=817,rank=0;double damage=30;auto operator<=>(const Discord&)const=default;};
inline std::optional<Discord> discord(unsigned rank,bool pvp=false){
 const unsigned id=pvp?2863:817;const auto* r=find_skill(id);
 if(rank>20||!r||r->profession!=4||r->attribute!=5||r->type!=5||r->special!=(pvp?4194308u:4u)||r->energy_cost()!=5||
    r->activation!=(pvp?2.f:1.f)||r->aftercast!=.75f||r->recharge!=2||r->pvp_skill!=(pvp?817u:2863u)||
    r->scale0!=30||r->scale15!=110||r->caster_body_resource!=2077||r->target_body_resource!=838)return {};
 return Discord{id,rank,damage_scale(30,110,rank)};
}
// Three-valued facts: no visual mask is promoted into an authoritative state.
inline std::optional<bool> discord_ready(std::optional<bool> condition,std::optional<bool> hex,std::optional<bool> enchantment){
 if(condition&&!*condition)return false;
 if((hex&&*hex)||(enchantment&&*enchantment))return condition;
 if(hex&&enchantment)return false;
 return {};
}
inline std::optional<combat::Damage> discord_damage(const Discord& plan,unsigned level){
 const auto expected=discord(plan.rank,plan.skill==2863);
 if(!expected||*expected!=plan)return {};
 combat::Damage damage{combat::Kind::armor_ignoring,plan.damage,level,plan.skill};
 return combat::valid(damage)?std::optional(damage):std::nullopt;
}
}
