#pragma once
#include <cstdint>
#include <vector>
#include <string_view>
#include <stdexcept>
#include "gw_character_name.h"
namespace fw::gw::player_names {
struct Record {uint32_t player=0,actor=0,appearance=0;uint8_t secondary=0;uint32_t field4=0,field5=0;};
// Native88 is a full-record update, not an agent spawn. Preserve every field
// previously published for this player; original792080 replaces its name.
inline std::vector<uint8_t> update_2022(const Record& r,std::u16string_view name){
 if(!r.player||!r.actor||name.empty()||name.size()>31||r.secondary>10)throw std::invalid_argument("invalid player name update");
 (void)fw::gw::character_name_to_utf8(name,31);
 std::vector<uint8_t> p;const auto append=[&](uint32_t v,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(static_cast<uint8_t>(v>>(8*i)));};
 append(88,2);append(r.player,4);append(r.actor,4);append(r.appearance,4);append(r.secondary,1);append(r.field4,4);append(r.field5,4);append(static_cast<uint32_t>(name.size()),2);for(auto c:name)append(c,2);return p;
}
}
