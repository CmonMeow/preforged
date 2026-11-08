#pragma once
#include "network.h"
#include <charconv>
#include <istream>
#include <set>
#include <stdexcept>
namespace gwserver {
inline void load_endpoints(std::istream& input,ServerSettings& settings){
 auto next=settings;std::set<std::string> keys;std::string line;unsigned total=0;
 auto trim=[](std::string text){const auto first=text.find_first_not_of(" \t\r");if(first==text.npos)return std::string{};return text.substr(first,text.find_last_not_of(" \t\r")-first+1);};
 while(std::getline(input,line)){
  total+=static_cast<unsigned>(line.size());if(total>4096)throw std::invalid_argument("server endpoint configuration exceeds bound");
  line=trim(line);if(line.empty()||line[0]=='#')continue;
  const auto equal=line.find('=');if(equal==line.npos)throw std::invalid_argument("endpoint setting requires key=value");
  const auto key=trim(line.substr(0,equal)),value=trim(line.substr(equal+1));
  if(value.empty()||!keys.insert(key).second)throw std::invalid_argument("empty or duplicate endpoint setting");
  if(key=="bind"||key=="public_host"){
   if(value.size()>253||value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-")!=value.npos)throw std::invalid_argument("invalid IPv4 endpoint address or hostname");
   if(key=="bind")next.bind=value;else next.public_host=value;
  }else if(key=="auth_port"||key=="portal_port"){
   unsigned port=0;const auto result=std::from_chars(value.data(),value.data()+value.size(),port);
   if(result.ec!=std::errc{}||result.ptr!=value.data()+value.size()||port<1024||port>65535)throw std::invalid_argument("invalid endpoint port");
   if(key=="auth_port")next.auth_port=port;else next.portal_port=port;
  }else if(key=="account_profile"){
   if(value.size()>1024||value.find_first_of("\n\t")!=value.npos||value.find('\0')!=value.npos)throw std::invalid_argument("invalid account profile path");
   next.profile=value=="none"?"":value;
  }else if(key=="identification_catalog"||key=="salvage_catalog"||key=="consumable_catalog"||key=="quest_catalog"||key=="quest_encounters"||key=="quest_waves"||key=="quest_corpses"||key=="quest_escorts"||key=="quest_givers"||key=="quest_giver_combat"){
   if(value.size()>1024||value.find_first_of("\n\t")!=value.npos||value.find('\0')!=value.npos)
    throw std::invalid_argument("invalid item catalog path");
   std::optional<std::filesystem::path> path;
   if(value!="none")path=std::filesystem::path(value);
   if(key=="quest_catalog")next.quest_catalog=std::move(path);
   else if(key=="quest_encounters")next.quest_encounters=std::move(path);
   else if(key=="quest_waves")next.quest_waves=std::move(path);
   else if(key=="quest_corpses")next.quest_corpses=std::move(path);
   else if(key=="quest_escorts")next.quest_escorts=std::move(path);
   else if(key=="quest_givers")next.quest_givers=std::move(path);
   else if(key=="quest_giver_combat")next.quest_giver_combat=std::move(path);
   else if(key=="identification_catalog")next.item_catalogs.identification=std::move(path);
   else if(key=="salvage_catalog")next.item_catalogs.salvage=std::move(path);
   else next.item_catalogs.consumables=std::move(path);
  }else throw std::invalid_argument("unknown endpoint setting: "+key);
 }
 if(input.bad())throw std::invalid_argument("endpoint configuration read failed");
 if(next.auth_port==65535||next.portal_port==next.auth_port||next.portal_port==next.auth_port+1)throw std::invalid_argument("auth/world/portal ports overlap or overflow");
 if(next.public_host=="0.0.0.0")throw std::invalid_argument("public_host must be a reachable address, not a wildcard");
 settings=std::move(next);
}
}
