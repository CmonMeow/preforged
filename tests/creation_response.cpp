#include "gw_creation_response.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv)try{
    if(argc!=3)throw std::runtime_error("fixture paths required");
    for(auto status:{FW_INVALID,FW_NOT_FOUND,FW_OVERLOADED,FW_CONFLICT}){
        const auto error=fw::gw::creation::persistence_rejection(status);
        if(!error||*error!=(status==FW_CONFLICT?29u:19u))
            throw std::runtime_error("expected creation rejection must return a client response");
    }
    for(auto status:{FW_OK,FW_TIMEOUT,FW_DRAINING,FW_INTERNAL,FW_UNAVAILABLE})
        if(fw::gw::creation::persistence_rejection(status))
            throw std::runtime_error("creation rejection hides success or infrastructure failure");
    if(fw::gw::creation::failure_2022(19)!=std::vector<std::uint8_t>{0x8a,1,19,0,0,0}||
       fw::gw::creation::failure_2022(29)!=std::vector<std::uint8_t>{0x8a,1,29,0,0,0})
        throw std::runtime_error("native creation error framing mismatch");
    bool zero_rejected=false;try{fw::gw::creation::failure_2022(0);}catch(const std::invalid_argument&){zero_rejected=true;}
    if(!zero_rejected)throw std::runtime_error("success accepted as creation failure");
    std::ifstream campaigns(argv[2]);
    for(auto masks:{std::array<std::uint8_t,4>{3,0,0,0},{15,0,0,0},{1,2,4,8}}){
        auto packet=fw::gw::creation::campaigns_2022(masks[0],masks[1],masks[2],masks[3]);
        packet[1]|=0x80;std::string hex;
        if(!(campaigns>>hex)||hex.size()!=packet.size()*2)throw std::runtime_error("campaign fixture missing");
        for(size_t i=0;i<packet.size();++i)
            if(packet[i]!=std::stoul(hex.substr(i*2,2),nullptr,16))throw std::runtime_error("campaign serializer mismatch");
    }
    std::ifstream input(argv[1]);if(!input)throw std::runtime_error("fixture missing");
    fw::gw::creation::Created response;
    for(unsigned i=0;i<16;++i)response.character[i]=static_cast<std::uint8_t>(i+1);
    response.name=u"New Hero";response.unknown=0x1234;
    for(unsigned count:{0,64}){
        response.summary.resize(count);
        for(unsigned i=0;i<count;++i)response.summary[i]=static_cast<std::uint8_t>(i);
        auto packet=response.encode_2022();packet[1]|=0x80;
        std::string hex;if(!(input>>hex))throw std::runtime_error("fixture truncated");
        std::vector<std::uint8_t> expected;
        for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        if(packet!=expected)throw std::runtime_error("retail creation response mismatch");
    }
    for(unsigned count:{65,1024,1025}){
        response.summary.resize(count);
        bool rejected=false;try{response.encode_2022();}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("client UI summary overflow accepted");
    }
    std::cout<<"PASS creation response retail fixtures and summary bound\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
