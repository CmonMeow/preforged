#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::npcs;
void check(bool x){if(!x)throw std::runtime_error("NPC healing eligibility");}
int main()try{
 const std::string hash(64,'a');
 for(auto value:{std::string("1 test"),std::string("0 test"),std::string("- -")}){
  std::string text="gw-npc-combat 11 "+hash+"\nsource test authored "+hash+" tests/healing_eligibility.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 100 - 1 0 test test - - -";
  // v1 fields0..20 followed by optional v2..10 fields21..43.
  for(unsigned i=21;i<=43;++i)text+=" -";
  // Version4 fire multiplier is nonoptional; default value1 has no evidence.
  std::istringstream tokens(text.substr(text.find("npc 146")));std::vector<std::string> fields;std::string f;while(tokens>>f)fields.push_back(f);
  fields[27]="1";text=text.substr(0,text.find("npc 146"));for(const auto& field:fields)text+=field+" ";text+=value+"\n";
  std::istringstream input(text);const auto catalog=load_catalog(input,hash);check(catalog.size()==1);
  const auto expected=value[0]=='-'?std::optional<bool>{}:std::optional<bool>(value[0]=='1');
  check(catalog.find(146,100)->healing_allowed==expected);
  auto legacy=text.substr(0,text.size()-value.size()-1)+"\n";legacy.replace(14,2,"10");
  std::istringstream old(legacy);const auto previous=load_catalog(old,hash);check(!previous.find(146,100)->healing_allowed.has_value());
  for(auto suffix:{std::string("1 -"),std::string("0 missing"),std::string("- test"),std::string("2 test")}){
   auto bad=text;bad.replace(bad.size()-value.size()-1,value.size(),suffix);bool rejected=false;try{std::istringstream stream(bad);load_catalog(stream,hash);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
  }
 }
 std::cout<<"PASS evidence-backed NPC healing eligibility true/false/unknown and malformed/missing evidence rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
