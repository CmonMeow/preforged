#pragma once
#include "gw_northlands_gate.h"
#include "gw_layered_navigation.h"
#include <memory>
namespace fw::gw::northlands {
// Reviewed dynamic barrier binding, not an authored route. This reciprocal
// pair is the only South approach into the captured gate's bridge corridor.
inline constexpr navigation::SurfaceLink south_barrier{{36,20},{37,10}};
inline std::unique_ptr<navigation::LayeredMesh> navigation_view(const navigation::Mesh& mesh,Scene scene,bool collision_open){
    const auto asset=scene==Scene::south146?113021u:scene==Scene::north147?116025u:0u;
    if(!asset||mesh.asset_hash()!=asset||(scene==Scene::north147&&!collision_open))
        throw std::invalid_argument("Northlands scene/navigation binding mismatch");
    if(scene==Scene::north147)return std::make_unique<navigation::LayeredMesh>(mesh);
    std::vector<navigation::SurfaceLink> links;unsigned forward=0,reverse=0;
    for(const auto edge:mesh.surface_links()){
        if(edge.a==south_barrier.a&&edge.b==south_barrier.b){++forward;continue;}
        if(edge.a==south_barrier.b&&edge.b==south_barrier.a){++reverse;continue;}
        links.push_back(edge);
    }
    if(forward!=1||reverse!=1||links.empty()||!mesh.portal(south_barrier.a,south_barrier.b))
        throw std::invalid_argument("Northlands reciprocal barrier missing or unsupported empty overlay");
    // An empty explicit span means default links in LayeredMesh. Never allow
    // filtering to silently fall back to the unfiltered graph.
    return collision_open?std::make_unique<navigation::LayeredMesh>(mesh):
        std::make_unique<navigation::LayeredMesh>(mesh,links);
}
}
