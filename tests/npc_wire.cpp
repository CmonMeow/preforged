#include "gw_npc_wire.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv)try{
    using namespace fw::gw::npc;
    if(argc!=3)throw std::runtime_error("definition and name fixture paths required");
    Definition n{17,1234,5678,{0,127,128,255},0x11223344,8,6,20,{}};
    std::vector<Packet> packets;
    for(const auto* name:{u"",u"\x8101\x1234\x1",u"1234567"}){
        n.encoded_name=name;packets.push_back(properties_2022(n));
    }
    for(unsigned count:{0,1,8}){
        std::vector<std::uint32_t> files;for(unsigned i=0;i<count;++i)files.push_back(100+i);
        packets.push_back(model_files_2022(17,files));
    }
    std::ifstream in(argv[1]);if(!in)throw std::runtime_error("missing retail NPC fixtures");
    for(auto packet:packets){
        unsigned opcode;std::string hex;
        if(!(in>>opcode>>hex)||hex.size()%2||opcode!=packet[0])throw std::runtime_error("invalid fixture");
        packet[1]|=0x80;Packet expected;
        for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        if(packet!=expected)throw std::runtime_error("retail NPC serialization mismatch");
    }
    std::string trailing;if(in>>trailing)throw std::runtime_error("untested NPC fixture");
    std::ifstream names(argv[2]);if(!names)throw std::runtime_error("missing NPC name fixtures");
    for(const auto* label:{u"Guild Lord",u"Bodyguard",u"Knight",u"Archer",u"Footman"}){
        auto packet=name_2022(172,std::u16string(u"\u0108\u0107")+label+u"\x0001");packet[1]|=0x80;
        std::string key,hex;if(!(names>>key>>hex)||hex.size()%2)throw std::runtime_error("invalid NPC name fixture");
        Packet expected;for(std::size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        if(packet!=expected)throw std::runtime_error("original NPC name serializer mismatch");
    }
    if(names>>trailing)throw std::runtime_error("untested NPC name fixture");
    const auto rejects=[](auto action){try{action();}catch(const std::invalid_argument&){return true;}return false;};
    if(!rejects([]{name_2022(65536,u"Archer");})||!rejects([]{name_2022(176,std::u16string(32,u'A'));})||!rejects([]{name_2022(176,std::u16string{u'A',0,u'B'});}))throw std::runtime_error("invalid NPC name override accepted");
    n.encoded_name.assign(8,u'A');if(!rejects([&]{properties_2022(n);}))throw std::runtime_error("name overflow accepted");
    n.encoded_name={u'A',0,u'B'};if(!rejects([&]{properties_2022(n);}))throw std::runtime_error("embedded name terminator accepted");
    n.encoded_name.clear();n.id=65536;if(!rejects([&]{properties_2022(n);}))throw std::runtime_error("NPC identity overflow accepted");
    if(!rejects([]{model_files_2022(1,std::vector<std::uint32_t>(9));}))throw std::runtime_error("model list overflow accepted");
    std::cout<<"PASS NPC properties/model files against six retail serializer fixtures\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
