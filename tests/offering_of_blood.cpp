#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Offering of Blood regression line "+std::to_string(at.line()));
}
int main()try{
    using namespace casting;
    for(unsigned rank=0;rank<=20;++rank)for(unsigned health:{50u,500u}){
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned[146]=1;
        Slots bar{};bar[0]={146,0};attributes::Ranks ranks{};ranks[4]=rank;
        vitals::Resources body{health,500,10,40};unsigned gains=0,sacrifices=0;std::vector<Packet> packets;Hooks hooks;
        hooks.resources=[&](auto id)->std::optional<vitals::Resources>{return id==42?std::optional(body):std::nullopt;};
        hooks.events=[&](const auto& p){packets.insert(packets.end(),p.begin(),p.end());};
        hooks.target_valid=[](auto id,float,bool self){return id==42&&self;};
        hooks.consume_energy=[&](unsigned n){check(n==1);body.energy-=n;return true;};
        hooks.restore_energy=[&](unsigned n){check(n==scaled(8,20,rank));++gains;body.energy+=std::min(n,body.max_energy-body.energy);return true;};
        hooks.health_sacrifice=[&](unsigned id,Milliseconds now)->std::optional<sacrifice::Loss>{check(id==146&&now==350);++sacrifices;auto loss=sacrifice::resolve(id,body.health,body.max_health);body.health=static_cast<unsigned>(loss->remaining);return loss;};
        Runtime runtime(42);check(runtime.start(100,{146,0,999},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&body.energy==9);
        runtime.update(349,hooks);check(!gains&&!sacrifices&&body.health==health);
        runtime.update(350,hooks);check(gains==1&&sacrifices==1&&body.health==(health==50?0u:400u)&&body.energy==9+scaled(8,20,rank));
        check(std::count(packets.begin(),packets.end(),generic(21,42,261))==1);
        check(std::count(packets.begin(),packets.end(),slot_event(228,42,{146,0},15))==1);
        body.health=500;check(runtime.start(1100,{146,0,42},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
    }
    {
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned[146]=1;Slots bar{};bar[0]={146,0};attributes::Ranks ranks{};ranks[4]=15;
        vitals::Resources body{500,500,40,40};unsigned sacrifices=0,gains=0;Hooks h;
        h.events=[](const auto&){};h.resources=[&](auto)->std::optional<vitals::Resources>{return body;};h.target_valid=[](auto,float,bool){return true;};
        h.consume_energy=[&](auto n){body.energy-=n;return true;};h.restore_energy=[&](auto n){++gains;body.energy+=std::min(n,body.max_energy-body.energy);return true;};
        Runtime absent_sacrifice(42);check(absent_sacrifice.start(100,{146,0,42},bar,policy,ranks,true,h)==casting::Reason::unsupported&&body.energy==40);
        h.health_sacrifice=[&](auto skill,auto)->std::optional<sacrifice::Loss>{++sacrifices;auto r=sacrifice::resolve(skill,body.health,body.max_health);body.health=static_cast<unsigned>(r->remaining);return r;};
        Runtime full(42);check(full.start(100,{146,0,42},bar,policy,ranks,true,h)==casting::Reason::accepted);full.update(350,h);
        check(body.energy==40&&body.health==400&&gains==1&&sacrifices==1);
        Runtime cancelled(42);check(cancelled.start(1100,{146,0,42},bar,policy,ranks,true,h)==casting::Reason::accepted);cancelled.cancel(h);cancelled.update(1350,h);
        check(body.energy==39&&body.health==400&&gains==1&&sacrifices==1);
        h.restore_energy=[](auto){return false;};Runtime failed(42);check(failed.start(2100,{146,0,42},bar,policy,ranks,true,h)==casting::Reason::accepted);failed.update(2350,h);
        check(sacrifices==1&&body.health==400); // Failed gain cannot be reported as success/sacrifice.
    }
    std::cout<<"Offering of Blood rank scaling, cost, energy cap, lethal sacrifice, retained recharge and cancellation PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
