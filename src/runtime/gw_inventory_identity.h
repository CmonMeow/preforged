#pragma once
#include "gw_storage.h"
#include <compare>
#include <limits>
#include <utility>

namespace fw::gw::inventory::identity {
// One native ItemContext serves the local character, the account bank and
// visible floor items. Durable IDs belong to independent ownership domains.
// Instance broadcasts can retain a common floor ID in the separate high pool;
// these are private-server allocation policies, not retail ID range claims.
inline constexpr std::uint32_t owned_first=1,owned_last=32767;
inline constexpr std::uint32_t ground_first=32768,ground_last=59999;
inline constexpr std::uint32_t placeholder_first=60000,placeholder_last=60004;
inline constexpr std::uint16_t local_inventory=1;
enum class Domain : std::uint8_t {character,bank};
struct Origin {
    Domain domain=Domain::character;
    std::uint32_t logical=0;
    auto operator<=>(const Origin&)const=default;
};
struct BagOrigin {
    Domain domain=Domain::character;
    std::uint16_t logical=0;
    auto operator<=>(const BagOrigin&)const=default;
};
struct Profiles {Profile character,bank;};

class Session {
    struct State {
        std::uint64_t revision=0;
        std::uint32_t next_item=owned_first;
        std::uint32_t next_bag=1;
        std::map<Origin,std::uint32_t> items;
        std::map<std::uint32_t,Origin> reverse_items;
        std::map<BagOrigin,std::uint16_t> bags;
        std::map<std::uint16_t,BagOrigin> reverse_bags;
        Profiles durable;
    } state_;
    bool initialized_=false;
    static void validate_domains(const Profiles& profiles){
        inventory::validate_character(profiles.character);storage::validate(profiles.bank);
    }
    static Profile& profile(Profiles& p,Domain d){return d==Domain::character?p.character:p.bank;}
    static const Profile& profile(const Profiles& p,Domain d){return d==Domain::character?p.character:p.bank;}
    static std::uint32_t allocate_item(State& state){
        if(state.next_item>owned_last)throw std::overflow_error("session owned wire item identities exhausted");
        return state.next_item++;
    }
    static std::uint16_t allocate_bag(State& state){
        if(state.next_bag>std::numeric_limits<std::uint16_t>::max())throw std::overflow_error("session wire bag identities exhausted");
        return static_cast<std::uint16_t>(state.next_bag++);
    }
    static void bind(State& state,std::uint32_t wire,Origin origin){
        if(!wire||wire>owned_last||!origin.logical||state.items.contains(origin)||state.reverse_items.contains(wire))
            throw std::invalid_argument("duplicate session item binding");
        state.items.emplace(origin,wire);state.reverse_items.emplace(wire,origin);
    }
    static Profile wire_profile(const Profile& logical,Domain domain,const State& state){
        Profile wire;wire.inventory=local_inventory;wire.next_item=state.next_item;wire.weapons=logical.weapons;wire.gold=logical.gold;wire.material_quota=logical.material_quota;
        const auto item_id=[&](std::uint32_t id){return id?state.items.at({domain,id}):0u;};
        for(const auto& [id,value]:logical.bags){auto bag=value;bag.id=state.bags.at({domain,id});bag.container=item_id(bag.container);wire.bags.emplace(bag.id,bag);}
        for(const auto& [id,value]:logical.items){auto item=value;item.definition.id=item_id(id);if(item.bag)item.bag=state.bags.at({domain,item.bag});wire.items.emplace(item.definition.id,std::move(item));}
        for(auto& set:wire.weapons.sets){set.weapon=item_id(set.weapon);set.offhand=item_id(set.offhand);}
        inventory::validate(wire);return wire;
    }
public:
    std::optional<Origin> origin(std::uint32_t wire)const{
        const auto it=state_.reverse_items.find(wire);if(it==state_.reverse_items.end())return {};return it->second;
    }
    struct Prepared {
        Profiles wire,durable;
    private:
        State staged;
        const Session* owner=nullptr;
        std::uint64_t expected_revision=0;
        friend class Session;
    };
    Session()=default;
    Session(const Session&)=delete;Session& operator=(const Session&)=delete;
    // Hydration/reload comes from already authoritative durable records. Stage
    // every allocation first so malformed input/exhaustion cannot change the
    // old registry. Surviving (domain,logical) identities keep their wire IDs;
    // disappeared IDs are retired forever for this connection.
    Profiles hydrate(const Profile& character,const Profile& bank){
        Profiles durable{character,bank};validate_domains(durable);
        State staged=state_;
        std::map<Origin,std::uint32_t> items;
        std::map<std::uint32_t,Origin> reverse_items;
        std::map<BagOrigin,std::uint16_t> bags;
        std::map<std::uint16_t,BagOrigin> reverse_bags;
        for(const auto domain:{Domain::character,Domain::bank}){
            const auto& current=profile(durable,domain);
            for(const auto& [logical,value]:current.items){const Origin origin{domain,logical};const auto found=staged.items.find(origin);
                const auto wire=found==staged.items.end()?allocate_item(staged):found->second;
                items.emplace(origin,wire);reverse_items.emplace(wire,origin);}
            for(const auto& [logical,value]:current.bags){const BagOrigin origin{domain,logical};const auto found=staged.bags.find(origin);
                const auto wire=found==staged.bags.end()?allocate_bag(staged):found->second;
                bags.emplace(origin,wire);reverse_bags.emplace(wire,origin);}
        }
        staged.items=std::move(items);staged.reverse_items=std::move(reverse_items);
        staged.bags=std::move(bags);staged.reverse_bags=std::move(reverse_bags);staged.durable=std::move(durable);
        Profiles wire{wire_profile(staged.durable.character,Domain::character,staged),wire_profile(staged.durable.bank,Domain::bank,staged)};
        if(staged.revision==UINT64_MAX)throw std::overflow_error("session identity revision exhausted");
        ++staged.revision;state_=std::move(staged);initialized_=true;return wire;
    }
    std::uint32_t next_item()const{return state_.next_item;}
    std::uint64_t revision()const{return state_.revision;}
    bool preserves_bank(const Prepared& prepared)const{
        return prepared.owner==this&&prepared.expected_revision==state_.revision&&
            inventory::encode(prepared.durable.bank)==inventory::encode(state_.durable.bank);
    }
    std::optional<Origin> item(std::uint32_t wire)const{const auto found=state_.reverse_items.find(wire);return found==state_.reverse_items.end()?std::nullopt:std::optional(found->second);}
    std::optional<BagOrigin> bag(std::uint16_t wire)const{const auto found=state_.reverse_bags.find(wire);return found==state_.reverse_bags.end()?std::nullopt:std::optional(found->second);}
    std::optional<std::uint32_t> wire_item(Domain domain,std::uint32_t logical)const{const auto found=state_.items.find({domain,logical});return found==state_.items.end()?std::nullopt:std::optional(found->second);}
    std::optional<std::uint16_t> wire_bag(Domain domain,std::uint16_t logical)const{const auto found=state_.bags.find({domain,logical});return found==state_.bags.end()?std::nullopt:std::optional(found->second);}
    // Existing inventory/storage/equipment proposal functions operate on these
    // wire profiles. Inbound IDs are resolved by domain before permission is
    // granted. New split/pickup items must use next_item() and advance their
    // profile next_item; newly introduced wire IDs must form a contiguous run.
    // The caller must persist ONLY result.durable in its owned CAS, then commit
    // this registry and install result.wire before publishing proposal packets.
    Prepared prepare(const Profile& character,const Profile& bank)const{
        if(!initialized_)throw std::logic_error("session identities not hydrated");
        Profiles proposed{character,bank};validate_domains(proposed);
        if(character.inventory!=local_inventory||bank.inventory!=local_inventory)
            throw std::invalid_argument("proposal changed local inventory identity");
        if(character.next_item<state_.next_item||bank.next_item<state_.next_item)
            throw std::invalid_argument("proposal regressed session wire item cursor");
        if(character.material_quota!=state_.durable.character.material_quota||bank.material_quota!=state_.durable.bank.material_quota)
            throw std::invalid_argument("item proposal changed reviewed material quota");
        Prepared result;result.owner=this;result.expected_revision=state_.revision;result.staged=state_;
        result.durable=state_.durable;
        for(const auto domain:{Domain::character,Domain::bank}){
            auto& durable=profile(result.durable,domain);durable.items.clear();durable.bags.clear();
            durable.weapons=profile(proposed,domain).weapons;
            durable.gold=profile(proposed,domain).gold;
            durable.material_quota=profile(proposed,domain).material_quota;
        }
        std::map<std::uint32_t,Origin> placement;
        std::set<std::uint32_t> introduced;
        for(const auto domain:{Domain::character,Domain::bank})for(const auto& [wire,value]:profile(proposed,domain).items){
            if(!wire||wire>owned_last||placement.contains(wire))throw std::invalid_argument("duplicate or reserved proposed wire item identity");
            const auto old=state_.reverse_items.find(wire);Origin origin;
            if(old!=state_.reverse_items.end()&&old->second.domain==domain)origin=old->second;
            else{
                if(old==state_.reverse_items.end()){
                    if(wire<state_.next_item)throw std::invalid_argument("proposal reused retired wire item identity");
                    introduced.insert(wire);
                }
                auto& destination=profile(result.durable,domain);
                if(destination.next_item==UINT32_MAX)throw std::overflow_error("durable owned item identities exhausted");
                origin={domain,destination.next_item++};
            }
            placement.emplace(wire,origin);
        }
        auto next=state_.next_item;
        for(auto wire:introduced){if(wire!=next)throw std::invalid_argument("new wire identities must be contiguous");++next;}
        if(character.next_item>next||bank.next_item>next)
            throw std::invalid_argument("proposal advanced unused wire identities");
        result.staged.next_item=next;
        result.staged.items.clear();result.staged.reverse_items.clear();
        for(const auto& [wire,origin]:placement)bind(result.staged,wire,origin);
        std::map<BagOrigin,std::uint16_t> surviving_bags;
        std::map<std::uint16_t,BagOrigin> reverse_bags;
        for(const auto domain:{Domain::character,Domain::bank}){
            const auto& current=profile(proposed,domain);auto& durable=profile(result.durable,domain);
            const auto logical_item=[&](std::uint32_t wire){if(!wire)return 0u;const auto found=placement.find(wire);
                if(found==placement.end()||found->second.domain!=domain)throw std::invalid_argument("cross-owner item reference in proposal");return found->second.logical;};
            for(const auto& [wire,value]:current.bags){const auto found=state_.reverse_bags.find(wire);
                if(found==state_.reverse_bags.end()||found->second.domain!=domain)throw std::invalid_argument("proposal changed bag ownership identity");
                auto logical=value;logical.id=found->second.logical;logical.container=logical_item(logical.container);
                durable.bags.emplace(logical.id,logical);surviving_bags.emplace(found->second,wire);reverse_bags.emplace(wire,found->second);}
            for(const auto& [wire,value]:current.items){auto logical=value;logical.definition.id=logical_item(wire);
                if(logical.bag){const auto owner=state_.reverse_bags.find(logical.bag);
                    if(owner==state_.reverse_bags.end()||owner->second.domain!=domain)throw std::invalid_argument("item placed in foreign ownership bag");
                    logical.bag=owner->second.logical;}
                durable.items.emplace(logical.definition.id,std::move(logical));}
            for(auto& set:durable.weapons.sets){set.weapon=logical_item(set.weapon);set.offhand=logical_item(set.offhand);}
        }
        result.staged.bags=std::move(surviving_bags);result.staged.reverse_bags=std::move(reverse_bags);
        validate_domains(result.durable);result.staged.durable=result.durable;
        if(result.staged.revision==UINT64_MAX)throw std::overflow_error("session identity revision exhausted");
        ++result.staged.revision;
        result.wire={wire_profile(result.durable.character,Domain::character,result.staged),wire_profile(result.durable.bank,Domain::bank,result.staged)};
        return result;
    }
    // No allocations/throws after durable CAS. A prepared object from another
    // session or an obsolete local revision cannot change this registry.
    bool commit(Prepared&& prepared)noexcept{
        if(prepared.owner!=this||prepared.expected_revision!=state_.revision)return false;
        state_=std::move(prepared.staged);prepared.owner=nullptr;return true;
    }
};
}
