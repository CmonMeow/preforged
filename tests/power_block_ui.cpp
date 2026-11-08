#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw::skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Power Block UI regression line "+std::to_string(at.line()));}
int main()try{using namespace casting;
 {
  Runtime runtime(2);Hooks hooks;std::vector<Packet> sent;Slots bar{};bar[0]={23,0};bar[1]={23,1};bar[2]={28,0};bar[3]={14,0};
  hooks.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
  hooks.current_skillbar=[&]()->std::optional<Slots>{return bar;};
  hooks.skill_disabled_until=[](unsigned skill,auto)->std::optional<Milliseconds>{return skill==23||skill==28?10200:0;};
  runtime.update(200,hooks);check(sent.size()==3);
  check(std::count(sent.begin(),sent.end(),slot_event(228,2,{23,0},10))==1&&std::count(sent.begin(),sent.end(),slot_event(228,2,{23,1},10))==1);
  runtime.update(201,hooks);check(sent.size()==3);
  bar[4]={23,2};runtime.update(1000,hooks);check(sent.size()==4&&sent.back()==slot_event(228,2,{23,2},10));
  runtime.update(10200,hooks);check(sent.size()==8);
  for(unsigned copy:{0u,1u,2u})check(std::count(sent.begin(),sent.end(),slot_event(229,2,{23,copy}))==1);
 }
 for(bool interrupted:{false,true}){
  Runtime runtime(1);Hooks hooks;std::vector<Packet> sent;Slots bar{};bar[0]={281,0};Milliseconds until=0;unsigned costs=0,heals=0;
  hooks.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
  hooks.current_skillbar=[&]()->std::optional<Slots>{return bar;};
  hooks.skill_disabled_until=[&](unsigned,auto)->std::optional<Milliseconds>{return until;};
  hooks.resources=[](unsigned)->std::optional<fw::gw::vitals::Resources>{return fw::gw::vitals::Resources{100,500,100,100};};
  hooks.consume_energy=[&](unsigned amount){check(amount==5);++costs;return true;};
  hooks.target_valid=[](unsigned id,float range,bool self){return id==1&&range==1248&&self;};
  hooks.heal=[&](unsigned,unsigned,float,bool){++heals;return true;};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[281]=1;fw::gw::attributes::Ranks ranks{};
  check(runtime.start(100,{281,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
  until=10200;runtime.update(200,hooks);
  if(interrupted)runtime.interrupt(200,hooks);else runtime.update(1100,hooks);
  check(heals==(interrupted?0u:1u));
  check(std::count(sent.begin(),sent.end(),slot_event(228,1,{281,0},2))==0);
  hooks.skill_disabled_until={}; // Inspect retained Runtime deadline independently of the admission hook.
  check(runtime.start(4000,{281,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge&&costs==1);
  runtime.update(10200,hooks);check(runtime.start(10200,{281,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&costs==2);
 }
 std::cout<<"PASS disable UI for every copy,new bar slots,no duplicate tick publication,expiry and longer deadlines preserved across completion/interrupt\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

