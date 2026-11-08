#pragma once
#include "gw_hex_breaker_transaction.h"

namespace fw::gw::skills::effects {
enum class HexApplicationKind {applied,prevented};
struct HexRetaliationProposal {HexBreakerReaction reaction;combat::Actor caster;combat::Hit hit;};
struct HexApplicationProposal {
    HexApplicationKind kind;
    Registry registry;
    Applied application;
    std::optional<HexRetaliationProposal> retaliation;
};
// Selected-recipient hex stage,AFTER initial spell effects. A valid prevented
// result is distinct from an invalid transaction:normal cast completion and
// other recipients continue. Only 'applied' may register degeneration,owned
// hex links,end effects or application observers. Do not roll back initial
// effects merely because the hex was prevented.
inline std::optional<HexApplicationProposal> selected_hex_application(const Registry& original,
        Scope owner,Scope caster,const Entry& incoming,Replace replacement,combat::Actor caster_actor,std::optional<unsigned> owner_level,Time now){
    const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
    const auto* row=find_skill(incoming.skill);
    if(!valid(owner)||!valid(caster)||owner.instance!=caster.instance||owner.actor==caster.actor||
       !row||row->type!=4||incoming.kind!=Category::hex||incoming.cause!=caster.actor||!incoming.instance||incoming.rank>20||
       incoming.applied!=now||(incoming.expires&&*incoming.expires<=now)||
       (replacement!=Replace::independent&&replacement!=Replace::same_skill&&replacement!=Replace::category)||
       caster_actor.id!=caster.actor||caster_actor.generation!=caster.generation||(owner_level&&(!*owner_level||*owner_level>255)))return {};
    if(original.newest_skill(owner,10,now)){
        if(!owner_level)return {};
        auto prevented=hex_breaker_transaction(original,owner,caster,caster_actor,incoming.skill,*owner_level,now);
        if(!prevented)return {};
        return HexApplicationProposal{HexApplicationKind::prevented,std::move(prevented->registry),{},
            HexRetaliationProposal{prevented->reaction,std::move(prevented->caster),prevented->retaliation}};
    }
    auto registry=original;auto applied=registry.apply(owner,incoming,replacement);if(!applied.accepted)return {};
    return HexApplicationProposal{HexApplicationKind::applied,std::move(registry),std::move(applied),{}};
}
}
