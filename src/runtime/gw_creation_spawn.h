#pragma once
#include "gw_map_portals.h"
#include <istream>
#include <sstream>

namespace fw::gw::creation {
// Separate from ordinary map arrivals: only a committed creation redirect uses
// these observed first-entry points. Navigation validation belongs to the host.
inline std::optional<map_portals::Arrival> first_arrival(std::istream& input,
        unsigned campaign,unsigned map){
    std::optional<map_portals::Arrival> selected;
    std::string line;
    while(std::getline(input,line)){
        const auto comment=line.find('#');if(comment!=line.npos)line.resize(comment);
        std::istringstream row(line);row>>std::ws;if(row.eof())continue;
        unsigned origin=0;map_portals::Arrival arrival;std::string extra;
        if(!(row>>origin>>arrival.map>>arrival.point.x>>arrival.point.z>>arrival.plane>>arrival.facing)||
           row>>extra||origin<1||origin>3||!arrival.map||arrival.map>65535||arrival.plane>65535||
           !map_portals::finite(arrival.point)||!std::isfinite(arrival.facing))
            throw std::runtime_error("invalid creation spawn configuration");
        if(origin==campaign&&arrival.map==map){
            if(selected)throw std::runtime_error("duplicate creation spawn configuration");
            selected=arrival;
        }
    }
    return selected;
}
}
