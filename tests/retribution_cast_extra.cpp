// Detached follow-up regression target; intentionally not registered during build freeze.
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;using namespace skills;using namespace skills::casting;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Retribution extra line "+std::to_string(at.line()));
}
struct Fixture {
 Runtime runtime{42};Hooks hooks;Policy policy;Slots bar{};attributes::Ranks ranks{};
 effects::Scope target{1,43,1};vitals::Resources resources{100,100,100,100};
 bool available=true,valid=true,accept=true;unsigned paid=0,admissions=0,completed=0;
 std::vector<Packet> packets;std::vector<std::uint64_t> events;
 std::optional<Milliseconds> snapshot;
 Fixture(){
  policy.primary=3;policy.campaigns=255;policy.learned[248]=1;bar[0]={248,0};ranks[14]=12;
  hooks.events=[this](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  hooks.resources=[this](auto)->std::optional<vitals::Resources>{return available?std::optional(resources):std::nullopt;};
  hooks.consume_energy=[this](unsigned cost){check(cost==10);++paid;resources.energy-=cost;return true;};
  hooks.target_scope=[this](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
  hooks.retribution_target_valid=[this](auto scope,unsigned divine,auto){check(scope==target&&divine==0);return valid;};
  hooks.before_spell_effect=[this](unsigned skill,std::uint64_t event,auto){check(skill==248&&event);++admissions;events.push_back(event);return true;};
  hooks.retribution_completed=[this](auto,const auto&,unsigned,std::uint64_t event,auto){
   check(!events.empty()&&event==events.back());++completed;EffectCompletion result{accept};result.effect_snapshot_at=snapshot;return result;
  };
 }
 Fixture(const Fixture&)=delete;Fixture& operator=(const Fixture&)=delete;
 casting::Reason start(Milliseconds now=100){return runtime.start(now,{248,0,43},bar,policy,ranks,true,hooks);}
 unsigned count(const Packet& packet)const{return unsigned(std::count(packets.begin(),packets.end(),packet));}
};
int main()try{
 // Required initial capabilities fail before payment.
 for(unsigned missing=0;missing<4;++missing){Fixture f;
  if(missing==0)f.hooks.retribution_completed={};if(missing==1)f.hooks.retribution_target_valid={};
  if(missing==2)f.hooks.before_spell_effect={};if(missing==3)f.hooks.target_scope={};
  check(f.start()==casting::Reason::unsupported);check(!f.paid&&!f.admissions&&!f.completed);
 }
 {Fixture f;f.valid=false;check(f.start()==casting::Reason::target);check(!f.paid);}
 // An interrupted zero-recharge spell never reaches the admission callback.
 for(bool control:{false,true}){Fixture f;std::uint64_t epoch=1;
  if(control){f.hooks.control_epoch=[&](){return std::optional(epoch);};
   f.hooks.control_event_time=[](auto){return std::optional<Milliseconds>{1000};};}
  check(f.start()==casting::Reason::accepted);
  if(control){++epoch;f.runtime.update(2100,f.hooks);}else f.runtime.interrupt(1000,f.hooks);
  f.runtime.update(2100,f.hooks);check(f.paid==1&&!f.admissions&&!f.completed);
  check(f.count(cancellation_packet(42,{248,0}))==1&&f.count(terminal(59,42))==1);
  check(f.count(slot_event(229,42,{248,0}))==1);check(f.start(2101)==casting::Reason::accepted);
 }
 // Death, missing resources and generation retirement cannot spend an admission.
 for(unsigned loss=0;loss<3;++loss){Fixture f;check(f.start()==casting::Reason::accepted);
  if(loss==0)f.resources.health=0;if(loss==1)f.available=false;if(loss==2)++f.target.generation;
  f.runtime.update(2100,f.hooks);check(!f.admissions&&!f.completed);
  check(!f.count(terminal(58,42)));
 }
 // Failed paid completion and spell-failure paths have no success aftercast.
 for(unsigned failure=0;failure<3;++failure){Fixture f;check(f.start()==casting::Reason::accepted);
  if(failure==0)f.accept=false;
  else f.hooks.targeted_spell_failure=[failure](auto,auto,auto,auto){return failure==1?SpellFailure::failed:SpellFailure::rejected;};
  f.runtime.update(2100,f.hooks);check(f.admissions==1&&f.completed==unsigned(failure==0));
  check(!f.count(terminal(58,42)));check(f.start(2101)==casting::Reason::accepted);
 }
 // Observer preparation failure occurs after admission, but before paid effect.
 {Fixture f;unsigned prepared=0;f.hooks.prepare_channeling=[&](auto,auto,auto){++prepared;return false;};
  f.hooks.complete_channeling=[](auto,bool,auto)->bool{throw std::runtime_error("unprepared Channeling completed");};
  check(f.start()==casting::Reason::accepted);f.runtime.update(2100,f.hooks);
  check(prepared==1&&f.admissions==1&&!f.completed);check(f.start(2101)==casting::Reason::accepted);
 }
 // Trusted settlement time drives successful observers, while aftercast uses Runtime time.
 for(Milliseconds at:{Milliseconds(2000),Milliseconds(2200)}){Fixture f;f.snapshot=at;
  std::vector<Milliseconds> observed;Milliseconds channeling_at=0;
  f.hooks.effect_snapshot=[&](auto time)->std::optional<std::vector<effects::Entry>>{observed.push_back(time);return std::vector<effects::Entry>{};};
  f.hooks.restore_energy=[](unsigned){return true;};
  f.hooks.prepare_channeling=[](auto,auto,auto){return true;};
  f.hooks.complete_channeling=[&](auto,bool success,auto time){check(success);channeling_at=time;return true;};
  check(f.start()==casting::Reason::accepted);f.runtime.update(2100,f.hooks);
  check(f.completed==1&&channeling_at==std::max(Milliseconds(2100),at));
  check(std::find(observed.begin(),observed.end(),channeling_at)!=observed.end());
  check(f.start(2849)==casting::Reason::busy);check(f.start(2850)==casting::Reason::accepted);
 }
 std::cout<<"PASS Retribution extra admission/interruption/death/failure/observer chronology\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
