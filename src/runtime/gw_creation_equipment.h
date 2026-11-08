#pragma once
#include "gw_item_wire.h"
#include <array>
#include <limits>
#include <span>

namespace fw::gw::creation {
struct StarterItem { std::uint8_t slot; items::Definition definition; };
// Owned by one preview connection. Never references an account's inventory.
// Catalog definitions provide content; this session assigns transient IDs.
class Equipment {
    std::array<std::uint32_t,9> slots_{};
    std::uint32_t next_id_=1;
    bool initialized_=false;
    bool inventory_exists_=false;
public:
    explicit Equipment(bool inventory_exists=false):inventory_exists_(inventory_exists){}
    using Batch=std::vector<std::vector<std::uint8_t>>;
    Batch replace(std::span<const StarterItem> catalog){
        if(catalog.empty()||catalog.size()>slots_.size())throw std::invalid_argument("invalid starter equipment count");
        std::array<std::uint32_t,9> replacement{};
        auto next=next_id_;Batch definitions,removals,placements,events,batch;
        for(const auto& entry:catalog){
            if(entry.slot>=slots_.size()||replacement[entry.slot])throw std::invalid_argument("duplicate or invalid starter slot");
            if(next==std::numeric_limits<std::uint32_t>::max())throw std::overflow_error("preview item ID exhausted");
            auto item=entry.definition;item.id=next++;replacement[entry.slot]=item.id;
            definitions.push_back(items::definition_2022(item));
            placements.push_back(items::equip_2022(1,item.id,1,entry.slot));
        }
        for(std::size_t slot=0;slot<slots_.size();++slot){
            // Retail placement asserts if the destination still contains the
            // previous item. Remove it before placing the replacement.
            if(slots_[slot])removals.push_back(items::equipment_change_2022(0,slots_[slot]));
            if(replacement[slot])events.push_back(items::equipment_change_2022(replacement[slot],0));
        }
        if(!initialized_){
            // SMSG 323: inventory WORD, local/remote BYTE. Local precedes bag.
            if(!inventory_exists_)batch.push_back({0x43,1,1,0,0});
            batch.push_back(items::equipment_bag_2022(1,1));
        }
        for(auto* packets:{&definitions,&removals,&placements,&events})
            for(auto& packet:*packets)batch.push_back(std::move(packet));
        // Commit only after every definition and placement validated.
        slots_=replacement;next_id_=next;initialized_=true;return batch;
    }
    const auto& slots()const{return slots_;}
};
}
