#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct ConvertHexesPlan {
    unsigned rank=0,energy=15,armor_bonus=10;
    Time duration=8000,activation=1000,aftercast=750,recharge=12000;
    float range=1248;
    auto operator<=>(const ConvertHexesPlan&)const=default;
};
inline std::optional<ConvertHexesPlan> convert_hexes(unsigned rank){
    const auto* row=find_skill(303);
    if(rank>20||!row||row->profession!=3||row->attribute!=15||row->type!=5||row->special||
       row->energy_cost()!=15||row->adrenaline||row->health_sacrifice||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=12||
       row->scale0!=10||row->scale15!=10||row->duration0!=8||row->duration15!=20||row->caster_body_resource!=2077||row->target_body_resource!=538)return {};
    return ConvertHexesPlan{rank,15,10,Time(interpolate(8,20,rank))*1000};
}
struct ConvertHexesProposal {
    Scope source,target;Time at=0,armor_until=0;
    std::vector<Entry> removed;
    unsigned necromancer_hexes=0,armor=0;
};
struct ConvertHexesArmor {
    Scope source,target;unsigned effect=0,rank=0,necromancer_hexes=0;
    Time begins=0,ends=0;
};
inline unsigned active_convert_hexes_armor(const Registry& registry,Scope target,
        const std::optional<ConvertHexesArmor>& owned,Time now){
    if(!owned||owned->target!=target||owned->source.instance!=target.instance||!owned->necromancer_hexes||
       owned->necromancer_hexes>128||now<owned->begins||now>=owned->ends)return 0;
    const auto entry=registry.lookup(target,owned->effect);
    if(!entry||entry->skill!=303||entry->cause!=owned->source.actor||entry->rank!=owned->rank||
       entry->kind!=Category::other||entry->applied!=owned->begins||entry->expires!=owned->ends)return 0;
    // Retail bug documented by the bundled wiki: one or more Necromancer
    // hexes grant only +10, despite the per-hex wording in the description.
    return 10;
}
// Selection/armor entitlement only. World must preflight and end each actual
// owned effect through its shared removal authority (timers,credit,callbacks),
// not swap a copied registry over unrelated state. No Health is healed here.
inline std::optional<ConvertHexesProposal> prepare_convert_hexes(const ConvertHexesPlan& plan,
        Scope source,Scope target,const Registry& registry,Time now){
    const auto canonical=convert_hexes(plan.rank);
    const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
    if(!canonical||*canonical!=plan||!valid(source)||!valid(target)||source.instance!=target.instance||
       source.actor==target.actor||!registry.matches(target)||now>UINT64_MAX-plan.duration)return {};
    ConvertHexesProposal result{source,target,now,now+plan.duration};auto probe=registry;
    while(const auto hex=probe.newest(target,Category::hex,now)){
        const auto* row=find_skill(hex->skill);if(!row||row->type!=4)return {};
        if(!probe.remove(target,hex->instance))return {};
        result.removed.push_back(*hex);if(row->profession==4)++result.necromancer_hexes;
    }
    result.armor=result.necromancer_hexes?plan.armor_bonus:0;
    return result;
}
}
