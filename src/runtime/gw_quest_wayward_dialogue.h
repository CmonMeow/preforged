#pragma once
#include "gw_quest_wayward_wizard.h"
#include "gw_quest_catalog.h"

namespace fw::gw::quests::wayward_wizard {
// Wiki-backed literal dialogue, not captured encoded strings or native choice
// identifiers. Provenance: data/gw/quests/wayward-wizard-evidence.json, wiki
// Dialogue/Notes, The_Wayward_Wizard.html revision2658089, wiki_sha256 above.
// Captured Orion sender events follow Ralena; they do not prove this earlier
// refusal. The wiki explicitly permits speaking to Orion before Ralena without
// advancing the quest. These bodies carry no action or reward authority.
inline std::optional<std::u16string> reminder(const Profile& current,const Catalog& catalog,
    const Eligibility& eligibility,unsigned map,std::string_view npc){
    if(!eligible(eligibility)||!catalog.contains(id))return {};
    const auto found=current.records.find(id);
    if(found==current.records.end()||found->second.stage!=Stage::active||
       !valid_progress(found->second))return {};
    if(map==148&&npc=="Sandre Elek")
        return u"Please. Have you found my brother yet? I have no more information to give you but that I think he's hiding out in Green Hills County.";
    if(map==161&&npc=="Orion Elek"&&found->second.objective_credits.empty())
        return u"Hmph. Sandre's a sweet girl, but she's very naive about the ways of the world. Some day, you will all see that peace and order can only be restored fully to Ascalon when the rightful royal line is restored to the throne.";
    return {};
}
}
