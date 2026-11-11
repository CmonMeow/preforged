#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
namespace capture {
// Raw client table fields, not inferred spawn rules or 2022 wire packets.
struct NpcDefinition {
    std::uint32_t model_file=0,skin_file=0,appearance=0,flags=0;
    std::uint32_t primary=0,secondary=0,level=0;
    std::array<std::uint8_t,4> visual_adjustment{};
    std::vector<std::uint32_t> model_files;
};
inline std::string npc_definition_json(const NpcDefinition& n){
    if(n.model_files.size()>64)throw std::invalid_argument("NPC model-file observation bound");
    std::string out="{\"model_file\":"+std::to_string(n.model_file)+
        ",\"skin_file\":"+std::to_string(n.skin_file)+",\"appearance\":"+std::to_string(n.appearance)+
        ",\"flags\":"+std::to_string(n.flags)+",\"primary\":"+std::to_string(n.primary)+
        ",\"secondary\":"+std::to_string(n.secondary)+",\"default_level\":"+std::to_string(n.level)+
        ",\"visual_adjustment_bytes\":[";
    for(std::size_t i=0;i<n.visual_adjustment.size();++i){if(i)out+=',';out+=std::to_string(n.visual_adjustment[i]);}
    out+="],\"model_files\":[";
    for(std::size_t i=0;i<n.model_files.size();++i){if(i)out+=',';out+=std::to_string(n.model_files[i]);}
    return out+"]}";
}
}
