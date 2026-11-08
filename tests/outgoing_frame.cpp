#include <string>
#include "gw_outgoing_frame.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
int main(int argc,char** argv) try {
    if(argc!=2)throw std::runtime_error("fixture path required");
    std::ifstream input(argv[1]);if(!input)throw std::runtime_error("fixture file missing");
    unsigned opcode=0,count=0;std::string hex;
    using namespace fw::gw::world;
    for(unsigned id=0;id<0x8000;++id){
        const OutgoingSchema* expected=nullptr;
        for(const auto& schema:outgoing_2022)if(schema.opcode==id)expected=&schema;
        const auto* actual=id<outgoing_index_2022.size()?outgoing_index_2022[id]:nullptr;
        if(actual!=expected)throw std::runtime_error("opcode index changed descriptor identity");
        if(!expected)for(unsigned flag:{0u,0x8000u}){
            const auto wire=id|flag;
            const std::uint8_t bytes[]{static_cast<std::uint8_t>(wire),static_cast<std::uint8_t>(wire>>8)};
            if(outgoing_frame_2022(bytes).state!=FrameState::unknown_opcode)
                throw std::runtime_error("unknown opcode accepted by index");
        }
    }
    while(input>>opcode>>hex){
        std::vector<std::uint8_t> data;
        for(std::size_t i=0;i<hex.size();i+=2)data.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        auto result=outgoing_frame_2022(data);
        if(result.state!=FrameState::complete||result.size!=data.size())throw std::runtime_error("native serializer mismatch opcode "+std::to_string(opcode));
        for(std::size_t n=0;n<data.size();++n)
            if(outgoing_frame_2022(std::span(data).first(n)).state!=FrameState::incomplete)
                throw std::runtime_error("fragment consumed early opcode "+std::to_string(opcode));
        data.push_back(10);data.push_back(0x80);
        if(outgoing_frame_2022(data).size!=data.size()-2)throw std::runtime_error("coalesced packet over-consumed");
        ++count;
    }
    if(count!=378)throw std::runtime_error("fixture coverage incomplete");
    // UTF-16 text count and opaque variable data must be bounded before consuming.
    for(const auto packet: {std::vector<std::uint8_t>{95,128,255,255},std::vector<std::uint8_t>{67,128,255,255}})
        if(outgoing_frame_2022(packet).state!=FrameState::invalid)throw std::runtime_error("oversize field accepted");
    std::cout<<"PASS "<<count<<" native serializer fixtures, all prefixes and coalesced frames; oversized fields rejected\n";
    return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
