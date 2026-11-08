#include "gw_player_name_wire.h"
#include <fstream>
#include <iostream>
#include <string>
int main(int argc,char**argv)try{
 if(argc!=2)throw std::runtime_error("native name fixture required");std::ifstream input(argv[1]);std::string hex;if(!(input>>hex))throw std::runtime_error("missing native name fixture");
 std::vector<uint8_t> expected;for(size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));expected[1]&=0x7f;
 using namespace fw::gw::player_names;Record r{2,42,0x1234,7,0x5678,0x9abc};if(update_2022(r,u"Renamed Hero")!=expected)throw std::runtime_error("native full name update mismatch");
 std::string owner_path(argv[1]);owner_path.replace(owner_path.find_last_of("/\\")+1,std::string::npos,"owner-name-2022.txt");std::ifstream owner_input(owner_path);if(!(owner_input>>hex))throw std::runtime_error("missing native owner name fixture");expected.clear();for(size_t i=0;i<hex.size();i+=2)expected.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));expected[1]&=0x7f;
 if(fw::gw::character_name_packet(380,"Renamed Hero")!=expected)throw std::runtime_error("native owner name update mismatch");
 if(update_2022(r,u"Hero \U0001f431").empty())throw std::runtime_error("valid surrogate pair rejected");
 bool rejected=false;try{update_2022(r,u"");}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("emptyname accepted");
 r.actor=0;rejected=false;try{update_2022(r,u"Name");}catch(const std::invalid_argument&){rejected=true;}if(!rejected)throw std::runtime_error("missingactor accepted");
 std::cout<<"PASS native existing player name update fixture\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
