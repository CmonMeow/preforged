#pragma once
#include "gw_outgoing_2022_generated.h"
#include <span>
#include <cstddef>
namespace fw::gw::world {
enum class FrameState { complete, incomplete, unknown_opcode, invalid };
struct FrameResult { FrameState state; std::size_t size; const char* reason; };
inline constexpr std::size_t outgoing_index_size=[] {
    std::size_t size=0;
    for(const auto& schema:outgoing_2022)if(size<=schema.opcode)size=std::size_t(schema.opcode)+1;
    return size;
}();
inline constexpr auto outgoing_index_2022=[] {
    std::array<const OutgoingSchema*,outgoing_index_size> index{};
    for(const auto& schema:outgoing_2022){
        if(index[schema.opcode])throw "duplicate outgoing opcode";
        index[schema.opcode]=&schema;
    }
    return index;
}();
// Wire rules from build 37130 serializer RVA 3615a0. Counts are bounded by
// the extracted descriptor, not by guessed fixed packet lengths.
inline FrameResult outgoing_frame_2022(std::span<const std::uint8_t> bytes) {
    if(bytes.size()<2)return {FrameState::incomplete,0,"header"};
    const auto opcode=(bytes[0]|(unsigned(bytes[1])<<8))&0x7fff;
    const auto* schema=opcode<outgoing_index_2022.size()?outgoing_index_2022[opcode]:nullptr;
    if(!schema)return {FrameState::unknown_opcode,0,"not in pinned outgoing table"};
    std::size_t at=2, variable=0;
    auto parse=[&](auto&& self,unsigned begin,unsigned end)->FrameState {
        for(unsigned i=begin;i<end;++i){
            const auto field=schema->fields[i],kind=field&15,count=field>>8;
            std::size_t width=0;
            switch(kind){
            case 0:case 1:width=4;break;
            case 2:width=8;break;
            case 3:width=12;break;
            case 4:case 5:case 8:case 9:width=count;break;
            case 6:case 10:width=variable;break;
            case 7:case 11:{
                if(bytes.size()-at<2)return FrameState::incomplete;
                const unsigned n=bytes[at]|(unsigned(bytes[at+1])<<8);at+=2;
                if(kind==7){if(n>=count)return FrameState::invalid;width=2*n;}
                else {if(n>count)return FrameState::invalid;variable=std::size_t(n)<<((field>>4)&15);}
                break;
            }
            case 12:{
                if(bytes.size()==at)return FrameState::incomplete;
                const auto n=bytes[at++];if(n>count)return FrameState::invalid;
                for(unsigned row=0;row<n;++row){const auto result=self(self,i+1,end);if(result!=FrameState::complete)return result;}
                return FrameState::complete;
            }
            default:return FrameState::invalid;
            }
            if(width>65536 || at+width>65536)return FrameState::invalid;
            if(bytes.size()-at<width)return FrameState::incomplete;
            at+=width;
        }
        return FrameState::complete;
    };
    const auto state=parse(parse,0,schema->count);
    return {state,state==FrameState::complete?at:0,state==FrameState::invalid?"field count exceeds descriptor":"descriptor framing"};
}
}
