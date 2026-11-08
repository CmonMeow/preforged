#pragma once
#include "gw_effect_registry.h"
#include "gw_player_skills_generated.h"
#include <algorithm>
#include <cmath>
namespace fw::gw::skills::effects {
struct InspiredHexPlan {
 unsigned skill=22,rank=0,energy=4;Time copy_duration=20000;float range=1248;
 bool allied=true;Category removed_category=Category::hex;
 auto operator<=>(const InspiredHexPlan&)const=default;
};
inline std::optional<InspiredHexPlan> inspired_hex(unsigned skill,unsigned rank){
 const auto* row=find_skill(skill);
 if(skill==21||skill==1048){
  if(rank>20||!row||row->type!=5||row->profession!=5||row->attribute!=3||row->energy_cost()!=10||
     row->activation!=1.f||row->aftercast!=.75f||row->recharge||row->scale0!=3||row->scale15!=15||
     row->duration0!=20||row->duration15!=20||row->caster_body_resource!=37||row->target_body_resource!=38)return {};
  return InspiredHexPlan{skill,rank,static_cast<unsigned>(std::lround(3.+12.*rank/15.)),20000,1248,false,Category::enchantment};
 }
 if((skill!=22&&skill!=1049)||rank>20||!row||row->type!=5||row->attribute!=3||
    row->scale0!=4||row->scale15!=10||row->duration0!=20||row->duration15!=20)return {};
 return InspiredHexPlan{skill,rank,static_cast<unsigned>(std::lround(4.+6.*rank/15.)),20000,1248};
}
enum class HexCopyKind {none,player,player_pvp_alias,monster_or_unobtainable};
struct HexCopyCandidate {HexCopyKind kind=HexCopyKind::none;unsigned actual=0,base=0;};
inline HexCopyCandidate classify_effect_copy(unsigned skill,Category kind){
 const auto* row=find_skill(skill);if(!row||(kind!=Category::hex&&kind!=Category::enchantment)||
    row->type!=(kind==Category::hex?4:6))return {};
 if(std::binary_search(obtainable_player_skills.begin(),obtainable_player_skills.end(),skill))return {HexCopyKind::player,skill,skill};
 for(unsigned base:obtainable_player_skills)if(find_skill(base)->pvp_skill==skill)return {HexCopyKind::player_pvp_alias,skill,base};
 return {HexCopyKind::monster_or_unobtainable,skill,0};
}
inline HexCopyCandidate classify_hex_copy(unsigned skill){return classify_effect_copy(skill,Category::hex);}
struct InspiredHexProposal {
 Registry registry;std::optional<Entry> removed;
 unsigned energy=0;HexCopyCandidate copy;
};
struct InspiredHexOffer {
 Scope caster,ally;InspiredHexPlan plan;Time at=0;
 std::uint64_t caster_epoch=0,ally_epoch=0;
 std::optional<Entry> hex;HexCopyCandidate copy;
};
struct InspiredHexWorldResult {
 std::optional<Entry> removed;HexCopyCandidate copy;
 unsigned energy_repaid=0,energy_restored=0;
 Scope caster,ally;Time at=0;unsigned source_skill=0;
};
// Only the removal/energy/copy candidate is planned here. World must execute
// removal end effects and stage debt-aware Energy gain with the temporary bar.
// A PvP alias is explicitly classified; this does not select its wire slot ID.
inline std::optional<InspiredHexProposal> prepare_inspired_hex(const Registry& original,
 Scope caster,Scope ally,const InspiredHexPlan& plan,Time now){
 const auto canonical=inspired_hex(plan.skill,plan.rank);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||plan!=*canonical||!valid(caster)||!valid(ally)||caster.instance!=ally.instance||
    !original.matches(ally)||now>UINT64_MAX-plan.copy_duration)return {};
 auto registry=original;const auto hex=original.newest(ally,plan.removed_category,now);
 if(!hex)return InspiredHexProposal{std::move(registry),{},0,{}};
 if(!registry.remove(ally,hex->instance))return {};
 return InspiredHexProposal{std::move(registry),hex,plan.energy,classify_effect_copy(hex->skill,plan.removed_category)};
}
}
