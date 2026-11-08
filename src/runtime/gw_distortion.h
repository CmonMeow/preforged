#pragma once
#include "gw_effect_registry.h"
#include <cmath>
namespace fw::gw::skills::effects {
struct DistortionPlan {
 unsigned rank=0,energy_loss=2;
 Time duration=1000;
 double block_chance=.75;
 auto operator<=>(const DistortionPlan&)const=default;
};
inline std::optional<DistortionPlan> distortion(unsigned rank){
 const auto* row=find_skill(11);
 if(rank>20||!row||row->type!=3||row->attribute!=1||row->scale0!=75||row->scale15!=75||
    row->bonus0!=2||row->bonus15!=2||row->duration0!=1||row->duration15!=5)return {};
 return DistortionPlan{rank,2,static_cast<Time>(std::lround(1.+4.*rank/15.))*1000,.75};
}
struct DistortionBlockProposal {
 Registry registry;
 unsigned stance_instance=0;
 bool ends=false;
 // Insufficient-Energy partial deduction is not established by our evidence.
 // Absence here requires explicit handling, never an implicit zero charge.
 std::optional<unsigned> remaining_energy;
};
// Call only for a block attributed to Distortion, not another block source,
// a miss, an unblockable attack, or a Rigor Mortis bypass. The contact has
// already blocked even when Energy is insufficient and the stance ends.
inline std::optional<DistortionBlockProposal> distortion_block(const Registry& original,
 Scope owner,unsigned energy,Time now){
 if(!owner.instance||!owner.actor||owner.actor>65535||!owner.generation)return {};
 const auto entry=original.newest_skill(owner,11,now);
 if(!entry||entry->kind!=Category::stance||entry->cause!=owner.actor||!entry->instance||
    entry->applied>now||!entry->expires||now>=*entry->expires||!distortion(entry->rank))return {};
 auto proposed=original;
 if(energy>=2)return DistortionBlockProposal{std::move(proposed),entry->instance,false,energy-2};
 if(!proposed.remove(owner,entry->instance))return {};
 return DistortionBlockProposal{std::move(proposed),entry->instance,true,std::nullopt};
}
}
