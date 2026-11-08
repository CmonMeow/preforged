#include "gw_skill_casting.h"
#include <iostream>
#include <fstream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Recall feedback line "+std::to_string(at.line()));
}
int main(int argc,char** argv)try{
 check(argc==2);std::ifstream input(argv[1]);check(bool(input));
 std::uint64_t ended=0;std::map<unsigned,std::uint64_t> captured_prior;std::string kind;
 while(input>>kind){if(kind=="end")input>>ended;else {check(kind=="prior");unsigned id;std::uint64_t until;input>>id>>until;check(captured_prior.emplace(id,until).second);}}
 check(ended&&captured_prior.size()==3);
 const effects::Scope owner{1,25,1};Slots bar{};
 const unsigned captured[]{799,876,925,926,927,929,948,949};
 for(unsigned i=0;i<8;++i)bar[i]={captured[i],0};
 for(bool prior_timers:{false,true})for(const std::uint64_t duration:{3000u,10000u}){
  casting::Runtime runtime(25);casting::Hooks hooks;std::vector<combat::Packet> wire;
  bool pending=false;std::map<unsigned,std::uint64_t> prior;
  if(prior_timers)prior=captured_prior;
  hooks.target_scope=[&](auto id)->std::optional<effects::Scope>{return id==25?std::optional{owner}:std::nullopt;};
  hooks.current_skillbar=[&](){return std::optional{bar};};
  hooks.skill_disabled_slot=[&](Slot slot,auto)->std::optional<std::uint64_t>{
   return std::max(prior.contains(slot.skill)?prior.at(slot.skill):0,pending?ended+duration:0);
  };
  hooks.recall_feedback_pending=[&](Slot,auto)->std::optional<bool>{return pending;};
  hooks.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};
  runtime.update(ended-1,hooks);check(wire.size()==(prior_timers?3:0));wire.clear();
  pending=true;runtime.update(ended,hooks);check(wire.empty());
  const effects::RecallFeedbackNotice notice{owner,42,ended,bar};
  const auto reply=runtime.prepare_recall_feedback(notice,ended,hooks);check(reply&&wire.empty());
  std::vector<combat::Packet> expected;
  for(const auto id:captured)if(!prior.contains(id)||prior.at(id)<ended+duration)expected.push_back(casting::slot_event(228,25,{id,0},static_cast<unsigned>(duration/1000)));
  check(reply->packets==expected&&reply->slots.size()==8&&expected.size()==(prior_timers?(duration==3000?5:6):8));
  check(!runtime.consume_recall_feedback(notice,ended,hooks,[](const auto&){return false;}));
  check(runtime.prepare_recall_feedback(notice,ended,hooks)->packets==expected);
  check(runtime.consume_recall_feedback(notice,ended,hooks,[&](const auto& commit){
   check(commit.packets==expected);pending=false;return true;
  }));
  // Use the real authority after acknowledging,not a no-longer-pending stub.
  hooks.skill_disabled_slot=[&](Slot slot,auto)->std::optional<std::uint64_t>{return std::max(prior.contains(slot.skill)?prior.at(slot.skill):0,ended+duration);};
  runtime.update(ended+1,hooks);check(wire.empty());
  auto replacement=owner;++replacement.generation;auto rebound=hooks;
  rebound.target_scope=[&](auto)->std::optional<effects::Scope>{return replacement;};
  auto stale=notice;stale.owner=replacement;
  check(!runtime.prepare_recall_feedback(stale,ended+1,rebound));
 }
 // Expiring an old UI timer during a retained return must not queue a229
 // behind the new228and thereby clear it after the return publication.
 casting::Runtime runtime(25);casting::Hooks hooks;std::vector<combat::Packet> wire;bool pending=false;
 hooks.target_scope=[&](auto)->std::optional<effects::Scope>{return owner;};hooks.current_skillbar=[&](){return std::optional{bar};};
 hooks.skill_disabled_slot=[&](Slot slot,auto)->std::optional<std::uint64_t>{return pending?12000:slot.skill==799?1999:0;};
 hooks.recall_feedback_pending=[&](Slot,auto)->std::optional<bool>{return pending;};
 hooks.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};
 runtime.update(1000,hooks);wire.clear();pending=true;runtime.update(2000,hooks);check(wire.empty());
 const effects::RecallFeedbackNotice notice{owner,1,2000,bar};
 check(runtime.prepare_recall_feedback(notice,2000,hooks)->packets.size()==8);
 std::cout<<"PASS Recall five/eight changed timers,retained synchronization,failed-commit retry,post-commit cache and expiry-clear deferral\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

