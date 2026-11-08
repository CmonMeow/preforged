#include "gw_item_modifiers.h"
#include <array>
#include <iostream>
int main()try{
 using namespace fw::gw::items::modifiers;auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 auto raw=std::array<std::uint32_t,3>{0x25e80200,0x24580019,0x25220001};auto records=decode_wire(raw);auto kit=reviewed_kit(records);
 check(kit&&kit->kind==Kit::expert_salvage&&kit->uses_each==25,"reviewed exact kit and uses fields");
 check(records[2].raw==raw[2]&&records[2].identifier()==0x2522&&records[2].arg1()==0&&records[2].arg2()==1,"opaque modifier preserved not falsely classified as rune");
 std::array<std::uint32_t,4> capture{raw[0],raw[1],raw[2],0xc0000000};check(capture_to_wire(capture)==std::vector<std::uint32_t>(raw.begin(),raw.end()),"strip exactly native final sentinel");
 bool denied=false;try{decode_wire(capture);}catch(const std::invalid_argument&){denied=true;}check(denied,"capture sentinel cannot leak back into352");
 auto duplicate=records;duplicate.push_back(records[0]);check(!reviewed_kit(duplicate),"ambiguous duplicate metadata denied");
 check(!reviewed_kit(decode_wire(std::array<std::uint32_t,1>{0x25e80200})),"missing actual uses not fabricated");
 std::cout<<"Bounded raw modifier and reviewed kit decoder tests passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
