#include "../src/runtime/gw_item_effect_references.h"
#include <iostream>
int main()try{
 using namespace fw::gw::items::effect_references;const auto check=[](bool b){if(!b)throw std::runtime_error("native item effect reference regression");};
 for(unsigned grade=1;grade<=3;++grade){
  const std::array<std::uint32_t,2> raw{0x27e80000u|((grade-1)<<16)|706u,0x12340050};
  const auto decoded=decode(raw);check(decoded.valid&&decoded.references.size()==1);
  const auto& ref=decoded.references.front();check(ref.raw==raw[0]&&ref.skill==706&&ref.grade==grade&&ref.rank==(grade==1?0:grade==2?8:15));
 }
 for(auto raw:{0x27eb02c2u,0x27e80000u,0x27e8ffffu}){const std::array<std::uint32_t,1> words{raw};check(!decode(words).valid);}
 const std::array<std::uint32_t,1> opaque{0x24580019};check(decode(opaque).valid&&decode(opaque).references.empty());
 std::cout<<"PASS exact native internal effect reference width, grade/rank and opaque preservation\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
