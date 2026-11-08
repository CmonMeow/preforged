#include "../src/runtime/gw_attribute_publication.h"
#include <iostream>
int main()try{
 using namespace fw::gw::attributes;Publication phase;Bonuses zero{},rune{};rune[9]=3;
 const auto check=[](bool value){if(!value)throw std::runtime_error("attribute bootstrap phase regression");};
 //397 marks server loaded but native55 has not arrived: even real equipment
 // changes cannot publish a snapshot querying a nonexistent attribute object.
 check(!phase.required(false,zero));check(!phase.required(true,rune));
 bool refused=false;try{phase.published(rune);}catch(const std::logic_error&){refused=true;}check(refused);
 //Final134 bootstrap already contains the initial rune snapshot; no duplicate.
 phase.initialize(rune);check(!phase.required(true,rune));check(phase.required(true,zero));
 phase.published(zero);check(!phase.required(true,zero));
 phase.reset();check(!phase.required(true,rune));phase.initialize(zero);
 check(!phase.required(false,rune)&&phase.required(true,rune));
 std::cout<<"PASS pre-spawn loaded state, initial snapshot, equipment transition and map reset\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
