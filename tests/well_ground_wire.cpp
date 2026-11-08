#include "gw_skill_particle_wire.h"
#include <cassert>
#include <bit>
#include <limits>
#include <string_view>
using namespace fw::gw;

static combat::Packet bytes(std::string_view hex){
    auto digit=[](char c)->unsigned{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:255;};
    assert(hex.size()%2==0);combat::Packet out;
    for(std::size_t i=0;i<hex.size();i+=2){assert(digit(hex[i])<16&&digit(hex[i+1])<16);
        out.push_back(static_cast<std::uint8_t>((digit(hex[i])<<4)|digit(hex[i+1])));}
    return out;
}
int main(){
    const auto x=std::bit_cast<float>(0xc5b6c69fu),y=std::bit_cast<float>(0x44ea0b8cu);
    const auto power=skills::particles::well_ground_2022(91,25,x,y,0);
    const auto blood=skills::particles::well_ground_2022(92,25,x,y,0);
    assert(power.size()==3&&blood.size()==3);
    assert(power[0]==bytes("a0809fc6b6c58c0bea44000000000000ad000000"));
    assert(power[1]==bytes("a0809fc6b6c58c0bea44000000000000ae000000"));
    assert(power[2]==bytes("a0809fc6b6c58c0bea4400001900000084030000"));
    assert(blood[0]==bytes("a0809fc6b6c58c0bea44000000000000af000000"));
    assert(blood[1]==power[1]&&blood[2]==power[2]);
    const auto suffering=skills::particles::well_ground_2022(93,25,
        std::bit_cast<float>(0xc5b95e2bu),std::bit_cast<float>(0x44fa6f57u),0);
    assert(suffering.size()==3);
    assert(suffering[0]==bytes("a0802b5eb9c5576ffa44000000000000b0000000"));
    assert(suffering[1]==bytes("a0802b5eb9c5576ffa44000000000000ae000000"));
    assert(suffering[2]==bytes("a0802b5eb9c5576ffa4400001900000084030000"));
    const auto profane=skills::particles::well_ground_2022(94,25,
        std::bit_cast<float>(0xc5b60f7fu),std::bit_cast<float>(0x45085c81u),0);
    assert(profane.size()==3);
    assert(profane[0]==bytes("a0807f0fb6c5815c0845000000000000b1000000"));
    assert(profane[1]==bytes("a0807f0fb6c5815c0845000000000000ae000000"));
    assert(profane[2]==bytes("a0807f0fb6c5815c084500001900000084030000"));
    for(unsigned skill:{90u,95u}){
        bool rejected=false;try{(void)skills::particles::well_ground_2022(skill,25,0,0,0);}
        catch(const std::invalid_argument&){rejected=true;}assert(rejected);
    }
    bool rejected=false;try{(void)skills::particles::well_ground_2022(91,0,0,0,0);}
    catch(const std::invalid_argument&){rejected=true;}assert(rejected);
    rejected=false;try{(void)skills::particles::well_ground_2022(91,25,
        std::numeric_limits<float>::quiet_NaN(),0,0);}
    catch(const std::invalid_argument&){rejected=true;}assert(rejected);
}
