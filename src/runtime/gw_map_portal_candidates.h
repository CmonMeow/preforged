#pragma once
#include "gw_map_portals.h"
#include "gw_layered_navigation.h"

namespace fw::gw::map_portals {
// Developer-only inference. It does not import a reverse retail spawn. A
// logical reverse edge must still be supported by the game's map adjacency.
inline std::optional<Route> reverse_candidate(const Route& forward,const navigation::Mesh& shared){
    if(forward.source_asset!=forward.destination_asset||shared.asset_hash()!=forward.source_asset)return {};
    validate(forward,shared,shared);
    const Point center{(forward.trigger.min_x+forward.trigger.max_x)*.5f,
                       (forward.trigger.min_z+forward.trigger.max_z)*.5f};
    const auto cell=shared.locate_cell(forward.source_plane,center);if(!cell)return {};
    navigation::LayeredMesh layers(shared);
    const Box exclusion{forward.trigger.min_x-32,forward.trigger.min_z-32,
                        forward.trigger.max_x+32,forward.trigger.max_z+32};
    // Search the inward ray with exact cell/plane connectivity. An endpoint
    // on an unrelated overlapping plane is never substituted as an arrival.
    for(unsigned distance=8;distance<=512;distance+=8){
        const auto inset=static_cast<float>(distance);
        const Point point{center.x-forward.outward.x*inset,center.z-forward.outward.z*inset};
        if(contains(exclusion,point))continue;
        const auto surface=layers.trace({forward.source_plane,*cell},center,point);
        if(!surface||!shared.walkable(surface->plane,point))continue;
        Route reverse=forward;reverse.id="candidate_reverse_"+forward.id;
        std::swap(reverse.source,reverse.destination);
        reverse.arrival={reverse.destination,static_cast<std::uint32_t>(surface->plane),point,
            std::atan2(-forward.outward.z,-forward.outward.x)};
        reverse.outward={-forward.outward.x,-forward.outward.z};
        reverse.provenance="generated_shared_asset_review_required";
        validate(reverse,shared,shared);return reverse;
    }
    return {};
}
} // namespace fw::gw::map_portals
