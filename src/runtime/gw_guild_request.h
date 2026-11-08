#pragma once
#include "gw_outgoing_frame.h"
#include "gw_chat_wire.h"
namespace fw::gw::guild {
struct AnnouncementRequest {uint32_t transaction=0;std::u16string body;size_t consumed=0;};
inline world::FrameState parse_announcement(std::span<const uint8_t> bytes,AnnouncementRequest& request){
 if(bytes.size()<2)return world::FrameState::incomplete;if(((bytes[0]|unsigned(bytes[1])<<8)&0x7fff)!=188)return world::FrameState::unknown_opcode;
 const auto frame=world::outgoing_frame_2022(bytes);if(frame.state!=world::FrameState::complete)return frame.state;const unsigned count=bytes[6]|unsigned(bytes[7])<<8;AnnouncementRequest r;for(unsigned i=0;i<4;++i)r.transaction|=uint32_t(bytes[2+i])<<(8*i);for(unsigned i=0;i<count;++i)r.body+=char16_t(bytes[8+i*2]|unsigned(bytes[9+i*2])<<8);if(!r.body.empty()&&!chat::valid_text(r.body))return world::FrameState::invalid;r.consumed=frame.size;request=std::move(r);return world::FrameState::complete;
}
}
