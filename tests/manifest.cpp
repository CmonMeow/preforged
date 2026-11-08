#include "../src/runtime/gw_manifest.h"
#include <iostream>
int main()try{
 using namespace fw::gw::manifest;
 const auto check=[](bool ok){if(!ok)throw std::runtime_error("manifest regression");};
 check(encode({1,2,256,511,66301,132092})==Bytes({6,0,1,0,0,0,0,253,254,0,0,254,255,255,255,255,0,1,0}));
 const auto native_chunk=Bytes({0x95,1,10,0,3,0,0x58,0x3a,0,0,0,0xfe,8,0});
 const auto wire=packets({248,0,{{{14936,14937,15200},{}}}});
 check(wire.size()==5&&wire[0]==Bytes({0x97,1,0})&&wire[1]==native_chunk&&wire[2]==Bytes({0x97,1,1})&&wire[3]==Bytes({0x95,1,2,0,0,0})&&wire[4]==Bytes({0x96,1,0,248,0,0,0,0,0}));
 std::vector<std::uint32_t> large;for(unsigned i=1;i<=4000;++i)large.push_back(i);
 const auto preview=packets({0,0,{}},Context::creation_preview);
 check(preview.size()==5&&preview.back()==Bytes({0x96,1,0,0,0,0,0,0,0}));
 const auto denied=[&](Entry entry,Context context){bool rejected=false;try{(void)packets(entry,context);}catch(const std::exception&){rejected=true;}check(rejected);};
 denied({0,0,{}},Context::world);denied({877,0,{}},Context::world);
 denied({248,0,{}},Context::creation_preview);denied({0,1,{}},Context::creation_preview);
 denied({0,0,{{{1},{}}}},Context::creation_preview);
 const auto payload=encode(large);const auto split=packets({248,7,{{large,{}}}});Bytes joined;
 for(std::size_t i=1;i+1<split.size();++i){const auto& p=split[i];if(p[0]!=0x95)break;check(p.size()<=1028);joined.insert(joined.end(),p.begin()+4,p.end());}
 check(joined==payload);
 std::istringstream good("# captured catalog\n248 17 3 14936 14937 15200 1 42\n");const auto catalog=read(good);check(catalog.size()==1&&catalog.at(248).version==17&&catalog.at(248).phases[1]==std::vector<std::uint32_t>{42});
 for(const auto bad:{"248 0 2 7 7 0","248 0 1 0 0","248 0 1 2147483648 0","877 0 0 0","248 0 2 7 0","248 0 0 0 junk","248 0 0 0\n248 1 0 0"}){bool rejected=false;try{std::istringstream input(bad);(void)read(input);}catch(const std::exception&){rejected=true;}check(rejected);}
 std::cout<<"PASS native manifest delta/chunk wire, phase separation, bounded catalog and multichunk reassembly\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
