#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace fw::gw::skills::effects {
// Detached original-2022 packet. Descriptor60bacc/opcode91 invokes873df0,
// gates on the local caster at7924b0, looks up the target at763bf0 and writes
// its type3 stage at779af0. Verified with the original serializer and consumer
// by tools/protocol/iron_palm_chain_2022.py. No timeout or rendered claim.
inline std::vector<std::uint8_t> iron_palm_chain_2022(
        unsigned caster,unsigned target,unsigned stage){
    if(!caster||caster>65535||!target||target>65535||caster==target||stage>3)
        throw std::invalid_argument("invalid reviewed Iron Palm chain update");
    std::vector<std::uint8_t> packet;
    const auto append=[&](unsigned value,unsigned bytes){
        for(unsigned i=0;i<bytes;++i)packet.push_back(std::uint8_t(value>>(8*i)));
    };
    append(91,2);append(caster,4);append(target,4);append(stage,4);
    return packet;
}
}
