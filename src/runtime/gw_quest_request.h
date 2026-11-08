#pragma once
#include "gw_outgoing_frame.h"
namespace fw::gw::quests {
struct MarkerRequest {std::uint32_t id=0;bool missing=false;};
inline world::FrameState parse_marker_request_2022(std::span<const std::uint8_t> bytes,MarkerRequest& output){
    if(bytes.size()<2)return world::FrameState::incomplete;
    if(((bytes[0]|unsigned(bytes[1])<<8)&0x7fff)!=18)return world::FrameState::invalid;
    const auto frame=world::outgoing_frame_2022(bytes);
    if(frame.state!=world::FrameState::complete)return frame.state;
    if(frame.size!=7||bytes[6]>1)return world::FrameState::invalid;
    std::uint32_t id=0;for(unsigned i=0;i<4;++i)id|=std::uint32_t(bytes[2+i])<<(8*i);
    if(!id)return world::FrameState::invalid;
    output={id,bytes[6]!=0};return world::FrameState::complete;
}
}
