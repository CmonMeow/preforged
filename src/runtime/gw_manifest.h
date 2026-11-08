#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <istream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fw::gw::manifest {
using Bytes=std::vector<std::uint8_t>;
struct Entry {std::uint16_t map;std::uint32_t version;std::array<std::vector<std::uint32_t>,2> phases;};
inline void word(Bytes& out,std::uint16_t n){out.push_back(n&255);out.push_back(n>>8);}
inline void dword(Bytes& out,std::uint32_t n){word(out,n&65535);word(out,n>>16);}
// Phase0: physical files. Phase1: composites, expanded by original7af100.
inline Bytes encode(const std::vector<std::uint32_t>& ids){
 if(ids.size()>65535)throw std::runtime_error("manifest dependency count exceeds WORD");
 Bytes out;word(out,static_cast<std::uint16_t>(ids.size()));
 std::uint32_t previous=0;
 for(std::size_t i=0;i<ids.size();++i){
  const auto id=ids[i];if(!id||id>=0x80000000u||(i&&id<=previous))throw std::runtime_error("manifest IDs must be positive sorted unique nonreserved DWORDs");
  if(!i)dword(out,id);else{
   const auto delta=id-previous;
   if(delta<=254)out.push_back(static_cast<std::uint8_t>(delta-1));
   else if(delta<=65790){out.push_back(254);word(out,static_cast<std::uint16_t>(delta-255));}
   else{out.push_back(255);dword(out,delta);}
  }previous=id;
 }return out;
}
enum class Context {world,creation_preview};
inline std::vector<Bytes> packets(const Entry& entry,Context context=Context::world){
 // An authenticated creation grant uses map0 and a separate scene asset;
 // it is neither a normal mission entry nor a provisionable map.
 if(context==Context::creation_preview){
  if(entry.map||entry.version||!entry.phases[0].empty()||!entry.phases[1].empty())throw std::runtime_error("invalid creation preview manifest");
 }else if(!entry.map||entry.map>876)throw std::runtime_error("manifest map outside pinned2022 mission table");
 std::vector<Bytes> out;
 for(unsigned phase=0;phase<2;++phase){
  Bytes start;word(start,407);start.push_back(static_cast<std::uint8_t>(phase));out.push_back(std::move(start));
  const auto payload=encode(entry.phases[phase]);
  for(std::size_t at=0;at<payload.size();at+=1024){
   const auto count=std::min<std::size_t>(1024,payload.size()-at);
   Bytes chunk;word(chunk,405);word(chunk,static_cast<std::uint16_t>(count));
   chunk.insert(chunk.end(),payload.begin()+at,payload.begin()+at+count);out.push_back(std::move(chunk));
  }
 }
 Bytes finish;word(finish,406);finish.push_back(0);word(finish,entry.map);dword(finish,entry.version);out.push_back(std::move(finish));return out;
}
// One line: map version phase0-count phase0-IDs... phase1-count phase1-IDs...
// A catalog is provisioned from reviewed capture data, never inferred from navmesh.
inline std::map<unsigned,Entry> read(std::istream& input){
 std::map<unsigned,Entry> result;std::string line;unsigned line_number=0;
 while(std::getline(input,line)){
  ++line_number;if(line.empty()||line[0]=='#')continue;
  std::istringstream row(line);std::uint64_t map,version;
  if(!(row>>map>>version)||!map||map>876||version>0xffffffffu)throw std::runtime_error("invalid manifest header line "+std::to_string(line_number));
  Entry entry{static_cast<std::uint16_t>(map),static_cast<std::uint32_t>(version),{}};
  for(auto& phase:entry.phases){
   unsigned count;if(!(row>>count)||count>65535)throw std::runtime_error("invalid manifest count");
   for(unsigned i=0;i<count;++i){std::uint64_t id;if(!(row>>id)||id>=0x80000000u||!id)throw std::runtime_error("invalid manifest dependency");phase.push_back(static_cast<std::uint32_t>(id));}
   (void)encode(phase);
  }
  std::string trailing;if(row>>trailing)throw std::runtime_error("unexpected manifest trailing fields");
  if(!result.emplace(static_cast<unsigned>(map),std::move(entry)).second)throw std::runtime_error("duplicate manifest map");
 }if(input.bad())throw std::runtime_error("manifest catalog read failed");return result;
}
}
