#pragma once
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"

namespace fw::gw::world {
// Provisional private-server content. The immutable 20261024 checkpoint's
// packets-1360-15821281.jsonl:499268 ties definition2937 to model141876 at
// skill475 completion. Hash-pinned world rows34101..34102 decode the name
// as Spirit of Quickening Zephyr and agree on appearance fields and actor97.
// That identifies the observed modern spirit appearance, but does NOT
// establish caster ownership or 2022 rendering. Level is server plan policy;
// the observed definition default_level is0 and actor97 separately has level1.
// Use a server-reserved definition per rank, avoiding conflicting live level
// records. Gameplay traits below are deliberate spirit policy, not a capture
// claim; Health is provisional balance and armor remains unknown.
inline std::optional<Instance::Npc> quickening_zephyr_candidate_template(
        std::uint32_t map,const skills::effects::QuickeningZephyrPlan& plan){
    if(!map||skills::effects::quickening_zephyr(plan.rank)!=plan)return {};
    Instance::Npc spirit;
    spirit.definition.id=60000+plan.rank;
    spirit.definition.model_file=141876;
    spirit.definition.level=std::uint8_t(plan.level);
    spirit.definition.visual_adjustment={0,0,0,80};
    spirit.definition.flags=16384;
    spirit.definition.primary=2;
    // Use the decoded text with the verified2022 literal-name serializer;
    // modern resource-name words are not assumed portable to2022.
    spirit.literal_display_name=u"Spirit of Quickening Zephyr";
    combat::npcs::Definition body;
    body.binding.map=map;body.binding.definition=spirit.definition.id;
    body.binding.model_file=spirit.definition.model_file;
    body.binding.level=spirit.definition.level;
    body.binding.definition_flags=spirit.definition.flags;
    body.relation=combat::npcs::Relation::allied_to_players;
    const auto health=30+20*plan.level;
    body.spawn_health=combat::Health{double(health),health};
    body.evidence.health="provisional private-server spirit balance: 30+20*level; not capture verified";
    body.evidence.relation="private-server summoned nature spirit policy; capture ownership unverified";
    body.damageable=true;
    body.healing_allowed=false;body.healing_evidence="private-server spirit policy";
    body.fleshy=false;body.fleshy_evidence="private-server spirit policy";
    body.creature_type="spirit";body.creature_type_evidence="skill475 nature ritual policy; observed Zephyr appearance; server spirit policy";
    body.holy_traits.summoned=true;body.summoned_evidence="server completed skill475 producer";
    spirit.combat_definition=std::move(body);
    return spirit;
}

// Shared by the production gateway and native cast regression. World keeps
// this actor hidden until Runtime queues finish/recharge, then invokes publish.
inline skills::casting::EffectCompletion stage_quickening_zephyr_candidate(
        Instance& world,skills::effects::Scope owner,
        const skills::effects::QuickeningZephyrPlan& plan,
        std::uint32_t map,std::uint64_t now){
    auto entry=quickening_zephyr_candidate_template(map,plan);
    if(!entry)return {false};
    std::vector<Packet> preflight;
    const auto spirit=world.spawn_quickening_zephyr_spirit(owner,plan,std::move(*entry),now,preflight);
    if(!spirit)return {false};
    return {true,{}, {},[&world,scope=*spirit,now]{
        world.publish_quickening_zephyr_spirit(scope,now);
    }};
}
}

