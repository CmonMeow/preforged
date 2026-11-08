#include "gw_item_wire.h"
#include <fstream>
#include <iostream>
using namespace fw::gw::items;
int main(int argc,char**argv)try{
    if(argc!=2)throw std::runtime_error("fixture path required");
    Definition item{42,1234,2,3,0x123,4,5,0xabcdef,10,567,1,u"Test",{}};
    std::vector<std::vector<std::uint8_t>> actual{equipment_change_2022(42,0),equipment_change_2022(42,41),definition_2022(item)};
    item.modifiers={0x12345678,0x9abcdef0};actual.push_back(definition_2022(item));
    std::ifstream in(argv[1]);if(!in)throw std::runtime_error("missing retail serializer fixtures");
    for(auto packet:actual){
        unsigned opcode;std::string hex;if(!(in>>opcode>>hex))throw std::runtime_error("missing fixture");
        packet[1]|=0x80;std::vector<std::uint8_t> expected;
        for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        if(packet!=expected)throw std::runtime_error("retail item serialization mismatch");
    }
    const auto rejects=[](auto action){try{action();}catch(const std::invalid_argument&){return true;}return false;};
    item.file=0;if(!rejects([&]{definition_2022(item);}))throw std::runtime_error("zero file accepted");item.file=1234;
    item.modifiers.resize(65);if(!rejects([&]{definition_2022(item);}))throw std::runtime_error("modifier overflow accepted");item.modifiers.clear();
    item.encoded_name.assign(64,u'A');if(!rejects([&]{definition_2022(item);}))throw std::runtime_error("name overflow accepted");
    if(!rejects([]{equipment_change_2022(0,0);}))throw std::runtime_error("empty update accepted");
    std::cout<<"PASS retail item/equipment serializer fixtures and bounds\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
