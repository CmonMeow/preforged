#pragma once
#include "gw_npc_skill_ai.h"
#include "gw_npc_pursuit_motion.h"
#include "gw_npc_pursuit_outcome.h"
#include "gw_npc_pursuit_leash.h"
#include "gw_npc_aggro_memory.h"
#include "gw_projectile_flight.h"
#include <cstddef>
#include <vector>
namespace fw::gw::combat::ai {
// Explicit host content, never inferred from NPC display name/level. Geometry
// and projectile tolerances are authored until measured from retail traces.
struct FlarePolicy {
    skills::effects::Scope source,target;
    unsigned fire_magic=0,level=0;
    float speed=0,acquire=0,follow=0,leash=0;
    projectiles::Policy flight{0,0};
    std::string evidence;
    std::size_t max_flights=8; // authored resource bound, not a retail AI limit
    // Explicit starter policy can require committed damage before acquisition.
    bool proximity_acquisition=true;
    bool valid(bool allow_zero_level=false)const{
        const auto scope=[](auto s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
        const bool no_target=!target.instance&&!target.actor&&!target.generation;
        return scope(source)&&(no_target||(scope(target)&&source.instance==target.instance&&source!=target))&&fire_magic<=20&&(level||allow_zero_level)&&level<=255&&
            std::isfinite(speed)&&speed>0&&speed<=440&&std::isfinite(acquire)&&acquire>0&&acquire<=10000&&
            std::isfinite(follow)&&follow>=acquire&&follow<=10000&&std::isfinite(leash)&&leash>0&&leash<=10000&&
            flight.valid()&&!evidence.empty()&&evidence.size()<=1024&&max_flights&&max_flights<=64;
    }
};
struct FlareController {
    FlarePolicy policy;float home_x=0,home_y=0;unsigned plane=0;
    std::uint64_t updated=0;bool engaged=false,returning=false;
    std::optional<npcs::control::Token> cast;
    struct Shot {
        skills::effects::Scope target;
        float x=0,y=0;unsigned plane=0;
        std::uint64_t released=0,arrives=0,sequence=0;
        float origin_x=0,origin_y=0;
    };
    std::vector<Shot> shots;
    std::optional<aggro::Memory> encounter;
    // Optional owned melee encounter; it has no spellbook or Energy pool.
    enum class OwnedAcquisition { damage_only, owner_proximity };
    struct MeleeQuest {skills::effects::Scope owner;unsigned quest=0,cycle=0;double health=0;bool triggered=false,retired=false;std::uint64_t health_loss_revision=0;OwnedAcquisition acquisition=OwnedAcquisition::damage_only;};
    std::optional<MeleeQuest> melee_quest;
    struct Movement {motion::Outcome outcome;std::uint64_t at=0;};
    std::optional<Movement> last_movement;
};
}
