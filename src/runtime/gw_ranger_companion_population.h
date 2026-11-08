#pragma once
#include "gw_npc_catalog.h"
namespace fw::gw::quests::ranger_companion {
inline constexpr unsigned nente_source_identity=23001,nente_runtime_identity=24001;
// Stalker23000 is installed before merge_quest_givers, whose per-map allocator
// assigns23001 to Ivor. Keep the pinned observation file immutable here and
// map Nente to a distinct reviewed private runtime identity after that merge.
inline void assign_nente_identity(world::Instance::Npc& nente,const world::NpcCatalog& population){
    if(nente.id!=nente_source_identity||nente.definition.id!=nente_source_identity||
       nente.captured_name!="Master Ranger Nente")throw std::invalid_argument("Ranger Companion source identity mismatch");
    for(const auto& [map,actors]:population.maps)for(const auto& old:actors)
        if(old.id==nente_runtime_identity||old.definition.id==nente_runtime_identity||old.captured_name==nente.captured_name)
            throw std::invalid_argument("Ranger Companion duplicate Nente identity");
    nente.id=nente_runtime_identity;nente.definition.id=nente_runtime_identity;
}
}
