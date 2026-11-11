#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include <stdexcept>
namespace capture::quests {
struct Observation {
 std::uint32_t process=0,map=0;
 std::uint64_t tick=0,epoch=0;
 bool snapshot=false;
};
struct Window {std::uint32_t process=0,map=0;std::uint64_t start=0,end=0;};
struct Selection {std::optional<std::size_t> baseline;std::vector<std::size_t> changes;bool context_changed=false;};
inline Selection select(const std::vector<Observation>& rows,Window window){
 if(!window.process||!window.map||window.end<window.start)throw std::invalid_argument("verified interaction window required");
 Selection result;
 std::optional<std::size_t> latest_context;
 // Choose the latest snapshot for this process, then verify the map. Looking
 // only at matching maps could resurrect a stale snapshot after zoning away.
 for(std::size_t i=0;i<rows.size();++i){const auto& row=rows[i];
  if(row.process==window.process&&row.tick<=window.start&&
     (!latest_context||rows[*latest_context].tick<=row.tick))latest_context=i;
  if(row.process==window.process&&row.snapshot&&row.tick<=window.start&&
     (!result.baseline||rows[*result.baseline].tick<=row.tick))result.baseline=i;
 }
 if(result.baseline&&rows[*result.baseline].map!=window.map)result.baseline.reset();
 std::optional<std::uint64_t> epoch;
 if(latest_context&&rows[*latest_context].map==window.map)epoch=rows[*latest_context].epoch;
 if(result.baseline&&epoch&&rows[*result.baseline].epoch!=*epoch)result.baseline.reset();
 for(std::size_t i=0;i<rows.size();++i){const auto& row=rows[i];
  if(row.process!=window.process||row.tick<window.start||row.tick>window.end)continue;
  if(row.map!=window.map||(epoch&&*epoch!=row.epoch)){result.context_changed=true;continue;}
  epoch=row.epoch;result.changes.push_back(i);
 }
 // An interaction spanning an observed zone/instance boundary has no reliable
 // single-instance join; retain neither baseline nor changes as attributed data.
 if(result.context_changed){result.baseline.reset();result.changes.clear();}
 return result;
}
}
