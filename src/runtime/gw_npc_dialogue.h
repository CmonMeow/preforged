#pragma once
#include "gw_chat_wire.h"
#include "gw_item_wire.h"
#include "gw_outgoing_frame.h"
#include <set>

namespace fw::gw::dialogue {
using Packet=items::Packet;
using Batch=std::vector<Packet>;
struct Button {std::u16string label;std::uint32_t id=0,skill=0x0fffffff;std::uint8_t icon=0;};
inline std::u16string literal(const std::u16string& value,unsigned maximum){
    if(value.size()>maximum||!chat::valid_text(value))throw std::invalid_argument("invalid NPC dialogue text");
    return std::u16string{u'\x108',u'\x107'}+value+u'\x1';
}
inline void text(Packet& packet,std::u16string_view value){
    items::put(packet,static_cast<std::uint32_t>(value.size()),2);
    for(auto unit:value)items::put(packet,unit,2);
}
// Build37130: body127 appends to the dialog buffer; sender128 publishes that
// buffer through UI100000A4 and clears it. Buttons125 emit UI100000A1.
// Captured native strings already contain resource/control codes. Never wrap
// them as literal localized text or reinterpret them as Unicode chat messages.
inline Batch open_encoded(std::uint32_t actor,const std::u16string& encoded,const std::vector<Button>& buttons){
    if(!actor||actor>65535||buttons.size()>32)throw std::invalid_argument("invalid NPC dialogue identity/capacity");
    auto valid=[](std::u16string_view value,unsigned bound){
        if(value.size()>bound||value.find(u'\0')!=value.npos)throw std::invalid_argument("invalid encoded NPC dialogue text");};
    valid(encoded,259);Batch out;
    for(std::size_t offset=0;offset<encoded.size();){
        auto count=std::min<std::size_t>(121,encoded.size()-offset);
        if(offset+count<encoded.size()&&encoded[offset+count-1]>=0xd800&&encoded[offset+count-1]<=0xdbff)--count;
        Packet packet;items::put(packet,127,2);text(packet,std::u16string_view(encoded).substr(offset,count));out.push_back(std::move(packet));offset+=count;
    }
    Packet sender;items::put(sender,128,2);items::put(sender,actor,4);out.push_back(std::move(sender));
    std::set<std::uint32_t> ids;
    for(const auto& button:buttons){
        if(!ids.insert(button.id).second)throw std::invalid_argument("duplicate NPC dialogue choice");
        valid(button.label,127);
        Packet packet;items::put(packet,125,2);items::put(packet,button.icon,1);text(packet,button.label);
        items::put(packet,button.id,4);items::put(packet,button.skill,4);out.push_back(std::move(packet));
    }
    return out;
}
inline Batch open(std::uint32_t actor,const std::u16string& body,const std::vector<Button>& buttons){
    auto encoded_buttons=buttons;
    for(auto& button:encoded_buttons)button.label=literal(button.label,124);
    return open_encoded(actor,literal(body,256),encoded_buttons);
}
// Native sender8764C0 uses command57, one DWORD dialog identifier. An ID
// alone grants no authority: the caller must check the active NPC and range.
inline std::uint32_t selection(std::span<const std::uint8_t> request){
    const auto frame=world::outgoing_frame_2022(request);
    if(frame.state!=world::FrameState::complete||frame.size!=6||request.size()!=6||((request[0]|unsigned(request[1])<<8)&0x7fff)!=57)
        throw std::invalid_argument("invalid NPC dialogue selection");
    std::uint32_t id=0;for(unsigned n=0;n<4;++n)id|=std::uint32_t(request[2+n])<<(8*n);return id;
}
}
