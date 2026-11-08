#pragma once
#include "gw_berserker_stance.h"
#include "gw_battle_rage.h"
namespace fw::gw::skills::effects {
struct JusticePlan {unsigned skill=343;Time duration=20000;bool pvp=false;bool operator==(const JusticePlan&)const=default;};
inline std::optional<JusticePlan> for_great_justice(unsigned skill=343,bool pvp=false){
 if(skill!=343&&skill!=2883)return {};pvp|=skill==2883;const auto id=pvp?2883u:343u;const auto* r=find_skill(id);
 if(!r||r->type!=15||r->attribute!=51||r->energy_cost()!=5||r->activation!=0||r->aftercast!=0||
    r->recharge!=45||r->duration0!=(pvp?8u:20u)||r->duration15!=r->duration0||
    r->scale0!=(pvp?1u:100u)||r->scale15!=r->scale0||r->caster_body_resource!=593||r->target_body_resource!=2077)return {};
 return JusticePlan{id,pvp?8000u:20000u,pvp};
}
inline bool justice_active(const Registry& registry,Scope owner,bool pvp,Time now){
 const auto entry=registry.newest_skill(owner,pvp?2883u:343u,now);
 return entry&&entry->cause==owner.actor&&entry->kind==Category::other;
}
// Rates are half-native units per whole native unit. Ordinary multipliers
// share the +100% cap; fixed additional strikes remain distinct gain events.
// Registry queries avoid allocating an active-effect vector on every hit.
inline unsigned adrenaline_half_rate(const Registry& registry,Scope owner,Time now){
 return justice_active(registry,owner,false,now)?4u:active_berserker_stance(registry,owner,now)?3u:2u;
}
inline std::uint64_t attack_hit_adrenaline_halves(const Registry& registry,Scope owner,Time now){
 // Battle Rage modifies actual attack-hit gain, not damage received or flat
 // skill rewards. Its +100% shares the ordinary cap with PvE Justice/Berserker.
 const auto rate=active_battle_rage(registry,owner,now)?4u:adrenaline_half_rate(registry,owner,now);
 return 25ull*rate*(justice_active(registry,owner,true,now)?2u:1u);
}
}
