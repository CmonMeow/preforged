#include "gw_ward_foes_player_wire.h"
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
float field(const fw::gw::agent::Packet& packet){
    std::uint32_t bits=0;for(unsigned i=0;i<4;++i)bits|=std::uint32_t(packet.at(6+i))<<(i*8);
    return std::bit_cast<float>(bits);
}
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
}
int main()try{
    using namespace fw::gw::movement;
    for(const auto gait:{1.f,.66f,.75f,.25f})for(std::uint8_t direction=0;direction<=8;++direction){
        const auto entry=ward_foes_player_speed_2022(7,.5f,gait,direction);
        const auto exit=ward_foes_player_speed_2022(7,1.f,gait,direction);
        check(entry[0]==fw::gw::agent::movement_base_speed_2022(7,144.f),"entry absolute base speed");
        check(exit[0]==fw::gw::agent::movement_base_speed_2022(7,288.f),"exit restores absolute base speed");
        check(entry[1]==exit[1]&&entry[1].back()==direction,"boundary preserves gait and facing mode");
        check(field(entry[0])*field(entry[1])==144.f*gait,"client rate agrees with composed server rate");
    }
    const auto capped=ward_foes_player_speed_2022(7,.75f,1.f,1);
    check(field(capped[0])==216.f,"caller composed boost and Ward before publication");
    for(const float value:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        bool rejected=false;try{ward_foes_player_speed_2022(7,value,1.f,1);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"invalid final multiplier accepted");
    }
    bool rejected=false;try{ward_foes_player_speed_2022(7,1.5f,1.5f,1);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"boost incorrectly encoded as packet43 fraction");
    std::cout<<"Ward player speed publication contract passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
