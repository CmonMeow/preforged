#pragma once
#include <optional>
namespace fw::gw::quests::presearing {
// These offers enter gw_auth's transactional kill-encounter acceptance paths.
// Captured quest text alone does not establish that their actors are installed.
// Supremacy of Air (37), Power of Blood (43), and Domination Magic (49)
// have distinct offer/objective maps in the installed captured catalogs. Their
// acceptance persists first and world entry restores the objective population.
inline bool requires_kill_encounter(unsigned quest){
    switch(quest){
    case 42:case 43:case 49:case 71:case 75:case 81:case 85:case 86:case 37:return true;
    default:return false;
    }
}
inline bool kill_encounter_offer_available(unsigned quest,unsigned offer_map,std::optional<unsigned> encounter_map){
    if(!requires_kill_encounter(quest))return true;
    return encounter_map&&*encounter_map&&((quest==37||quest==43||quest==49)||*encounter_map==offer_map);
}
}
