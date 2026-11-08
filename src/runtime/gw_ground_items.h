#pragma once
#include "gw_item_wire.h"
#include "gw_item_metadata.h"
#include "gw_inventory.h"
#include "gw_effect_registry.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <vector>

namespace fw::gw::world::ground {
using Scope=skills::effects::Scope;
using Packet=items::Packet;
using Batch=std::vector<Packet>;
// IDs belong to the client item table, independently of the actor table. The
// caller must reserve a global wire ID, never a per-character inventory ID.
// Pinned type4 registration7cb5c0 ->7cb570 stores the raw item ID, with no
// NPC/player model tag. Callback7cb5d0 links/unlinks item.agent_id.
inline Packet spawn_2022(std::uint32_t actor,std::uint32_t item,float x,float y,std::uint16_t plane){
    if(!actor||actor>65535||!item||item>65535||!std::isfinite(x)||!std::isfinite(y))
        throw std::invalid_argument("invalid ground item spawn");
    Packet out;auto put=[&](std::uint32_t n,unsigned width){items::put(out,n,width);};
    auto real=[&](float n){put(std::bit_cast<std::uint32_t>(n),4);};
    put(32,2);put(actor,4);put(item,4);put(4,1);put(0,1);
    real(x);real(y);put(plane,2);real(1);real(0);put(0,1);
    real(0);real(1);for(unsigned n=0;n<7;++n)put(0,4);
    for(unsigned n=0;n<4;++n)real(0);put(0,2);put(0,4);real(0);real(0);put(0,2);
    return out;
}
// Native308: DWORD item, WORD owner actor, float seconds. Zero releases an
// allocation; this does not delete the item or its ground actor.
inline Packet owner_2022(std::uint32_t item,std::uint32_t owner,float seconds){
    if(!item||item>65535||owner>65535||!std::isfinite(seconds)||seconds<0||seconds>600||(!owner&&seconds!=0))
        throw std::invalid_argument("invalid ground item owner");
    Packet out;items::put(out,308,2);items::put(out,item,4);items::put(out,owner,2);
    items::put(out,std::bit_cast<std::uint32_t>(seconds),4);return out;
}
inline Packet despawn_2022(std::uint32_t actor){
    if(!actor||actor>65535)throw std::invalid_argument("invalid ground actor removal");
    Packet out;items::put(out,33,2);items::put(out,actor,4);return out;
}
struct PickupRequest {std::uint32_t actor=0;bool call_target=false;};
// Original8765b0 and descriptor60b7b0: actor DWORD, call-target BYTE.
inline std::optional<PickupRequest> pickup_request(std::span<const std::uint8_t> p){
    if(p.size()!=7||p[0]!=61||(p[1]&0x7f)||p[6]>1)return {};
    std::uint32_t actor=0;for(unsigned n=0;n<4;++n)actor|=std::uint32_t(p[n+2])<<(8*n);
    if(!actor||actor>65535)return {};return PickupRequest{actor,p[6]!=0};
}
struct Position {float x=0,y=0;std::uint16_t plane=0;};
struct Allocation {Scope owner{};std::uint64_t until=0;};
struct Item {
    Scope scope{},source{};
    std::uint32_t source_item=0;
    items::Definition definition; // Global ground wire identity, not source_item.
    Position position;
    std::uint64_t created=0;
    std::optional<Allocation> allocation;
    std::optional<std::uint8_t> profession;
    std::optional<std::u16string> customized;
    std::optional<inventory::ReviewedArmor> reviewed_armor;
};
inline bool valid_reviewed_armor(const Item& item){
    return !item.reviewed_armor||(item.profession&&*item.profession>=1&&*item.profession<=10&&item.definition.quantity==1&&
        item.reviewed_armor->base<=200&&item.reviewed_armor->physical<=100&&item.reviewed_armor->elemental<=100&&
        (item.definition.type==7||item.definition.type==19||item.definition.type==16||item.definition.type==4||item.definition.type==13));
}
inline bool valid_scope(Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
inline bool valid_position(Position point){return std::isfinite(point.x)&&std::isfinite(point.y);}
inline std::optional<Scope> owner(const Item& item,std::uint64_t now){
    if(item.allocation&&item.created<=now&&now<item.allocation->until)return item.allocation->owner;
    return {};
}
// Range/plane are explicit private-server policy, not an inferred retail
// pickup distance. Default0 denies; the integration must choose a bound.
inline bool can_pickup(const Item& item,Scope picker,Position position,std::uint64_t now,float range){
    if(!valid_scope(picker)||picker.instance!=item.scope.instance||!valid_position(position)||
       now<item.created||!std::isfinite(range)||range<=0||range>512||
       position.plane!=item.position.plane||
       std::hypot(double(position.x)-item.position.x,double(position.y)-item.position.y)>range)return false;
    const auto reserved=owner(item,now);return !reserved||*reserved==picker;
}
inline Batch presentation(const Item& item,std::uint64_t now){
    if(now<item.created)throw std::invalid_argument("ground snapshot before creation");
    Batch result{items::definition_2022(item.definition)};
    if(item.profession)result.push_back(items::profession_2022(item.definition.id,*item.profession));
    if(item.customized&&!item.customized->empty())result.push_back(items::customization_2022(item.definition.id,*item.customized));
    // Match the proved352 -> queued type4 add ->308 path. Allocating before
    // add would additionally require the client's ownership hash lookup.
    result.push_back(spawn_2022(item.scope.actor,item.definition.id,item.position.x,item.position.y,item.position.plane));
    if(const auto reserved=owner(item,now))result.push_back(owner_2022(item.definition.id,reserved->actor,
        static_cast<float>(double(item.allocation->until-now)/1000.0)));
    return result;
}
inline Batch removal(const Item& item){return {despawn_2022(item.scope.actor),items::delete_2022(item.definition.id)};}

// A staged copy is prepared/validated before the caller's persistent CAS.
// Commit it only after that transaction succeeds. There is no generated loot,
// starter item, implicit ownership grant, automatic timeout, or ID reuse.
class State {
    std::map<std::uint32_t,Item> entries_;
    std::set<std::uint32_t> used_actors_,used_items_;
    std::uint64_t revision_=1;
public:
    std::uint64_t revision()const{return revision_;}
    static constexpr std::size_t capacity=64;
    const std::map<std::uint32_t,Item>& entries()const{return entries_;}
    std::size_t size()const{return entries_.size();}
    bool used_actor(std::uint32_t id)const{return used_actors_.contains(id);}
    bool used_item(std::uint32_t id)const{return used_items_.contains(id);}
    const Item* find(Scope scope)const{
        const auto it=entries_.find(scope.actor);return it!=entries_.end()&&it->second.scope==scope?&it->second:nullptr;
    }
    const Item* by_actor(std::uint32_t actor)const{
        const auto it=entries_.find(actor);return it==entries_.end()?nullptr:&it->second;
    }
    bool add(const Item& item){
        if(revision_==UINT64_MAX||entries_.size()>=capacity||!valid_scope(item.scope)||!valid_scope(item.source)||
           item.scope.instance!=item.source.instance||!item.source_item||item.definition.id>65535||!valid_position(item.position)||
           used_actor(item.scope.actor)||used_item(item.definition.id))return false;
        if(item.profession&&*item.profession>11)return false;
        if(!valid_reviewed_armor(item))return false;
        if(item.customized&&(item.customized->size()>31||item.customized->find(u'\0')!=std::u16string::npos))return false;
        if(item.allocation&&(!valid_scope(item.allocation->owner)||
           item.allocation->owner.instance!=item.scope.instance||item.allocation->until<=item.created||
           item.allocation->until-item.created>600000))return false;
        try{(void)items::definition_2022(item.definition);}
        catch(const std::invalid_argument&){return false;}
        entries_.emplace(item.scope.actor,item);used_actors_.insert(item.scope.actor);used_items_.insert(item.definition.id);++revision_;return true;
    }
    bool consume(Scope scope,unsigned quantity){
        auto it=entries_.find(scope.actor);
        if(revision_==UINT64_MAX||it==entries_.end()||it->second.scope!=scope||!quantity||quantity>it->second.definition.quantity)return false;
        if(quantity==it->second.definition.quantity)entries_.erase(it);else it->second.definition.quantity-=quantity;
        ++revision_;return true;
    }
    // Assigned drops become public at10min or when their exact owner leaves.
    // Retain items; assignment expiration is not item expiration.
    Batch release(std::uint64_t now,std::optional<Scope> leaving={}){
        Batch packets;
        for(auto& [id,item]:entries_)if(item.allocation&&
           ((item.created<=now&&now>=item.allocation->until)||(leaving&&item.allocation->owner==*leaving))){
            if(revision_==UINT64_MAX)throw std::overflow_error("ground revision exhausted");
            packets.push_back(owner_2022(item.definition.id,0,0));item.allocation.reset();
            ++revision_;
        }
        return packets;
    }
};
}
