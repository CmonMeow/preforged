#pragma once
#include "gw_effect_registry.h"
#include "gw_combat.h"
#include "gw_hex_duration.h"
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace fw::gw::skills::effects {
struct BondsOfTormentPlan {
    unsigned skill=1881;
    // Fixed DoA packet1073163 publishes a 30.0-second effect on target39.
    // The older wiki mirror calls it indefinite; follow the observed packet.
    Time duration=30000;
    auto operator<=>(const BondsOfTormentPlan&)const=default;
};
inline std::optional<BondsOfTormentPlan> bonds_of_torment_npc(){
    const auto* row=find_skill(1881);
    if(!row||row->profession!=0||row->attribute!=51||row->type!=4||
       row->energy_cost()!=10||row->activation!=1.0f||row->aftercast!=.75f||
       row->recharge!=20||row->duration0!=30||row->duration15!=30||
       row->target_body_resource!=2077)return {};
    return BondsOfTormentPlan{};
}

// This ledger tracks only an already-committed hex instance. World must own
// Registry application, combat preflight and the eventual damage transaction.
class BondsOfTorment {
public:
    struct Link {
        Scope caster,target;
        unsigned effect=0;
        Time applied=0,expires=0;
        auto operator<=>(const Link&)const=default;
    };
    struct Intent {
        Scope caster,target;
        unsigned effect=0;
        Time at=0;
        combat::Kind kind=combat::Kind::cold;
        double cold_damage=0;
    };
private:
    using Key=std::pair<Scope,Scope>;
    std::map<Key,Link> links_;
    std::map<Scope,std::pair<std::uint64_t,Time>> last_damage_event_;
    static bool valid(Scope scope){return scope.instance&&scope.actor&&scope.generation;}
public:
    bool add(const BondsOfTormentPlan& plan,const Link& link){
        if(bonds_of_torment_npc()!=plan||!valid(link.caster)||!valid(link.target)||
           link.caster.instance!=link.target.instance||link.caster==link.target||
           !link.effect||link.applied>std::numeric_limits<Time>::max()-plan.duration||
           link.expires!=link.applied+plan.duration||links_.size()>=4096)return false;
        return links_.emplace(Key{link.caster,link.target},link).second;
    }
    bool erase(Scope caster,Scope target,unsigned effect){
        const auto it=links_.find({caster,target});
        if(it==links_.end()||it->second.effect!=effect)return false;
        links_.erase(it);
        return true;
    }
    bool erase_effect(Scope target,unsigned effect){
        for(auto it=links_.begin();it!=links_.end();++it)
            if(it->second.target==target&&it->second.effect==effect){links_.erase(it);return true;}
        return false;
    }
    void discard(Scope actor){
        for(auto it=links_.begin();it!=links_.end();)
            if(it->second.caster==actor||it->second.target==actor)it=links_.erase(it);
            else ++it;
        last_damage_event_.erase(actor);
    }
    std::size_t expire(Time now){
        std::size_t count=0;
        for(auto it=links_.begin();it!=links_.end();)
            if(it->second.expires<=now){it=links_.erase(it);++count;}
            else ++it;
        return count;
    }
    // World supplies unique, increasing committed damage-event IDs and the
    // post-protection Health loss. Return cold-damage intents without mutating
    // recipients. Caller revalidates target lifetime/hex and applies armor.
    std::optional<std::vector<Intent>> propagate(Scope caster,Time now,
                                                 std::uint64_t event,double applied_loss){
        if(!valid(caster)||!event||!std::isfinite(applied_loss)||applied_loss<=0)return {};
        const auto previous=last_damage_event_.find(caster);
        if(previous!=last_damage_event_.end()&&
           (event<=previous->second.first||now<previous->second.second))return {};
        std::vector<Intent> out;
        for(const auto& [key,link]:links_)
            if(link.caster==caster&&link.applied<=now&&now<link.expires)
                out.push_back({caster,link.target,link.effect,now,combat::Kind::cold,applied_loss});
        last_damage_event_[caster]={event,now};
        return out;
    }
    std::size_t size()const{return links_.size();}
    std::vector<Link> pending()const{
        std::vector<Link> out;out.reserve(links_.size());
        for(const auto& [key,link]:links_)out.push_back(link);
        return out;
    }
};

struct BondsOfTormentApplication {
    Registry registry;
    BondsOfTorment ledger;
    Applied change;
    Entry entry;
};
// Pure two-state proposal: a failed registry application or ledger admission
// must not remove the old link. World commits both copies at one event frontier.
inline std::optional<BondsOfTormentApplication> prepare_bonds_of_torment_application(
        const Registry& registry,const BondsOfTorment& ledger,Scope caster,Scope target,
        unsigned effect,const BondsOfTormentPlan& plan,Time now,
        std::optional<HexDuration> adjusted={}){
    const auto canonical=bonds_of_torment_npc();
    const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
    if(!canonical||plan!=*canonical||!valid(caster)||!valid(target)||
       caster.instance!=target.instance||caster==target||!registry.matches(target)||
       !effect||registry.lookup(target,effect))return {};
    const auto duration=adjusted?adjusted->for_hex(1881,plan.duration,caster.actor):
                                 std::optional(plan.duration);
    if(!duration||!*duration||now>UINT64_MAX-*duration)return {};
    const Entry entry{effect,caster.actor,1881,0,Category::hex,now,now+*duration};
    auto proposed_registry=registry;
    auto change=proposed_registry.apply(target,entry,Replace::same_skill);
    if(!change.accepted)return {};
    auto proposed_ledger=ledger;
    for(const auto& old:change.removed)
        if(old.skill==1881)proposed_ledger.erase_effect(target,old.instance);
    if(!proposed_ledger.add(plan,{caster,target,effect,now,now+*duration}))return {};
    return BondsOfTormentApplication{std::move(proposed_registry),
                                    std::move(proposed_ledger),std::move(change),entry};
}
}
