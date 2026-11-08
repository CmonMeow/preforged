#pragma once
#include "gw_attributes.h"
namespace fw::gw::attributes {
// Local Weakness.html: -1 only when base rank>0; equipment-only ranks and
// title/engine attributes are unaffected. Never change the allocation profile.
inline Bonuses condition_bonuses(const Ranks& base,Bonuses equipment,bool weakness){
 if(weakness)for(unsigned id=0;id<base.size();++id)
  if(base[id]&&catalog[id].profession>=1&&catalog[id].profession<=10)--equipment[id];
 return equipment;
}
}
