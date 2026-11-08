#pragma once
#include "gw_heal_effects.h"
#include "gw_npc_action_control.h"
#include <string>
namespace fw::gw::combat::ai::friendly {
// An explicit profile is required; capture loadout observations do not supply
// unknown NPC Energy, attribute ranks, or a retail target-priority policy.
struct HealProfile {unsigned skill=0,rank=0,divine=0;std::string evidence;unsigned copy=0;bool operator==(const HealProfile&)const=default;};
struct Healer {
    skills::effects::Scope scope;Health health;unsigned energy=0;
    std::uint64_t health_clock=0,energy_clock=0;bool friendly=false;
};
struct Recipient {
    skills::effects::Scope scope;Health health;std::uint64_t health_clock=0;
    float distance=0;bool allied=false,same_plane=false,line_of_sight=false,healing_allowed=false;
};
inline bool scope_valid(skills::effects::Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
inline bool admissible(const HealProfile& profile,const Healer& source,const Recipient& target,std::uint64_t now,bool selection=true){
    const auto heal=skills::effects::direct_heal(profile.skill);const auto* row=skills::find_skill(profile.skill);
    if(!heal||!row||row->type!=5||profile.rank>20||profile.divine>20||profile.evidence.empty()||
       row->health_sacrifice||row->adrenaline||!scope_valid(source.scope)||!scope_valid(target.scope)||
       source.scope.instance!=target.scope.instance||!source.friendly||!target.allied||!target.same_plane||
       !target.line_of_sight||!target.healing_allowed||!valid(source.health)||source.health.current<=0||
       !valid(target.health)||target.health.current<=0||(selection&&target.health.current>=target.health.maximum)||
       now<source.health_clock||now<source.energy_clock||now<target.health_clock||
       !std::isfinite(target.distance)||target.distance<0||target.distance>heal->range||
       (!heal->self&&source.scope==target.scope)||source.energy<row->energy_cost())return false;
    return true;
}
struct HealQuote {HealProfile profile;skills::effects::Scope source,target;std::optional<npcs::control::Token> paid;};
inline std::optional<HealQuote> bind(HealQuote quote,const npcs::control::Token& token){
    if(quote.paid||!token.sequence||token.actor!=quote.source||token.kind!=npcs::control::Action::spell||
       !token.skill||*token.skill!=skills::Slot{quote.profile.skill,quote.profile.copy}||token.completes<=token.begins)return {};
    quote.paid=token;return quote;
}
inline std::optional<HealQuote> prepare(const HealProfile& profile,const Healer& source,const Recipient& target,std::uint64_t now){
    if(!admissible(profile,source,target,now))return {};
    return HealQuote{profile,source.scope,target.scope};
}
// Caller must supply the actual owned completed action, after fresh resource
// settlement and reactions. This helper neither spends Energy nor writes Health.
// World must commit the amount through LiveActor::heal, never assign a snapshot.
inline std::optional<unsigned> completion(const HealQuote& quote,const npcs::control::Token& token,
        const Healer& source,const Recipient& target,std::uint64_t now,bool owned_completion,const HealProfile& current_profile){
    if(!owned_completion||!quote.paid||*quote.paid!=token||current_profile!=quote.profile||!token.sequence||token.actor!=quote.source||source.scope!=quote.source||
       target.scope!=quote.target||token.kind!=npcs::control::Action::spell||!token.skill||
       token.skill->skill!=quote.profile.skill||now<token.completes)return {};
    const auto* row=skills::find_skill(quote.profile.skill);if(!row)return {};
    auto paid=source;paid.energy=row->energy_cost();
    if(!admissible(quote.profile,paid,target,now,false))return {};
    if(target.health.current==target.health.maximum)return 0;
    return skills::effects::direct_heal_amount_precise(quote.profile.skill,quote.profile.rank,quote.profile.divine,
        target.health.current,static_cast<unsigned>(target.health.maximum));
}
}
