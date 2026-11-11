#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace capture {
struct QuestSnapshotTracker {
 std::uint64_t epoch=0,revision=0;std::uint32_t active=0;bool known=false;
 std::map<std::uint32_t,std::string> entries;
 bool observe(std::uint64_t next_epoch,std::uint32_t next_active,const std::map<std::uint32_t,std::string>& next){
  if(known&&epoch==next_epoch&&active==next_active&&entries==next)return false;
  epoch=next_epoch;active=next_active;entries=next;known=true;++revision;return true;
 }
};
struct NpcVisibilityTracker {
 struct State{std::uint32_t model;bool dead;bool operator==(const State&)const=default;};
 struct Delta{std::uint32_t actor,model;bool appeared,dead,status_changed=false;};
 std::uint64_t epoch=0;std::map<std::uint32_t,State> previous;
 std::vector<Delta> observe(std::uint64_t next_epoch,const std::map<std::uint32_t,State>& next){
  std::vector<Delta> result;
  if(epoch!=next_epoch){previous.clear();epoch=next_epoch;}
  for(auto [actor,model]:next){auto old=previous.find(actor);if(old==previous.end()||old->second.model!=model.model){if(old!=previous.end())result.push_back({actor,old->second.model,false,old->second.dead});result.push_back({actor,model.model,true,model.dead});}else if(old->second.dead!=model.dead)result.push_back({actor,model.model,true,model.dead,true});}
  for(auto [actor,model]:previous)if(!next.contains(actor))result.push_back({actor,model.model,false,model.dead});
  previous=next;return result;
 }
};
}
