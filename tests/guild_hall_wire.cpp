#include "gw_guild_wire.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
int main(int argc,char** argv)try{
 if(argc!=2)throw std::runtime_error("native fixture required");
 std::ifstream source(argv[1]);std::string label,expected;source>>label>>expected;
 if(!source||label!="visited_hall")throw std::runtime_error("native fixture unavailable");
 fw::gw::guild::General hall;hall.local_id=2;hall.key={1,2,3,4};hall.name=u"Unclaimed Private Hall";
 auto packet=fw::gw::guild::visited_hall(hall);packet[1]|=0x80;
 std::string actual;for(auto byte:packet){actual+="0123456789abcdef"[byte>>4];actual+="0123456789abcdef"[byte&15];}
 if(actual!=expected)throw std::runtime_error("visited hall differs from native fixture");
 for(unsigned id:{0u,1u}){hall.local_id=static_cast<std::uint16_t>(id);bool denied=false;try{(void)fw::gw::guild::visited_hall(hall);}catch(const std::invalid_argument&){denied=true;}if(!denied)throw std::runtime_error("hall aliases own membership");}
 hall.local_id=2;hall.key={};bool denied=false;try{(void)fw::gw::guild::visited_hall(hall);}catch(const std::invalid_argument&){denied=true;}if(!denied)throw std::runtime_error("hall lacks authoritative identity");
 std::cout<<"PASS native visited hall record, distinct membership and explicit instance identity\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
