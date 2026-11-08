#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;using namespace skills;
static void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Natural Stride Runtime edge line "+std::to_string(at.line()));
}
struct Fixture{
 effects::Scope owner{1,1,1};effects::Registry registry{owner};casting::Runtime runtime{1};
 Slots bar{};Policy policy;attributes::Ranks ranks{};casting::Hooks hooks;
 vitals::Resources resources{100,100,30,30};std::vector<Packet> packets;
 unsigned paid=0,starts=0,removals=0;bool snapshot_ready=true;
 Fixture(){
  bar[0]={1727,0};policy.primary=2;policy.campaigns=255;policy.learned[1727]=1;
  hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  hooks.resources=[&](auto)->std::optional<vitals::Resources>{return resources;};
  hooks.target_valid=[](auto actor,float,bool self){return actor==1&&self;};
  hooks.consume_energy_at=[&](unsigned amount,auto){if(resources.energy<amount)return false;resources.energy-=amount;paid+=amount;return true;};
  hooks.begin_action=[&](auto,auto){++starts;};
  hooks.effect_snapshot=[&](auto now)->std::optional<std::vector<effects::Entry>>{if(!snapshot_ready)return {};return registry.active(owner,now);};
  hooks.status_mutation=[&](auto skill,auto id,auto rank,auto now,auto until,bool active){
   if(!active){++removals;registry.remove(owner,id);return true;}
   return registry.observe_applications(owner,1727)&&registry.apply(owner,{id,1,skill,rank,effects::Category::stance,now,until},effects::Replace::category).accepted;
  };
 }
 casting::Reason cast(unsigned long long now=1000){return runtime.start(now,{1727,0,1},bar,policy,ranks,true,hooks);}
};
int main()try{
 {
  Fixture f;check(f.cast()==casting::Reason::accepted);check(f.paid==5&&f.starts==1);
  const auto stance=f.registry.active(f.owner,1000).front();
  check(f.registry.apply(f.owner,{99,2,100,0,effects::Category::hex,1100,5100},effects::Replace::same_skill).accepted);
  auto events=f.registry.take_applications(f.owner);bool ended=false;
  for(const auto& event:events)ended=bool(effects::natural_stride_application(f.registry,f.owner,event))||ended;
  check(ended); // World owns the one removal packet after this committed application.
  f.packets.clear();f.runtime.update(1100,f.hooks);f.runtime.update(2000,f.hooks);
  check(f.removals==0);check(std::count(f.packets.begin(),f.packets.end(),casting::effect_remove(1,stance.instance))==0);
  check(f.cast(2001)==casting::Reason::recharge);check(f.paid==5&&f.starts==1);
 }
 {
  Fixture f;check(f.cast()==casting::Reason::accepted);f.snapshot_ready=false;
  f.runtime.update(1100,f.hooks);check(f.removals==0);
  f.runtime.update(2000,f.hooks);check(f.removals==1);
  f.runtime.update(2001,f.hooks);check(f.removals==1);
 }
 {
  Fixture f;f.resources.energy=4;check(f.cast()==casting::Reason::energy);check(f.paid==0&&f.starts==0&&f.registry.active(f.owner,1000).empty());
  f.resources.energy=30;check(f.cast()==casting::Reason::accepted);check(f.paid==5&&f.starts==1);
 }
 {
  Fixture f;f.hooks.effect_snapshot={};check(f.cast()==casting::Reason::unsupported);check(f.paid==0&&f.starts==0);
 }
 std::cout<<"PASS Natural Stride Runtime early-end reconciliation, unavailable snapshots, exact expiry, energy retry and missing consumer rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

