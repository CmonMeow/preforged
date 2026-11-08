#include "gw_vitals.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>
using namespace fw::gw;
std::string hex(const std::vector<std::uint8_t>& packet){std::ostringstream out;out<<std::hex<<std::setfill('0');for(auto b:packet)out<<std::setw(2)<<unsigned(b);return out.str();}
int main()try{
 auto check=[](bool ok){if(!ok)throw std::runtime_error("Energy ceiling wire regression");};
 // Original60bdd8 serializer emits9c80; framework logical packets omit the
 // transport0x8000 opcode flag. Payloads below retain its exact bytes, using
 // the captured0.33f/50 normalized rate (0x3bd844d1).
 check(hex(vitals::energy_ceiling_curve_2022(25,50,40,.33))=="9c0019000000cdcc4c3fd144d83b");
 check(hex(vitals::energy_ceiling_curve_2022(25,50,-10,.33))=="9c0019000000cdcc4cbed144d83b");
 check(hex(vitals::energy_ceiling_curve_2022(25,50,50,0))=="9c00190000000000803f00000000");
 for(unsigned mode=0;mode<9;++mode){bool rejected=false;try{
  vitals::energy_ceiling_curve_2022(mode==0?0:mode==1?65536:25,mode==2?0:mode==3?1000001:50,
    mode==4?51:mode==5?-1000001:mode==6?std::numeric_limits<double>::quiet_NaN():40,
    mode==7?-1:mode==8?std::numeric_limits<double>::infinity():.33);
 }catch(const std::invalid_argument&){rejected=true;}check(rejected);}
 std::cout<<"PASS original156 signed ceiling/recovery wire and malformed projection rejection; no World integration claimed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
