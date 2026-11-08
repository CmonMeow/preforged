#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace fw::gw::chat {
using Packet=std::vector<std::uint8_t>;
inline bool valid_text(std::u16string_view text){
    if(text.empty()||text.size()>256)return false;
    for(std::size_t i=0;i<text.size();++i){
        const auto c=text[i];
        if(c<32||c==127)return false;
        if(c>=0xd800&&c<=0xdbff){
            if(++i==text.size()||text[i]<0xdc00||text[i]>0xdfff)return false;
        }else if(c>=0xdc00&&c<=0xdfff)return false;
    }
    return true;
}
inline std::vector<Packet> local(std::uint16_t sender,std::uint8_t channel,const std::u16string& text){
    if(!sender||!valid_text(text))throw std::invalid_argument("invalid chat message");
    // Literal string wrapper used by GWCA Chat::WriteChat. Reject embedded
    // control codes so user text cannot become an encoded-string program.
    auto encoded=std::u16string{u'\x108',u'\x107'}+text+u'\x1';
    std::vector<Packet> result;
    for(std::size_t offset=0;offset<encoded.size();){
        auto count=std::min<std::size_t>(121,encoded.size()-offset);
        if(offset+count<encoded.size()&&encoded[offset+count-1]>=0xd800&&encoded[offset+count-1]<=0xdbff)--count;
        Packet p{92,0,static_cast<std::uint8_t>(count),0};
        for(std::size_t i=0;i<count;++i){auto c=encoded[offset+i];p.push_back(c&255);p.push_back(c>>8);}
        result.push_back(std::move(p));offset+=count;
    }
    result.push_back({96,0,static_cast<std::uint8_t>(sender),static_cast<std::uint8_t>(sender>>8),channel});
    return result;
}
// Pinned37130 descriptor60bafc: BYTEchannel,string32 sender,string6 tag.
// Unlike96 this does not reference another map's player-index namespace.
inline std::vector<Packet> global(std::uint8_t channel,const std::u16string& name,const std::u16string& tag,const std::u16string& text){
    if(name.empty()||name.size()>31||tag.size()>4||!valid_text(name)||(!tag.empty()&&!valid_text(tag)))throw std::invalid_argument("invalid global chat identity");
    auto packets=local(1,channel,text);packets.pop_back();Packet end{95,0,channel};
    for(const auto* field:{&name,&tag}){end.push_back(static_cast<std::uint8_t>(field->size()));end.push_back(0);for(auto c:*field){end.push_back(c&255);end.push_back(c>>8);}}
    packets.push_back(std::move(end));return packets;
}
}
