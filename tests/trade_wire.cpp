#include "gw_trade_wire.h"
#include <fstream>
#include <iostream>
#include <string>
using namespace fw::gw::trade;
int main(int argc,char**argv)try{
    if(argc!=2)throw std::runtime_error("trade fixture required");
    std::ifstream input(argv[1]);if(!input)throw std::runtime_error("trade fixture missing");
    std::vector<std::vector<std::uint8_t>> packets{initiated_2022(42),requested_2022(42)};
    for(unsigned status=0;status<4;++status)packets.push_back(operation_result_2022(status));
    packets.push_back(cancelled_2022());
    for(auto packet:packets){
        packet[1]|=0x80;std::string hex;if(!(input>>hex))throw std::runtime_error("missing trade fixture");
        std::vector<std::uint8_t> expected;for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        if(packet!=expected)throw std::runtime_error("native trade fixture mismatch");
    }
    std::string path(argv[1]);path.replace(path.find_last_of("/\\")+1,std::string::npos,"trade-offer-2022.txt");
    std::ifstream offers(path);if(!offers)throw std::runtime_error("trade offer fixture missing");
    for(auto packet:{partner_offer_gold_2022(12345),partner_offer_item_2022(61000,250),offered_count_2022(7),partner_accepted_2022(),revoke_2022(),clear_partner_offer_2022()}){
        packet[1]|=0x80;std::string hex;if(!(offers>>hex))throw std::runtime_error("missing trade offer fixture");
        std::vector<std::uint8_t> expected;for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        if(packet!=expected)throw std::runtime_error("native trade offer fixture mismatch");
    }
    for(unsigned op=0;op<8;++op){
        std::vector<std::uint8_t> packet{static_cast<std::uint8_t>(op),0x80};
        if(op==0||op==2||op==3||op==5)packet.insert(packet.end(),{42,0,0,0});
        if(op==2||op==5)packet.insert(packet.end(),{7,0,0,0});
        auto request=parse_2022(packet);if(!request||unsigned(request->action)!=op)throw std::runtime_error("trade parse failed");
        if(packet.size()>2&&request->value!=42)throw std::runtime_error("trade field order");
        if(packet.size()==10&&request->quantity!=7)throw std::runtime_error("trade quantity order");
        packet.push_back(0);if(parse_2022(packet))throw std::runtime_error("trade trailing data accepted");
        packet.resize(1);if(parse_2022(packet))throw std::runtime_error("trade truncated data accepted");
    }
    if(parse_2022(std::vector<std::uint8_t>{8,0}))throw std::runtime_error("unknown trade action accepted");
    const std::vector<std::uint8_t> initiate{0x47,0x80,2,0,0,0};
    const auto start=parse_2022(initiate);if(!start||start->action!=Action::Open||start->value!=2)throw std::runtime_error("native71 trade initiate target rejected");
    auto bad=initiate;bad.push_back(0);if(parse_2022(bad))throw std::runtime_error("trade71 trailing data accepted");bad.pop_back();bad.pop_back();if(parse_2022(bad))throw std::runtime_error("trade71 truncated data accepted");
    std::cout<<"PASS native trade opening/cancel/status fixtures and request framing\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
