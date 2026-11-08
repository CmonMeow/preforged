#include "gw_skill_casting.h"
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace fw::gw;
using namespace fw::gw::skills;
using namespace fw::gw::skills::casting;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main()try{
 Policy assassin;assassin.primary=7;assassin.campaigns=0x1e;assassin.learned[814]=1;
 Slots bar{};bar[0]={814,0};attributes::Ranks ranks{};ranks[31]=12;
 vitals::Resources body{100,1000,20,20};unsigned calls=0;Hooks hooks;std::vector<Packet> packets;
 hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
 hooks.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42?std::optional(body):std::nullopt;};
 hooks.target_valid=[](unsigned actor,float,bool self){return actor==42&&self;};
 hooks.consume_energy=[&](unsigned cost){if(cost!=5||cost>body.energy)return false;body.energy-=cost;return true;};
 hooks.apply_health_regeneration=[&](unsigned target,const regeneration::Plan& plan,Milliseconds now,Milliseconds expiry,float,bool){
  ++calls;check(target==42&&plan.skill==814&&plan.pips==9&&now==1000&&expiry==7000,"paid self effect");return true;};
 Hooks missing=hooks;missing.apply_health_regeneration={};
 auto absent=std::make_unique<Runtime>(42);
 check(absent->start(0,{814,0,999},bar,assassin,ranks,true,missing)==casting::Reason::unsupported&&body.energy==20,
       "missing World hook charged Energy");
 auto runtime=std::make_unique<Runtime>(42);
 const auto began=runtime->start(0,{814,0,999},bar,assassin,ranks,true,hooks);
 if(began!=casting::Reason::accepted||body.energy!=15)std::cerr<<"begin="<<int(began)<<" energy="<<body.energy<<'\n';
 check(began==casting::Reason::accepted&&body.energy==15,
       "start/target/cost");
 runtime->update(999,hooks);check(calls==0,"applied before activation");
 runtime->update(1000,hooks);check(calls==1,"completion missing");
 check(std::find(packets.begin(),packets.end(),activation(42,42,814))!=packets.end(),"native cast activation packet missing");
 std::cout<<"PASS Shadow Refuge paid native cast\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
