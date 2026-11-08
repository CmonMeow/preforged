#pragma once
#include "gw_effect_registry.h"
#include "gw_skillbar.h"
#include <array>
#include <map>

namespace fw::gw::skills::effects {
struct DenyHexesPlan {
    unsigned skill=991,energy=5,recharge=12;
    Time activation=1000,aftercast=750;
    float range=1248;
    auto operator<=>(const DenyHexesPlan&)const=default;
};
inline std::optional<DenyHexesPlan> deny_hexes(){
    const auto* row=find_skill(991);
    if(!row||row->campaign!=2||row->profession!=3||row->attribute!=16||row->type!=5||
       row->energy_cost()!=5||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=12||
       row->caster_body_resource!=2077||row->target_body_resource!=1253)return {};
    return DenyHexesPlan{};
}
// Read a committed local recharge ledger, not a caller-invented preview.
// Reject bar changes and an uncommitted Deny Hexes timer: the wiki describes
// this skill itself contributing to the removal count while recharging.
inline std::optional<std::array<Time,8>> deny_hexes_committed_recharges(
        const Slots& cast_bar,const Slots& current_bar,Slot cast,
        const std::map<std::pair<unsigned,unsigned>,Time>& recharging,Time now){
    if(cast.skill!=991||cast_bar!=current_bar)return {};
    std::array<Time,8> result{};unsigned copies=0;
    for(unsigned index=0;index<cast_bar.size();++index){
        const auto slot=cast_bar[index];
        if(slot==cast)++copies;
        if(!slot.skill)continue;
        const auto deadline=recharging.find({slot.skill,slot.copy});
        if(deadline!=recharging.end()&&deadline->second>now)result[index]=deadline->second;
    }
    if(copies!=1)return {};
    for(unsigned index=0;index<cast_bar.size();++index)
        if(cast_bar[index]==cast&&result[index]<=now)return {};
    return result;
}
struct DenyHexesProposal {
    Scope source,target;Time at=0;
    unsigned recharging_divine_favor=0;
    std::vector<Entry> removed;
};
// Caller supplies authoritative per-slot recharge deadlines at cast completion.
// The casting slot is deliberately included if its recharge has begun.
// This only selects effects; World must end them through its shared authority.
inline std::optional<DenyHexesProposal> prepare_deny_hexes(
        const DenyHexesPlan& plan,Scope source,Scope target,const Registry& registry,
        const Slots& bar,const std::array<Time,8>& recharge_until,Time now){
    if(!deny_hexes()||plan!=*deny_hexes()||!source.instance||!source.actor||source.actor>65535||!source.generation||
       !target.instance||!target.actor||target.actor>65535||!target.generation||
       source.instance!=target.instance||!registry.matches(target))return {};
    DenyHexesProposal result{source,target,now};
    for(unsigned slot=0;slot<bar.size();++slot){
        if(!bar[slot].skill||recharge_until[slot]<=now)continue;
        const auto* row=find_skill(bar[slot].skill);
        if(!row)return {};
        if(row->profession==3&&row->attribute==16)++result.recharging_divine_favor;
    }
    auto probe=registry;
    const auto limit=1u+result.recharging_divine_favor;
    for(unsigned count=0;count<limit;++count){
        const auto hex=probe.newest(target,Category::hex,now);
        if(!hex)break;
        if(!probe.remove(target,hex->instance))return {};
        result.removed.push_back(*hex);
    }
    return result;
}
}
