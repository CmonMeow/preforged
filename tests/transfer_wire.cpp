#include "gw_transfer_wire.h"
#include <fstream>
#include <iostream>
#include <string>
int main(int argc,char**argv)try{
    using fw::gw::world::development_travel_map;
    if(!fw::gw::world::valid_travel_flags_2022(0)||!fw::gw::world::valid_travel_flags_2022(1)||
       fw::gw::world::valid_travel_flags_2022(2)||fw::gw::world::valid_travel_flags_2022(255))
        throw std::runtime_error("native waypoint travel flag validation");
    if(development_travel_map(u"/travel 248")!=248||development_travel_map(u"/travel 148")!=148||
       development_travel_map(u"/tp 55")!=55||development_travel_map(u"/tp 166")!=166)
        throw std::runtime_error("development travel command rejected");
    for(auto text:{u"/travel ",u"/travel 0",u"/travel -1",u"/travel 65536",u"/travel 999999999999",u"/travel 148 x",u"hello /travel 148",u"/tp ",u"/tp 0",u"/tp 65536",u"/tp 148 x"})
        if(development_travel_map(text))throw std::runtime_error("invalid travel command accepted");
    if(argc!=2)throw std::runtime_error("fixture required");
    std::array<std::uint8_t,24> host{2,0,0x17,0xe8,127,0,0,1};
    auto packet=fw::gw::world::transfer_2022(host,0x11223344,7,248,-2,false);
    packet[1]|=0x80;
    std::ifstream in(argv[1]);std::string hex;in>>hex;
    if(hex.size()!=packet.size()*2)throw std::runtime_error("fixture size mismatch");
    for(std::size_t i=0;i<packet.size();++i)
        if(packet[i]!=std::stoul(hex.substr(i*2,2),nullptr,16))throw std::runtime_error("retail transfer mismatch");
    for(auto values:std::vector<std::array<int,4>>{{0,7,248,0},{1,0,248,0},{1,65536,248,0},{1,7,65536,0},{1,7,0,0},{1,7,248,-3},{1,7,248,5}}){
        bool rejected=false;
        try{fw::gw::world::transfer_2022(host,values[0],values[1],values[2],values[3],false);}
        catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("invalid transfer accepted");
    }
    std::cout<<"PASS retail2022 transfer address, field widths, signed region and identity validation\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
