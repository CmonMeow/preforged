#include "../src/runtime/gw_basic_attack_request.h"
#include "../src/runtime/gw_outgoing_frame.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
int main(int argc,char**argv)try{
 if(argc!=2)throw std::runtime_error("fixture path required");std::ifstream file(argv[1]);if(!file)throw std::runtime_error("fixture unavailable");
 const std::uint32_t targets[]{43,123456,0,43};const unsigned calls[]{0,1,0,255};std::string line;unsigned row=0;
 while(std::getline(file,line)){
  if(row>=4||line.size()!=14)throw std::runtime_error("fixture shape");std::vector<std::uint8_t> bytes;
  for(unsigned at=0;at<line.size();at+=2)bytes.push_back(static_cast<std::uint8_t>(std::stoul(line.substr(at,2),nullptr,16)));
  const auto parsed=fw::gw::combat::parse_attack(bytes);const auto framed=fw::gw::world::outgoing_frame_2022(bytes);
  if(!parsed||parsed->target!=targets[row]||parsed->call!=calls[row]||framed.state!=fw::gw::world::FrameState::complete||framed.size!=7)throw std::runtime_error("original native request mismatch");
  bytes[1]&=0x7f;if(!fw::gw::combat::parse_attack(bytes))throw std::runtime_error("plain opcode rejected");
  bytes.pop_back();if(fw::gw::combat::parse_attack(bytes))throw std::runtime_error("truncated accepted");bytes.push_back(0);bytes.push_back(0);if(fw::gw::combat::parse_attack(bytes))throw std::runtime_error("coalesced accepted");++row;
 }if(row!=4)throw std::runtime_error("fixture count");std::cout<<"PASS4 original ordinary attack requests, framing and exact consumption\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
