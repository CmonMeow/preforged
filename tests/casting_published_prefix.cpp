// Detached regression: register only after the coordinated source freeze.
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::casting;
int main()try{
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.learned[65]=1;
    Slots bar{};bar[0]={65,0};attributes::Ranks ranks{};
    vitals::Resources resources{100,100,60,60};const effects::Scope target{1,43,1};
    std::vector<Packet> events;unsigned calls=0,published=0;
    const auto finish=terminal(58,42);
    const auto body=*skill_success_body_effect(42,65);
    const auto marker=generic(21,43,1234);
    Hooks hooks;hooks.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
    hooks.resources=[&](unsigned actor)->std::optional<vitals::Resources>{return actor==42||actor==43?std::optional(resources):std::nullopt;};
    hooks.consume_energy=[&](unsigned amount){if(resources.energy<amount)return false;resources.energy-=amount;return true;};
    hooks.target_scope=[&](unsigned actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
    hooks.mimicry_target_valid=[&](Slot slot,effects::Scope scope,Milliseconds){return slot==Slot{65,0}&&scope==target;};
    hooks.before_spell_effect=[](unsigned,std::uint64_t event,Milliseconds){return event!=0;};
    hooks.mimicry=[&](Slot,effects::Scope scope,Milliseconds now){
        check(scope==target&&now==2100,"completion identity/time");++calls;
        // Model a World completion that already published its native prefix,
        // while retaining a deferred effect publication for Runtime to release.
        hooks.events(std::vector<Packet>{finish,body});
        return MimicryCompletion{EffectCompletion{true,{}, {},[&]{++published;hooks.events(std::vector<Packet>{marker});},false,true},false};
    };
    Runtime runtime(42);
    check(runtime.start(100,{65,0,43},bar,policy,ranks,true,hooks)==casting::Reason::accepted,"real Runtime admission");
    runtime.update(2099,hooks);check(!calls&&!published,"premature completion");
    runtime.update(2100,hooks);check(calls==1&&published==1,"effect/publication exactly once");
    check(std::count(events.begin(),events.end(),finish)==1,"duplicate native58 after published prefix");
    check(std::count(events.begin(),events.end(),body)==1,"duplicate caster body after published prefix");
    const auto f=std::find(events.begin(),events.end(),finish),b=std::find(events.begin(),events.end(),body),m=std::find(events.begin(),events.end(),marker);
    check(f<b&&b<m,"prefix must precede deferred effect publication");
    runtime.update(3000,hooks);check(calls==1&&published==1,"completion replay");
    std::cout<<"PASS Runtime published completion prefix ownership\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
