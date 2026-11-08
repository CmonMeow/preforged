#pragma once
#include "gw_skillbar.h"
#include <optional>
#include <array>
#include <set>
namespace fw::gw::skills::effects {
// Revision vector for every independent World disable source.
using DisableRevisions=std::array<std::uint64_t,11>;
using ResetSlots=std::set<std::pair<unsigned,unsigned>>;
inline std::optional<ResetSlots> validated_reset_slots(const std::vector<Slot>& slots){
 ResetSlots result;
 for(const auto slot:slots){const auto* row=find_skill(slot.skill);
  if(!row||!row->id||!row->type||slot.copy>65535||!result.emplace(slot.skill,slot.copy).second)return {};
 }
 return result;
}
}

