#include "gw_outgoing_frame.h"
#include "gw_skill_use_request.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc,char** argv)try{
    using namespace fw::gw::skills;
    auto require=[](bool okay,const char* message){if(!okay)throw std::runtime_error(message);};
    require(argc==2,"pinned skill-use fixture path required");
    std::ifstream input(argv[1]);require(bool(input),"fixture missing");
    unsigned opcode=0,count=0;std::string hex;
    while(input>>opcode>>hex){
        require(hex.size()==30,"native request must be15 wire bytes");
        std::vector<std::uint8_t> bytes;
        for(std::size_t at=0;at<hex.size();at+=2)bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(at,2),nullptr,16)));
        const auto expected=opcode==37?UseRequest{37,385,2,43,1}:UseRequest{68,281,1,43,1};
        require(opcode==37||opcode==68,"unexpected fixture opcode");
        const auto frame=fw::gw::world::outgoing_frame_2022(bytes);
        require(frame.state==fw::gw::world::FrameState::complete&&frame.size==15,"native descriptor framing mismatch");
        require(parse_use_request(bytes)==expected,"native sender field order mismatch");
        for(std::size_t n=0;n<bytes.size();++n)
            require(!parse_use_request(std::span(bytes).first(n)),"truncated frame accepted");
        bytes[1]&=0x7f;
        require(parse_use_request(bytes)==expected,"plain opcode decode changed fields");
        bytes[14]=0;
        auto noncall=expected;noncall.call=0;
        require(parse_use_request(bytes)==noncall,"non-call request lost");
        bytes[14]=255;
        noncall.call=255;
        require(parse_use_request(bytes)==noncall,"BYTE call changed by invented boolean gate");
        bytes.push_back(0);
        require(!parse_use_request(bytes),"coalesced/trailing frame accepted");
        bytes.resize(15);bytes[0]=67;
        require(!parse_use_request(bytes),"unrelated opcode accepted");
        ++count;
    }
    require(count==2,"both native senders must be covered");
    std::cout<<"PASS original37/68 sender fixtures, copy/target ordering, opcode directions, fullBYTE call, exact framing\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
