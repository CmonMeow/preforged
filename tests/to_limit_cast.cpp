#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills;using namespace effects;
std::string context;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("To the Limit cast line "+std::to_string(at.line())+context);
}
int main()try{
    for(unsigned rank:{0u,12u,20u})for(bool pvp:{false,true})for(unsigned mode=0;mode<7;++mode){
        context=" rank="+std::to_string(rank)+" pvp="+std::to_string(pvp)+" mode="+std::to_string(mode);
        world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);const auto source=*w.effect_scope(1);
        Slots pools{};pools[0]={336,0};check(w.configure_adrenaline(source,pools,100));
        casting::Runtime cast(1);casting::Hooks hooks;std::vector<Packet> wire;unsigned payments=0,completions=0;bool stale=false;
        hooks.resources=[&](auto id){return w.resources(id);};hooks.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};
        hooks.consume_energy_at=[&](auto cost,auto at){++payments;if(mode==2)return false;const auto paid=w.consume_energy(1,cost,source,at);if(mode==3)stale=true;return paid;};
        hooks.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(stale&&scope)++scope->generation;return scope;};
        hooks.target_valid=[](auto id,float,bool){return id==1;};
        if(mode!=1)hooks.to_limit=[&](auto owner,const auto& plan,auto at){++completions;check(owner==source);return mode!=4&&w.complete_to_limit(owner,plan,at);};
        if(mode==6)hooks.target_scope={};
        Policy policy;policy.primary=1;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[316]=1;
        Slots bar{};bar[0]={316,0};attributes::Ranks ranks{};ranks[21]=mode==5?21:rank;
        const auto accepted=cast.start(100,{316,0,2},bar,policy,ranks,true,hooks);
        context+=" result="+std::to_string(static_cast<int>(accepted))+" payments="+std::to_string(payments)+" completions="+std::to_string(completions);
        if(mode==1||mode==5||mode==6){check(accepted==casting::Reason::unsupported&&payments==0&&completions==0&&w.resources(1)->energy==100);continue;}
        if(mode==2){check(accepted==casting::Reason::energy&&payments==1&&completions==0&&w.resources(1)->energy==100);continue;}
        check(accepted==casting::Reason::accepted&&!cast.active()&&payments==1&&w.resources(1)->energy==95);
        check(completions==unsigned(mode!=3));
        if(mode==3||mode==4){check(w.resources(1)->max_health==500&&w.adrenaline(source)->front().units==0);continue;}
        const auto plan=*to_limit(rank);check(w.resources(1)->health==400+plan.health_bonus&&w.adrenaline(source)->front().units==25);
        const auto entries=*w.status_snapshot(source,100);check(entries.size()==1&&entries[0].skill==316&&entries[0].expires==100+plan.duration);
        check(std::count(wire.begin(),wire.end(),casting::generic(48,1,316))==1);
        check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{316,0},15))==1);
        check(std::count(wire.begin(),wire.end(),casting::terminal(58,1))==0); // Native instant shout has no cast-finish event.
        cast.update(101,hooks);check(completions==1&&payments==1&&w.adrenaline(source)->front().units==25);
        check(cast.start(101,{316,0,2},bar,policy,ranks,true,hooks)==casting::Reason::recharge&&payments==1);
        check(w.settle_resources(source,100+plan.duration)&&w.resources(1)->max_health==500);
    }
    {world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);const auto source=*w.effect_scope(1);
        casting::Runtime cast(1);casting::Hooks hooks;unsigned heals=0;
        hooks.events=[](const auto&){};hooks.resources=[&](auto id){return w.resources(id);};hooks.consume_energy_at=[&](auto cost,auto at){return w.consume_energy(1,cost,source,at);};
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};hooks.target_valid=[](auto id,float,bool){return id==1;};hooks.heal=[&](auto,auto,float,bool){++heals;return true;};
        hooks.to_limit=[&](auto owner,const auto& plan,auto at){return w.complete_to_limit(owner,plan,at);};
        Policy policy;policy.primary=1;policy.secondary=3;policy.campaigns=255;policy.learned[316]=policy.learned[281]=1;Slots bar{};bar[0]={316,0};bar[1]={281,0};attributes::Ranks ranks{};
        check(cast.start(100,{281,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&cast.active());
        check(cast.start(200,{316,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&cast.active());
        check(w.resources(1)->max_health==510&&heals==0);cast.update(1100,hooks);check(!cast.active()&&heals==1&&w.resources(1)->energy==90);
        check(!casting::Runtime::knockdown_restricted(316));}
    std::cout<<"PASS42 To the Limit normal Runtime/World rank/mode/payment/scope/failure cases,instant activation,15s recharge,no duplicate effect and preserved concurrent cast\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
