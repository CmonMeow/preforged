#include "gw_character_change.h"
#include "gw_character_name.h"
#include <iostream>
#include <vector>
int main()try{
    using namespace fw::gw::auth;using State=CharacterChange2022::State;
    if(fw::gw::character_name_packet(1,"",true)!=std::vector<std::uint8_t>{1,0,0,0})
        throw std::runtime_error("unnamed creation preview must retain empty name frame");
    if(fw::gw::character_name_packet(0x1234,"\xc3\x89\xe6\x9e\x97\xf0\x9f\x98\x80")!=
       std::vector<std::uint8_t>{0x34,0x12,4,0,0xc9,0,0x97,0x67,0x3d,0xd8,0,0xde})
        throw std::runtime_error("map name must count and encode UTF-16 units");
    if(fw::gw::character_name_packet(1,"Hero")!=std::vector<std::uint8_t>{1,0,4,0,'H',0,'e',0,'r',0,'o',0})
        throw std::runtime_error("ASCII map name changed");
    for(const auto& invalid:std::vector<std::string>{"",std::string(20,'a'),"\xc0\xaf","\xed\xa0\x80","\xf0\x9f"}){
        bool rejected=false;
        try{(void)fw::gw::character_name_packet(1,invalid);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("malformed map name accepted");
    }
    std::vector<std::uint8_t> packet{10,128,7,0,0,0,3,0,'A',0,' ',0,'B',0};
    CharacterChange2022 out;out.name=u"untouched";
    for(std::size_t n=0;n<packet.size();++n)
        if(parse_character_change_2022(std::span(packet).first(n),out)!=State::incomplete||out.name!=u"untouched")
            throw std::runtime_error("fragment changed output");
    packet.insert(packet.end(),{0,128,0,0,0,0});
    if(parse_character_change_2022(packet,out)!=State::complete||out.consumed!=14||out.transaction!=7||out.name!=u"A B")
        throw std::runtime_error("counted name/coalesced frame boundary");
    packet[6]=20;
    if(parse_character_change_2022(packet,out)!=State::invalid)throw std::runtime_error("oversize accepted");
    packet[6]=3;packet[2]=0;
    if(parse_character_change_2022(packet,out)!=State::invalid)throw std::runtime_error("zero transaction accepted");
    packet[2]=7;packet[8]=0;
    if(parse_character_change_2022(packet,out)!=State::invalid)throw std::runtime_error("embedded null accepted");
    packet[8]='A';packet[0]=7;
    if(parse_character_change_2022(packet,out,7)!=State::complete||out.name!=u"A B"||out.consumed!=14)
        throw std::runtime_error("named delete framing");
    if(parse_character_change_2022(packet,out)!=State::invalid)throw std::runtime_error("delete interpreted as selection");
    std::cout<<"PASS 2022 character selection framing\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
