#pragma once
#include "gw_world_simulation.h"
namespace fw::gw::world {
// Exact supplemental actor only. Catalog/evidence distinguishes first-observed
// placement and private tuning from captured appearance and measured hits.
inline bool enroll_presearing_stalker(Instance& instance,const Instance::Npc& npc,std::uint64_t now){
    if(npc.id!=23000||npc.definition.id!=23000||!npc.combat_definition)return false;
    const auto scope=instance.effect_scope(npc.id);
    if(!scope||!instance.configure_tameable_stalker(*scope,npc.combat_definition->binding))return false;
    combat::ai::FlarePolicy motion;motion.source=*scope;motion.level=5;
    motion.speed=220;motion.acquire=1000;motion.follow=5000;motion.leash=5000;
    motion.flight={1000,32};motion.proximity_acquisition=false;
    motion.evidence="Private wild Stalker level5 policy; observed retaliation6/5/5HP, unknown target armor; authored4..7base/900ms windup/220speed/128reach";
    combat::ai::ordinary::Reviewed attack{4,7,0,2,false,combat::ai::ordinary::Delivery::melee,900,2000,128,{1000,32},motion.evidence};
    combat::ai::wander::Policy patrol{{{24,0},{0,24},{-24,0},{0,-24}},4000,10000,
        "Authored local patrol around first-observed anchor; not recovered retail spawn or route"};
    return instance.configure_tameable_stalker_combat(*scope,std::move(motion),std::move(attack),std::move(patrol),now);
}
}
