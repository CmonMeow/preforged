#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;
using namespace skills;
static unsigned scenario=0;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Selfless cast scenario "+std::to_string(scenario)+" line "+std::to_string(at.line()));
}

int main()try{
    // Each alias independently takes the complete paid Runtime -> World route.
    for(unsigned alias:{1952u,2095u})for(unsigned rank:{0u,6u})for(scenario=0;scenario<10;++scenario){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
        w.join(2,"Selected foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{400,500,50,100},100);w.set_vitals(2,{500,500,50,100},100);
        const auto source=*w.effect_scope(1);
        check(w.energy_capacity(1,5,0,100,{80,-2}));
        Slots bar{};bar[0]={alias,0};Policy policy;policy.primary=3;policy.campaigns=255;
        policy.learned[alias]=1;policy.pvp_match=scenario==3;
        attributes::Ranks ranks{};ranks[16]=12;
        casting::Runtime runtime(1);casting::Hooks h;
        unsigned payments=0,completions=0,self_checks=0;
        h.resources=[&](auto id){return w.resources(id);};
        h.events=[&](const auto& packets){box->enqueue(packets);};
        h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.target_valid=[&](unsigned id,float,bool self){++self_checks;check(id==1&&self);return true;};
        h.consume_energy_at=[&](unsigned cost,auto at){check(cost==5);++payments;return w.consume_energy(1,cost,source,at);};
        h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
        h.allegiance_rank=[&](unsigned skill)->std::optional<unsigned>{check(skill==alias);return scenario==7?13u:rank;};
        h.enchantment_duration_ready=[&](unsigned skill,auto base,auto){check(skill==alias&&base==(rank?20000u:15000u));return true;};
        h.selfless_spirit=[&](const effects::SelflessSpiritPlan& plan,unsigned divine,auto at)->casting::EffectCompletion{
            ++completions;check(plan.skill==alias&&plan.rank==rank&&plan.duration_ms==(rank?20000u:15000u));
            const bool ok=w.complete_selfless_spirit(source,plan,divine,at);
            return {ok,{},{},{},false,true};
        };
        if(scenario==4)h.selfless_spirit={};
        if(scenario==6)h.allegiance_rank={};
        const bool denied=scenario==3||scenario==4||scenario==6||scenario==7;
        const auto begun=runtime.start(1000,{alias,0,2},bar,policy,ranks,true,h);
        check(begun==(scenario==3?casting::Reason::pve_only:
            denied?casting::Reason::unsupported:casting::Reason::accepted));
        if(denied){check(payments==0&&completions==0&&w.resources(1)->energy==50);continue;}
        check(payments==1&&w.resources(1)->energy==45&&self_checks>0);
        runtime.update(1249,h);check(completions==0);
        if(scenario==1)runtime.cancel(h);
        if(scenario==2)runtime.interrupt(1249,h);
        if(scenario==5)h.selfless_spirit={};
        if(scenario==8){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1249,1);w.set_vitals(1,{500,500,45,100},100);}
        if(scenario==9)check(w.kill(source,1249));
        box->drain();runtime.update(1250,h);runtime.update(1250,h);
        check(!runtime.active()&&payments==1&&completions==unsigned(scenario==0));
        const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==unsigned(scenario==0));
        check(std::count(packets.begin(),packets.end(),casting::generic(21,1,553))==unsigned(scenario==0));
        if(scenario!=0)continue;
        const auto state=w.selfless_spirit_state(source,1250);check(state&&*state);
        check((**state).rank==rank&&(**state).skill==alias&&(**state).expires==1250+(rank?20000u:15000u));
        check(w.status_snapshot(*w.effect_scope(2),1250)->empty());
        check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{alias,0},45))==1);
        check(runtime.start(2000,{alias,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
        check(runtime.start(46249,{alias,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
        check(payments==1);
    }
    std::cout<<"PASS Selfless aliases/rank0+6 paid Runtime casts, self target, PvP/hooks/rank refusal, cancellation/interruption/lifetime/death, single finish/body and recharge\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
