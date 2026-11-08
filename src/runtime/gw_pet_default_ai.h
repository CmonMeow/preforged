#pragma once
#include "gw_pet_profile.h"
#include "gw_npc_flare_ai.h"
#include "gw_npc_ordinary_ai.h"
#include "gw_companion_policy.h"
namespace fw::gw::pets {
struct DefaultAi {combat::ai::FlarePolicy motion;combat::ai::ordinary::Reviewed ordinary;ai::companion::Config behavior;};
inline std::optional<DefaultAi> default_stalker_ai(skills::effects::Scope source,const Pet& pet){
    if(!valid(pet)||pet.evolution!=Evolution::unevolved||!source.instance||!source.actor||source.actor>65535||!source.generation)return {};
    DefaultAi out;out.motion.source=source;out.motion.level=pet.level;
    // Animal_companion:25% movement bonus. Private base288 ->360; all distance
    // thresholds below are authored playability policy, not retail AI receipts.
    out.motion.speed=360;out.motion.acquire=1000;out.motion.follow=5000;out.motion.leash=5000;
    out.motion.flight={1000,32};out.motion.proximity_acquisition=false;
    out.motion.evidence="Reviewed pet2000ms cadence/slashing; authored360speed,128reach,900windup and follow thresholds";
    // Archived Stalker attack->release observations718..1188ms (36 contacts),
    // around900ms; they do not prove one exact native hit clock.900 is authored.
    // Actual damage comes from the scoped pet model, never these raw endpoints.
    out.ordinary={17,29,0,2,false,combat::ai::ordinary::Delivery::melee,900,2000,128,{1000,32},out.motion.evidence};
    out.behavior.follow_start=200;out.behavior.follow_stop=100;
    out.behavior.pet_guard_combat_break=1000;out.behavior.pet_attack_lock_range=2000;
    return out;
}
}
