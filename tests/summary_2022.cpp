#include "gw_summary_2022.h"
#include <fstream>
#include <iostream>
std::vector<std::uint8_t> unhex(const std::string& s){
    if(s.size()%2)throw std::runtime_error("odd fixture hex");
    std::vector<std::uint8_t> out;
    for(size_t i=0;i<s.size();i+=2)out.push_back(static_cast<std::uint8_t>(std::stoul(s.substr(i,2),nullptr,16)));
    return out;
}
int main(int argc,char**argv)try{
    if(argc!=2)throw std::runtime_error("fixture required");
    std::ifstream input(argv[1]);std::string source,expected;unsigned count=0;
    while(input>>source>>expected){
        auto bytes=unhex(source);if(bytes.size()!=112)throw std::runtime_error("invalid source struct");
        auto word=[&](unsigned index){std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(bytes[index*4+i])<<(8*i);return v;};
        fw::gw::Summary2022 s;s.appearance=word(0);s.low_nibble=word(1);
        for(unsigned i=0;i<4;++i)s.opaque_words[i]=word(2+i);
        for(unsigned i=0;i<word(6);++i)s.equipment.push_back({static_cast<std::uint16_t>(word(7+i)),static_cast<std::uint16_t>(word(12+i)),static_cast<std::uint8_t>(word(17+i))});
        s.level=word(22);s.map=word(23);s.opaque_metadata=word(24);s.is_pvp=word(25);s.secondary_profession=word(26);s.flag14=word(27);
        if(s.encode()!=unhex(expected))throw std::runtime_error("retail summary mismatch");
        const auto p=s.profile(0x12345678);const auto saved=p.encode();
        if(p.origin_campaign_2022()!=s.low_nibble||
           fw::gw::origin_campaign_packet_2022(p.origin_campaign_2022())!=
               std::vector<std::uint8_t>{123,0,s.low_nibble,0,0,0})
            throw std::runtime_error("origin campaign lost between roster and world");
        if(p.unknown0!=0x12345678||p.extension_count!=s.encode().size()||p.equipment_2022().size()!=word(6))
            throw std::runtime_error("profile envelope mismatch");
        auto summary=s.encode();
        if(!std::equal(summary.begin(),summary.end(),saved.begin()+6))throw std::runtime_error("profile summary mismatch");
        s.level=32;bool rejected=false;try{s.encode();}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("level overflow accepted");
        s.level=20;s.secondary_profession=11;rejected=false;
        try{s.encode();}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("invalid secondary profession accepted");++count;
    }
    if(count!=6)throw std::runtime_error("missing fixtures");
    bool rejected_campaign=false;
    try{fw::gw::origin_campaign_packet_2022(16);}catch(const std::invalid_argument&){rejected_campaign=true;}
    if(!rejected_campaign)throw std::runtime_error("campaign overflow accepted");
    fw::gw::CharacterProfile missing;
    bool rejected_profile=false;
    try{missing.origin_campaign_2022();}catch(const std::invalid_argument&){rejected_profile=true;}
    if(!rejected_profile)throw std::runtime_error("missing summary interpreted as campaign");
    std::cout<<"PASS summary writer fixtures and profile packaging\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

