#pragma once
#include "gw_character_profile.h"
#include <algorithm>
namespace fw::gw::experience {
// Archived wiki/Level.html: from level L requires2000+600*(L-1) XP.
// Cumulative level20 threshold is140600.
inline unsigned level(std::uint32_t total){
    unsigned result=1;std::uint32_t threshold=0;
    while(result<20){threshold+=2000+600*(result-1);if(total<threshold)break;++result;}
    return result;
}
inline CharacterProfile advance(const CharacterProfile& current,std::uint32_t total){
    if((current.unknown1!=6&&current.unknown1!=8)||current.extension_count<37)
        throw std::invalid_argument("unsupported level summary layout");
    auto result=current;const unsigned previous=(current.extended[12]>>4)&31;
    if(!previous||previous>20)throw std::invalid_argument("invalid saved player level");
    // Imported accounts can have partial XP baselines. Preserve their level.
    result.extended[12]=(result.extended[12]&~0x1f0u)|(std::max(previous,level(total))<<4);
    return result;
}
}
