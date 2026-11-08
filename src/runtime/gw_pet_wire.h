#pragma once
#include "gw_item_wire.h"
#include "gw_outgoing_frame.h"
#include <optional>

namespace fw::gw::pets::wire {
using Packet=items::Packet;
enum class Behavior:unsigned {fight=0,guard=1,avoid_combat=2};
struct Command {enum class Kind {behavior,target};Kind kind;unsigned pet=0,value=0;};
inline bool valid_actor(unsigned actor){return actor&&actor<=65535;}
// Original2022 sender790270->875B10 cmd19;7902A0->875B60 cmd20. The
// authenticated session must validate this pet belongs to its current owner.
inline std::optional<Command> command(std::span<const std::uint8_t> bytes){
    const auto frame=world::outgoing_frame_2022(bytes);
    if(frame.state!=world::FrameState::complete||frame.size!=10||bytes.size()!=10)return {};
    const auto opcode=(bytes[0]|unsigned(bytes[1])<<8)&0x7fff;
    const auto word=[&](unsigned at){std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::uint32_t(bytes[at+i])<<(8*i);return value;};
    const auto pet=word(2),value=word(6);if(!valid_actor(pet))return {};
    if(opcode==19&&value<=2)return Command{Command::Kind::behavior,pet,value};
    if(opcode==20&&value<=65535)return Command{Command::Kind::target,pet,value};return {};
}
// PetAdd177 descriptor60BEC8/callback874E60, native World+6AC 28byte record
// and UI10000049 proved in tools/protocol/pet_2022.py. Pet name is encoded
// resource text, not a literal string; metadata never grants ownership itself.
inline Packet add(unsigned pet,unsigned owner,std::u16string_view name,unsigned model1,unsigned model2,Behavior behavior){
    if(!valid_actor(pet)||!valid_actor(owner)||pet==owner||name.empty()||name.size()>=32||unsigned(behavior)>2||!model1||!model2||name.find(u'\0')!=name.npos)
        throw std::invalid_argument("invalid pet add identity");
    Packet out;items::put(out,177,2);items::put(out,pet,4);items::put(out,owner,4);items::put(out,unsigned(name.size()),2);
    for(auto unit:name)items::put(out,unit,2);
    items::put(out,model1,4);items::put(out,model2,4);items::put(out,unsigned(behavior),4);return out;
}
inline Packet remove(unsigned pet){
    if(!valid_actor(pet))throw std::invalid_argument("invalid pet remove identity");
    Packet out;items::put(out,178,2);items::put(out,pet,4);return out;
}
inline Packet behavior(unsigned pet,Behavior value){
    if(!valid_actor(pet)||unsigned(value)>2)throw std::invalid_argument("invalid pet behavior");
    Packet out;items::put(out,97,2);items::put(out,pet,4);items::put(out,unsigned(value),4);return out;
}
inline Packet locked_target(unsigned pet,unsigned target){
    if(!valid_actor(pet)||target>65535||target==pet)throw std::invalid_argument("invalid pet target lock");
    Packet out;items::put(out,98,2);items::put(out,pet,4);items::put(out,target,4);return out;
}
}
