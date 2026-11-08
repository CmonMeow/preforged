#include "gw_vitals.h"
#include <fstream>
#include <iostream>
#include <string>
using namespace fw::gw::vitals;
int main(int argc,char** argv)try{
    if(argc!=3)throw std::runtime_error("resource and morale fixture paths required");
    std::ifstream regen(std::string(argv[1]).substr(0,std::string(argv[1]).find_last_of("/\\")+1)+"energy-regen-2022.txt");
    if(!regen)throw std::runtime_error("energy regeneration fixture missing");
    for(auto sample:{std::pair{2,20u},std::pair{4,40u},std::pair{0,20u},std::pair{-2,20u},std::pair{10,80u}}){
        auto packet=energy_regeneration_2022(42,sample.first,sample.second);packet[1]|=0x80;
        std::string hex;if(!(regen>>hex))throw std::runtime_error("missing native regeneration packet");
        std::vector<std::uint8_t> expected;
        for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        if(packet!=expected)throw std::runtime_error("native energy regeneration mismatch");
    }
    for(auto sample:{std::pair{-11,20u},std::pair{11,20u},std::pair{2,0u},std::pair{2,1000001u}}){
        bool rejected=false;try{energy_regeneration_2022(42,sample.first,sample.second);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("invalid regeneration accepted");
    }
    if(level_2022(42,20)!=std::vector<std::uint8_t>{158,0,36,0,0,0,42,0,0,0,20,0,0,0})
        throw std::runtime_error("agent level field order");
    for(auto level:{0u,21u}){
        bool rejected=false;try{level_2022(42,level);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("invalid player level accepted");
    }
    if(base(1).max_health!=100||base(20).max_health!=480||base(20).max_energy!=20)
        throw std::runtime_error("base resource rules");
    std::ifstream input(argv[1]);if(!input)throw std::runtime_error("fixture missing");
    for(auto r:{Resources{480,480,20,20},Resources{120,480,10,20},Resources{0,480,0,20}})
        for(auto packet:packets_2022(42,r)){
            packet[1]|=0x80;std::string hex;
            if(!(input>>hex))throw std::runtime_error("missing retail packet");
            std::vector<std::uint8_t> expected;
            for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
            if(packet!=expected)throw std::runtime_error("retail vitals packet mismatch");
        }
    for(auto r:{Resources{1,0,0,20},Resources{481,480,20,20},Resources{480,480,21,20}}){
        bool rejected=false;try{packets_2022(42,r);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("invalid resources accepted");
    }
    std::ifstream morale(argv[2]);if(!morale)throw std::runtime_error("morale fixture missing");
    for(auto percent:{40u,100u,110u}){
        auto packet=morale_2022(42,percent);packet[1]|=0x80;std::string hex;
        if(!(morale>>hex))throw std::runtime_error("missing morale fixture");
        std::vector<std::uint8_t> expected;
        for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        if(packet!=expected)throw std::runtime_error("retail morale packet mismatch");
    }
    std::cout<<"PASS retail resource/morale fixtures, base level rules, bounds\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
