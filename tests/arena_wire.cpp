#include "gw_arena_wire.h"
#include "gw_outgoing_frame.h"
#include "gw_agent_wire.h"
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

int main(int argc,char** argv)try{
    using namespace fw::gw::arena;
    auto require=[](bool okay,const char* message){if(!okay)throw std::runtime_error(message);};
    require(fw::gw::agent::team_token_2022(1)==Packet({241,0,1,0,0,0}),"native241 local team token fixture");
    require(fw::gw::agent::allegiance_token_2022(42,2)==Packet({47,0,42,0,0,0,2,0,0,0}),"native47 agent team token fixture");
    const auto spawn=fw::gw::agent::spawn_2022(42,0x3000002a,0,0,0,1,0,2);
    require(spawn[43]==2&&spawn[44]==0&&spawn[45]==0&&spawn[46]==0,"native32 decoded team token offset");
    auto rejected=[&](auto f){try{f();}catch(const std::invalid_argument&){return;}throw std::runtime_error("invalid countdown accepted");};
    require(argc==2,"native arena fixture path required");
    std::ifstream file(argv[1]);require(bool(file),"native arena fixtures missing");
    const std::map<std::string,Packet> expected{
        {"pending472",pending_2022(true)},{"ready472",pending_2022(false)},
        {"start383",countdown_2022(u"Arena",0,30000)},
        {"quest383",countdown_2022(u"\x56b",0,90000,4)},
        {"correct382",correct_clock_2022(25032)},
        {"sync383",countdown_2022(u"Arena",5000,30000)},
        {"freeze384",freeze_clock_2022(30000)},{"clear381",clear_clock_2022()}};
    std::string name,hex;unsigned found=0,requests=0;
    while(file>>name>>hex){
        require(hex.size()%2==0,"invalid native hex");Packet bytes;
        for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(std::uint8_t(std::stoul(hex.substr(i,2),nullptr,16)));
        if(name=="enter163"||name=="cancel161"){
            const auto value=name=="enter163"?Request{163,0x6D}:Request{161,0};
            require(parse_request(bytes)==value,"native request field mismatch");
            const auto frame=fw::gw::world::outgoing_frame_2022(bytes);
            require(frame.state==fw::gw::world::FrameState::complete&&frame.size==bytes.size(),"native request framing mismatch");
            for(std::size_t n=0;n<bytes.size();++n)require(!parse_request(std::span(bytes).first(n)),"truncated request accepted");
            bytes[1]&=0x7F;require(parse_request(bytes)==value,"plain opcode changed request");
            bytes.push_back(0);require(!parse_request(bytes),"coalesced request accepted");++requests;
        }else{
            const auto row=expected.find(name);require(row!=expected.end(),"unexpected native fixture");
            bytes[1]&=0x7F;require(bytes==row->second,"retail serializer bytes mismatch");++found;
        }
    }
    require(found==expected.size()&&requests==2,"missing native arena fixtures");
    for(unsigned selector:{0u,1u,109u,255u}){
        const Packet p{163,0,std::uint8_t(selector)};
        require(parse_request(p)==Request{163,std::uint8_t(selector)},"native BYTE selector guessed as a restricted mapID");
    }
    require(cancel_2022()==Batch{clear_clock_2022(),pending_2022(false)},"cancel leaves clock or pending membership lock");
    require(countdown_2022(u"Arena",30000,30000).size()==23,"zero remaining clock must still be serializable");
    const auto literal=countdown_plain_2022(u"Arena",0,30000);
    require(literal[2]==8&&literal[4]==8&&literal[5]==1&&literal[6]==7&&literal[7]==1,"literal countdown must use native coded text");
    rejected([&]{countdown_plain_2022(std::u16string(119,u'x'),0,30000);});
    rejected([&]{countdown_plain_2022(u"bad\nlabel",0,30000);});
    rejected([&]{countdown_2022(u"Arena",0,0);});
    rejected([&]{countdown_2022(u"Arena",30001,30000);});
    rejected([&]{countdown_2022(u"Arena",0,30000,8);});
    rejected([&]{countdown_2022(std::u16string(122,u'x'),0,30000);});
    rejected([&]{countdown_2022(std::u16string(u"a\0b",3),0,30000);});
    std::cout<<"PASS native163/161 parsing,383 start/sync,384 freeze,381 clear,472 pending/ready and cancellation ordering\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
