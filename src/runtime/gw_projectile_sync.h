#pragma once
#include <cstdint>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <vector>
namespace fw::gw::projectiles {
// Private allocation policy for the opaque uint32 key retained by native
// projectile trigger14. The client compares keys; it does not index by them.
class SynchronizationTokens {
 std::uint32_t next_=1;
public:
 explicit SynchronizationTokens(std::uint32_t next=1):next_(next){if(!next)throw std::invalid_argument("zero projectile token seed");}
 std::optional<std::vector<std::uint32_t>> reserve(unsigned count,std::span<const std::uint32_t> live){
  if(!count||count>4096||live.size()>4096)return {};
  std::set<std::uint32_t> used;for(auto key:live)if(key)used.insert(key);
  auto next=next_;std::vector<std::uint32_t> result;
  for(unsigned i=0;i<count;++i){
   while(used.contains(next)){if(++next==0)next=1;}
   result.push_back(next);used.insert(next);if(++next==0)next=1;
  }
  next_=next;return result;
 }
};
}
