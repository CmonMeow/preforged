#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace fw::gw::npc {
using Packet=std::vector<std::uint8_t>;
struct Definition {
    std::uint32_t id=0,model_file=0,skin_file=0;
    std::array<std::uint8_t,4> visual_adjustment{};
    std::uint32_t appearance=0,flags=0;
    std::uint8_t primary=0,level=0;
    std::u16string encoded_name;
};
inline void put(Packet& out,std::uint32_t value,unsigned bytes){
    for(unsigned i=0;i<bytes;++i)out.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
// Pinned37130 registry60BDC0 descriptorA266BC {154,DWORD,string32}.
// 874AE0->7945C0 indexes WorldContext+7CC AgentInfoArray by agent ID,
// replacing its encoded name. The NPC definition table is separate at+7FC.
inline Packet name_2022(std::uint32_t actor,std::u16string_view encoded){
    if(!actor||actor>65535||encoded.size()>31||encoded.find(u'\0')!=encoded.npos)
        throw std::invalid_argument("invalid NPC name override");
    Packet out;put(out,154,2);put(out,actor,4);put(out,static_cast<std::uint32_t>(encoded.size()),2);
    for(auto c:encoded)put(out,c,2);return out;
}
// Literal encoding is independently checked by npc_name_2022.py against the
// original37130 serializer. Use decoded capture text, never modern resource
// codes whose text-parser grammar has not been verified for this build.
inline Packet literal_name_2022(std::uint32_t actor,std::u16string_view name){
    if(name.empty()||name.size()>28||std::any_of(name.begin(),name.end(),[](char16_t c){return c<32;}))throw std::invalid_argument("invalid literal NPC name");
    return name_2022(actor,std::u16string{u'\x108',u'\x107'}+std::u16string(name)+u'\x1');
}
// 37130 descriptor RVA60ba84; callback873c30 -> 791b90. Secondary profession
// is zeroed by that callback, not supplied by this message. Names are encoded
// client strings, not UTF-16 display text, and must remain uninterpreted.
inline Packet properties_2022(const Definition& n){
    if(n.id>65535||n.encoded_name.size()>7||n.encoded_name.find(u'\0')!=n.encoded_name.npos)
        throw std::invalid_argument("invalid NPC definition identity or encoded name");
    Packet out;put(out,85,2);put(out,n.id,4);put(out,n.model_file,4);put(out,n.skin_file,4);
    out.insert(out.end(),n.visual_adjustment.begin(),n.visual_adjustment.end());
    put(out,n.appearance,4);put(out,n.flags,4);put(out,n.primary,1);put(out,n.level,1);
    put(out,static_cast<std::uint32_t>(n.encoded_name.size()),2);
    for(auto c:n.encoded_name)put(out,c,2);
    return out;
}
// 37130 descriptor RVA60ba90; callback873cb0 asserts count<=8. The NPC table
// entry must be populated before any agent referencing this ID is spawned.
inline Packet model_files_2022(std::uint32_t id,std::span<const std::uint32_t> files){
    if(id>65535||files.size()>8)throw std::invalid_argument("invalid NPC model-file list");
    Packet out;put(out,86,2);put(out,id,4);put(out,static_cast<std::uint32_t>(files.size()),2);
    for(auto file:files)put(out,file,4);
    return out;
}
}
