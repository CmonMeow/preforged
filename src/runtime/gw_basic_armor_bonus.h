#pragma once
#include "gw_energy_regeneration.h"
namespace fw::gw::energy {
// Local Basic_armor.html: torso/arms energy, legs/feet recovery.
inline ArmorBonus armor_piece(unsigned profession,unsigned slot){
    valid_primary(profession);if(slot<2||slot>6)throw std::invalid_argument("armor slot");
    if(profession==1)return {};
    if(slot==3)return {0,profession==9?0:1};
    if(slot==5)return {0,profession==2||profession==9?0:1};
    if(slot==2)return {profession==10?0u:5u,0};
    if(slot==6)return {profession==2||profession==7?0u:5u,0};
    return {};
}
}
