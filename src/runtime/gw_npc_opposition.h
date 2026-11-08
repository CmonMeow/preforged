#pragma once
#include "gw_npc_combat_catalog.h"
namespace fw::gw::combat::ai::opposition {
using Scope=skills::effects::Scope;
enum class Side {allied,hostile};
struct QuestDomain {Scope owner;unsigned quest=0,cycle=0;bool operator==(const QuestDomain&)const=default;};
struct Ownership {
    std::optional<QuestDomain> source,target,current;
    bool owner_current=false,target_assigned=false;
};
inline bool valid_domain(const QuestDomain& domain){
    return domain.owner.instance&&domain.owner.actor&&domain.owner.actor<=65535&&domain.owner.generation&&domain.quest&&domain.cycle;
}
inline bool authorized(const Ownership& ownership,Scope source,Scope target){
    if(!ownership.source&&!ownership.target)return !ownership.current;
    return ownership.source&&ownership.target&&ownership.current&&ownership.owner_current&&ownership.target_assigned&&
        valid_domain(*ownership.source)&&*ownership.source==*ownership.target&&*ownership.source==*ownership.current&&
        ownership.current->owner.instance==source.instance&&source.instance==target.instance;
}
inline std::optional<Side> living_side(const npcs::LiveActor& actor,const npcs::Lifetime& expected){
    if(!actor.matches(expected))return {};
    const auto* state=actor.actor(expected);
    // Protected services are not combatants merely because they are healable.
    if(!state||!state->damageable||!state->health||state->health->current<=0)return {};
    if(actor.hostile_to_players(expected))return Side::hostile;
    if(actor.healable_ally(expected))return Side::allied;
    return {};
}
class Receipt {
    Scope source_,target_;Side source_side_,target_side_;
    Receipt(Scope source,Scope target,Side a,Side b):source_(source),target_(target),source_side_(a),target_side_(b){}
public:
    static std::optional<Receipt> prepare(const npcs::LiveActor& source,const npcs::Lifetime& a,
            const npcs::LiveActor& target,const npcs::Lifetime& b,const Ownership& ownership){
        const auto x=living_side(source,a),y=living_side(target,b);
        if(!x||!y||*x==*y||source.hex_scope()==target.hex_scope()||source.hex_scope().instance!=target.hex_scope().instance||
           !authorized(ownership,source.hex_scope(),target.hex_scope()))return {};
        return Receipt(source.hex_scope(),target.hex_scope(),*x,*y);
    }
    Scope source()const{return source_;}Scope target()const{return target_;}
    bool current(const npcs::LiveActor& source,const npcs::Lifetime& a,const npcs::LiveActor& target,
            const npcs::Lifetime& b,const Ownership& ownership,bool released=false)const{
        if(!source.matches(a)||!target.matches(b)||source.hex_scope()!=source_||target.hex_scope()!=target_||
           !authorized(ownership,source_,target_))return false;
        const auto recipient=living_side(target,b);if(!recipient||*recipient!=target_side_)return false;
        const auto attacker=living_side(source,a);
        // Already released deliveries retain immutable source allegiance after
        // source death. Lifetime/ownership remain mandatory; no new action starts.
        return attacker?*attacker==source_side_:released&&source.actor(a)&&source.actor(a)->health&&source.actor(a)->health->current<=0;
    }
};
}
