#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include <span>
namespace fw::gw::skills::effects {
struct DwarvenBattlePlan {
 unsigned rank=0,armor=40;Time duration=5000;unsigned attack_speed_percent=33;
 bool operator==(const DwarvenBattlePlan&)const=default;
};
inline std::optional<DwarvenBattlePlan> dwarven_battle_stance(unsigned rank){
 const auto* r=find_skill(375);
 if(rank>20||!r||r->type!=3||!r->elite()||r->attribute!=17||r->energy_cost()!=5||r->adrenaline||
    r->activation!=0||r->aftercast!=0||r->recharge!=20||r->duration0!=5||r->duration15!=11||
    r->scale0!=33||r->scale15!=33||r->bonus0!=40||r->bonus15!=40||
    r->caster_body_resource!=634||r->target_body_resource!=2077)return {};
 return DwarvenBattlePlan{rank,40,1000ull*interpolate(5,11,rank),33};
}
struct DwarvenBattleBonuses {bool haste=false;unsigned armor=0;bool operator==(const DwarvenBattleBonuses&)const=default;};
// null means weapon authority unknown. false means authoritatively unarmed or
// wielding another weapon;this must differ from never receiving a refresh.
inline std::optional<DwarvenBattleBonuses> dwarven_battle_bonuses(std::span<const Entry> entries,
 std::optional<bool> hammer,Time now){
 bool active=false;
 for(const auto& e:entries)if(e.skill==375&&e.kind==Category::stance&&e.instance&&e.cause&&e.applied<=now&&e.expires&&now<*e.expires){
  if(!dwarven_battle_stance(e.rank))return {};active=true;
 }
 if(!active)return DwarvenBattleBonuses{};
 if(!hammer)return {};
 return DwarvenBattleBonuses{*hammer,*hammer?40u:0u};
}
inline std::optional<bool> dwarven_attack_interrupt(std::span<const Entry> entries,
 std::optional<bool> hammer,unsigned skill,Time now){
 const auto bonus=dwarven_battle_bonuses(entries,hammer,now);if(!bonus)return {};
 if(!skill)return false;
 const auto* row=find_skill(skill);if(!row)return {};
 return bonus->haste&&row->type==14;
}
}
