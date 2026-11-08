#pragma once
#include "gw_presearing_collectors.h"
#include "gw_npc_dialogue.h"
#include "gw_inventory_identity.h"

namespace fw::gw::collectors {
inline constexpr std::uint32_t choose_token=0x7e040000,cancel_token=0x7e04ffff;
inline bool collector_name(std::string_view displayed,std::string_view expected){
    constexpr std::string_view suffix=" [Collector]";
    if(displayed.ends_with(suffix))displayed.remove_suffix(suffix.size());
    while(!displayed.empty()&&displayed.back()==' ')displayed.remove_suffix(1);
    return displayed==expected;
}
inline std::u16string ascii(std::string_view value){return {value.begin(),value.end()};}
inline dialogue::Batch open_menu(Menu& state,std::span<const Offer> catalog,const inventory::Profile& profile,
                                 unsigned actor,unsigned map,unsigned primary,std::string_view name,
                                 std::uint64_t revision,std::uint64_t now){
    state.clear();Menu next{actor,map,revision,now+120000,{},{}};std::vector<dialogue::Button> buttons;
    for(const auto& offer:catalog){
        if(offer.map!=map||!collector_name(name,offer.collector)||(offer.profession&&offer.profession!=primary))continue;
        if(!offer.key||offer.key>=0xffff)throw std::invalid_argument("collector dialogue key bound");
        next.collector=offer.collector;next.offers.push_back(offer.key);
        buttons.push_back({ascii(offer.name+" - "+std::to_string(offer.cost)+" "+offer.trophy_name+
            " (carrying "+std::to_string(available(profile,offer))+")"),choose_token+offer.key});
    }
    if(buttons.empty())return {};
    buttons.push_back({u"Goodbye",cancel_token});
    auto packets=dialogue::open(actor,u"Choose an item to exchange for your trophies.",buttons);
    state=std::move(next);return packets;
}
// Every click consumes the menu, including stale or forged clicks. Opening
// another NPC also clears it. Matching a shared model ID is never sufficient.
inline const Offer* select_menu(Menu& state,std::span<const Offer> catalog,unsigned token,unsigned actor,
                                unsigned map,unsigned primary,std::string_view nearby_name,
                                std::uint64_t revision,std::uint64_t now){
    const auto pending=std::move(state);state.clear();
    if(token<=choose_token||token>=cancel_token||!collector_name(nearby_name,pending.collector)||
       !pending.accepts(token-choose_token,actor,map,revision,now))return nullptr;
    for(const auto& offer:catalog)if(offer.key==token-choose_token&&offer.map==map&&offer.collector==pending.collector&&
        (!offer.profession||offer.profession==primary))return &offer;
    return nullptr;
}
struct CommittedExchange {inventory::Batch packets;unsigned reward=0;};
// save(durableCharacter, expectedRevision) returns the committed revision or
// nullopt. It must implement the existing authoritative inventory CAS. All
// projection allocations and packet preparation precede the durable write.
template<class Save>
std::optional<CommittedExchange> commit_exchange(inventory::identity::Session& identities,
    inventory::Profile& character,inventory::Profile& bank,std::uint64_t& revision,
    const Offer& offer,unsigned map,unsigned primary,std::u16string_view owner,Save&& save){
    auto proposed=exchange(character,offer,map,primary,owner);if(!proposed)return {};
    // Exhaustion forbids introducing another owned identity, but a captured
    // stackable reward can merge into an existing owned item without one.
    if(proposed->reward>inventory::identity::owned_last)return {};
    auto prepared=identities.prepare(proposed->profile,bank);
    const auto saved=save(prepared.durable.character,revision);if(!saved)return {};
    if(!identities.commit(std::move(prepared)))throw std::runtime_error("obsolete committed collector projection");
    revision=*saved;character=std::move(prepared.wire.character);bank=std::move(prepared.wire.bank);
    return CommittedExchange{std::move(proposed->packets),proposed->reward};
}
}
