#include "gw_skill_casting.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace casting;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Cooldown layers line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<3;++mode){Runtime runtime(1);Hooks h;std::vector<Packet> wire;Slots bar{};bar[0]={281,0};Milliseconds until=0;unsigned paid=0;
 h.events=[&](const auto& values){wire.insert(wire.end(),values.begin(),values.end());};h.current_skillbar=[&]()->std::optional<Slots>{return bar;};h.skill_disabled_slot=[&](Slot,auto)->std::optional<Milliseconds>{return until;};
 h.resources=[](auto)->std::optional<vitals::Resources>{return vitals::Resources{100,500,100,100};};h.consume_energy=[&](auto cost){check(cost==5);++paid;return true;};h.target_valid=[](auto id,float,bool){return id==1;};h.heal=[](auto,auto,float,bool){return true;};
 Policy p;p.primary=3;p.campaigns=255;p.learned[281]=1;attributes::Ranks ranks{};
 if(mode)check(runtime.start(100,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);
 until=10200;runtime.update(200,h);if(mode)runtime.update(1100,h);
 until=0;const auto at=mode?2000u:300u;wire.clear();runtime.update(at,h);
 if(!mode){check(wire==std::vector<Packet>{slot_event(229,1,{281,0})});check(runtime.start(301,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);}
 else{check(wire==std::vector<Packet>{slot_event(228,1,{281,0},2)});check(runtime.start(2001,{281,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);
 if(mode==2){until=20200;runtime.update(2100,h);wire.clear();runtime.update(3100,h);check(wire.empty());check(runtime.start(3100,{281,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);wire.clear();until=0;runtime.update(3200,h);check(wire==std::vector<Packet>{slot_event(229,1,{281,0})});check(runtime.start(3201,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);}
 else{wire.clear();runtime.update(3100,h);check(wire==std::vector<Packet>{slot_event(229,1,{281,0})});check(runtime.start(3100,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);}}
 }
 // A failed authoritative query must neither publish nor discard the cache.
 {Runtime runtime(1);Hooks h;Slots bar{};bar[0]={281,0};bool valid=true;Milliseconds until=10200;unsigned packets=0;
 h.current_skillbar=[&]()->std::optional<Slots>{return bar;};h.events=[&](const auto& p){packets+=unsigned(p.size());};h.skill_disabled_slot=[&](auto,auto)->std::optional<Milliseconds>{return valid?std::optional(until):std::nullopt;};
 runtime.update(200,h);check(packets==1);valid=false;runtime.update(300,h);check(packets==1);valid=true;until=0;runtime.update(400,h);check(packets==2);runtime.update(401,h);check(packets==2);}
 std::cout<<"PASS World disable removal/shortening preserves independent local recharge,new disable survives base expiry,no duplicate UI clear and failed query atomicity\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
