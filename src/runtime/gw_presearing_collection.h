#pragma once
#include "gw_quest_state.h"
#include "gw_presearing_items.h"
namespace fw::gw::quests::presearing {
// Completion is durable, while the turn-in item can leave carried inventory.
// Re-entering without it must allow collection again under the SAME attempt;
// it must neither invent a new acceptance nor grant another reward.
inline bool scale_collection_open(const Progress& progress){
    return progress.cycle&&(progress.stage==Stage::active||progress.stage==Stage::completed);
}
inline bool restore_scale_encounter(const Progress& progress,const inventory::Profile& inventory){
    return scale_collection_open(progress)&&!inventory::presearing::carried_shimmering_scale(inventory);
}
}
