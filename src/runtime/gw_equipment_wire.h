#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace fw::gw::equipment {
enum class Update : std::uint32_t { composite=14, color=15 };
// SMSG158 type64 assigns WorldContext+6a4 (FUN_00798be0). Creation's
// UI1000004e handler compares its actor field against this local character ID.
inline std::vector<std::uint8_t> local_character_2022(std::uint32_t actor){
    if(!actor)throw std::invalid_argument("invalid local character identity");
    std::vector<std::uint8_t> out{158,0};
    for(auto field:{64u,actor,0u})for(unsigned shift=0;shift<32;shift+=8)
        out.push_back(static_cast<std::uint8_t>(field>>shift));
    return out;
}
// Build 37130 SMSG110 / callback874090. This updates AgentInfo's owned
// item association and queues AgentView action9, unlike SMSG159 type20,
// which queues effect9 in a separate queue.
inline std::vector<std::uint8_t> assignment_2022(std::uint32_t agent,
                                               std::uint32_t slot,std::uint32_t item){
    if(!agent||slot>=9)throw std::invalid_argument("invalid 2022 equipment assignment");
    // AgentInfo wire order differs from physical composite/equipped-bag order.
    constexpr std::uint32_t wire_slot[]{0,1,2,4,6,3,5,7,8};
    std::vector<std::uint8_t> out{110,0};
    for(auto field:{agent,wire_slot[slot],item})for(unsigned shift=0;shift<32;shift+=8)
        out.push_back(static_cast<std::uint8_t>(field>>shift));
    return out;
}
// Type 0 initializes the packed appearance consumed by FUN_0078f150 and
// passed unchanged to CompositePlayer constructor FUN_007b0db0.
inline std::vector<std::uint8_t> appearance_2022(std::uint32_t agent,std::uint32_t appearance){
    if(!agent||!appearance)throw std::invalid_argument("invalid 2022 composite appearance");
    std::vector<std::uint8_t> out{158,0};
    for(auto field:{0u,agent,appearance})for(unsigned shift=0;shift<32;shift+=8)
        out.push_back(static_cast<std::uint8_t>(field>>shift));
    return out;
}
// Build 37130 SMSG 159, descriptor RVA 626708 / callback VA 874ba0.
// The transport applies the framing bit. Composite value includes the
// composite ID in its low WORD and the third appearance parameter above it.
inline std::vector<std::uint8_t> encode_2022(Update update,std::uint32_t agent,
                                          std::uint32_t index,std::uint32_t value) {
    if(!agent||index>=5||(update!=Update::composite&&update!=Update::color))
        throw std::invalid_argument("invalid 2022 equipment update");
    std::vector<std::uint8_t> out{159,0};
    for(auto field:{static_cast<std::uint32_t>(update),agent,index,value})
        for(unsigned shift=0;shift<32;shift+=8)
            out.push_back(static_cast<std::uint8_t>(field>>shift));
    return out;
}
}
