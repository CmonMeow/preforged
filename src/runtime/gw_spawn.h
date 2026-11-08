#pragma once
#include "gw_navigation.h"
#include <sstream>
namespace fw::gw::world {
struct SpawnPoint { std::uint32_t map=0; float x=0,y=0; };
// Explicit per-map entry points. This reader returns empty when no map row
// exists; auth applies a deterministic map-center fallback when navigation is
// present, otherwise a provisional map-origin fallback. No character logout
// position is consulted.
inline std::optional<SpawnPoint> map_spawn(const std::filesystem::path& configuration,
        std::uint32_t map,const navigation::Mesh& mesh) {
    std::ifstream input(configuration);
    if(!input)throw std::runtime_error("spawn configuration unavailable");
    std::optional<SpawnPoint> found;std::string line;
    while(std::getline(input,line)){
        const auto comment=line.find('#');if(comment!=std::string::npos)line.resize(comment);
        std::istringstream row(line);row>>std::ws;if(row.eof())continue;
        SpawnPoint point;std::string extra;
        if(!(row>>point.map>>point.x>>point.y)||(row>>extra)||!point.map||
           !std::isfinite(point.x)||!std::isfinite(point.y))
            throw std::runtime_error("invalid map spawn row");
        if(point.map!=map)continue;
        if(found)throw std::runtime_error("duplicate map spawn");
        // All current world position messages select ground plane zero.
        if(!mesh.walkable(0,{point.x,point.y}))throw std::runtime_error("map spawn outside ground navigation");
        found=point;
    }
    return found;
}
}
