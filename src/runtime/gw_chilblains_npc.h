#pragma once
#include "gw_chilblains.h"
#include "gw_skill_particle_wire.h"
#include "gw_npc_combat_catalog.h"
#include <set>
namespace fw::gw::skills::effects {
// Explicit ordering remains available for controlled comparison tests.
struct ChilblainsNpcRecipient {
    combat::npcs::LiveActor actor;
    combat::npcs::Lifetime expected;
    double distance=0; // World must supply current same-plane center distance.
};
struct ChilblainsNpcChange {
    combat::npcs::LiveActor actor;
    combat::npcs::Lifetime expected;
    combat::Hit hit;
    std::vector<Entry> removed;
    combat::Packet visual;
};
struct ChilblainsNpcProposal {
    combat::conditions::State caster_conditions;
    std::vector<ChilblainsNpcChange> recipients;
};
// Copy-only whole-batch proposal. Inputs must already be settled at now; the
// World owns spatial selection,source authority,publication and reactions.
inline std::optional<ChilblainsNpcProposal> prepare_chilblains_npcs(
        const ChilblainsPlan& plan,Scope caster,unsigned level,
        const combat::conditions::State& caster_conditions,
        const std::vector<ChilblainsNpcRecipient>& recipients,
        ChilblainsOrder order,std::uint64_t now){
    const auto canonical=chilblains(plan.rank);
    if(!canonical||*canonical!=plan||!caster.instance||!caster.generation||!level||level>255||
       (order!=ChilblainsOrder::damage_then_remove&&order!=ChilblainsOrder::remove_then_damage))return {};
    const auto poison=chilblains_poison(plan,caster_conditions,caster.actor,now);
    if(!poison)return {};
    ChilblainsNpcProposal result{*poison,{}};std::set<unsigned> seen;
    for(const auto& input:recipients){
        const auto scope=input.actor.hex_scope();
        if(!std::isfinite(input.distance)||input.distance<0||scope.instance!=caster.instance||
           scope.actor==caster.actor||!seen.insert(scope.actor).second||
           input.actor.lifetime()!=input.expected||!input.actor.hostile_to_players(input.expected))return {};
        // Out-of-area actors are not hit or stripped. The supplied snapshot
        // remains validated so stale/duplicate inputs cannot hide at boundaries.
        if(input.distance>plan.radius)continue;
        const auto clock=input.actor.health_clock(input.expected);
        if(clock&&*clock!=now)return {};
        auto proposed=input.actor;std::vector<Entry> removed;
        const auto strip=[&](){
            for(unsigned i=0;i<plan.enchantments;++i){
                const auto entry=proposed.newest_enchantment(input.expected,now);
                if(!entry)break;
                if(entry->skill==1392&&order!=ChilblainsOrder::damage_then_remove)return false;
                if(!proposed.remove_reviewed_enchantment(input.expected,entry->instance,now))return false;
                removed.push_back(*entry);
            }
            return true;
        };
        if(order==ChilblainsOrder::remove_then_damage&&!strip())return {};
        const auto hit=proposed.apply_damage(input.expected,{combat::Kind::cold,plan.damage,level,144},now);
        if(hit.reason!=combat::Reason::accepted)return {};
        if(order==ChilblainsOrder::damage_then_remove&&!strip())return {};
        result.recipients.push_back({std::move(proposed),input.expected,hit,std::move(removed),
            particles::chilblains_target_2022(scope.actor,caster.actor)});
    }
    return result;
}
}

