#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <fstream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
namespace casting=skills::casting;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Energy drain completion line "+std::to_string(at.line()));
}
int main(int argc,char** argv)try{
    check(argc<=2);std::ofstream exported;if(argc==2){exported.open(argv[1]);check(exported.good());}
    unsigned cases=0;
    for(unsigned skill:{39u,40u,42u,79u,80u})for(unsigned renewal:{0u,181u,2860u})
    for(bool npc:{false,true})for(unsigned energy:{0u,3u,20u})for(unsigned mode=0;mode<6;++mode){
        ++cases;world::Instance w;auto box=std::make_shared<world::Outbox>();
        const unsigned victim=npc?100:2;
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={900,1000};d.damageable=d.conditions_allowed=true;
            d.spawn_energy=combat::npcs::Energy{energy,40};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});}
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.public_professions(1,6,5);w.set_vitals(1,{100,1000,50,100},100);
        if(!npc){w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{900,1000,energy,40},100);
            w.join(3,"Nearby foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{900,1000,5,40},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(victim);
        if(renewal){const auto r=*effects::ether_renewal(renewal,15);check(w.status_mutation(source,renewal,80,15,100,100+r.duration,true));}
        const auto plan=*effects::energy_drain(skill,12);std::uint64_t now=100,event=0;bool stale=false;unsigned calls=0;
        std::vector<world::Packet> packets;
        casting::Hooks h;h.events=[&](const auto& ps){packets.insert(packets.end(),ps.begin(),ps.end());box->enqueue(ps);};h.resources=[&](auto actor){return w.resources(actor);};
        h.consume_energy=[&](auto amount){return w.consume_energy(1,amount);};
        h.target_scope=[&](auto actor){auto scope=w.effect_scope(actor);if(scope&&stale&&actor==victim)++scope->generation;return scope;};
        h.projectile_target_valid=[&](auto actor,auto range){return w.projectile_target_valid(source,actor,range,now);};
        h.before_spell_effect=[&](auto id,auto token,auto at){event=token;return mode!=5&&w.before_spell_effect(source,id,token,at);};
        h.energy_drain=[](auto,auto,auto)->casting::EffectCompletion{throw std::runtime_error("drain fallback");};
        h.energy_drain_completed=[&](auto scoped,const auto& p,auto token,auto at){
            ++calls;check(token==event);std::vector<world::Packet> visuals;std::function<void()> publish;
            const bool accepted=w.energy_drain(source,scoped,p,20,at,&visuals,token,&publish);
            if(accepted)check(!w.energy_drain(source,scoped,p,20,at,nullptr,token));
            return casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publish),true};};
        skills::Policy policy;policy.primary=6;policy.secondary=5;policy.campaigns=255;policy.learned[skill]=1;
        skills::Slots slots{};slots[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};ranks[skills::find_skill(skill)->attribute]=12;
        {auto absent=h;absent.target_scope=[](auto)->std::optional<effects::Scope>{return {};};casting::Runtime rejected(1);
            check(rejected.start(100,{skill,0,victim},slots,policy,ranks,true,absent)==casting::Reason::target&&w.resources(1)->energy==50);}
        casting::Runtime runtime(1);check(runtime.start(100,{skill,0,victim},slots,policy,ranks,true,h)==casting::Reason::accepted);
        now=100+static_cast<unsigned>(skills::find_skill(skill)->activation*1000);
        check(w.consume_energy(1,0,source,now));const auto paid=w.resources(1)->energy;
        if(!npc){check(w.settle_resources(target,now)&&w.settle_resources(*w.effect_scope(3),now));w.set_vitals(2,{900,1000,energy,40},100);w.set_vitals(3,{900,1000,5,40},100);}
        if(mode==1)h.energy_drain_completed={};if(mode==2)h.before_spell_effect={};if(mode==3)runtime.cancel(h);if(mode==4)stale=true;
        box->drain();runtime.update(now,h);const bool applied=mode==0;const unsigned lost=std::min(energy,plan.amount);
        check(calls==unsigned(applied));
        const auto remaining=npc?w.npc_energy_snapshot(target)->current:w.resources(victim)->energy;
        check(remaining==energy-(applied?lost:0));
        const double hp=npc?w.npc_combat_snapshot(target)->health->current:w.resources(victim)->health;
        check(hp==900-(applied?lost*plan.damage_per_energy:0));
        check(w.resources(1)->energy==paid+(applied?(renewal?effects::ether_renewal(renewal,15)->energy_per_enchantment:0)+lost*plan.gain_multiplier:0));
        check(w.resources(1)->health==100+(applied?(renewal?20:0)+lost*plan.heal_per_energy:0));
        if(!npc){check(w.resources(3)->health==900-(applied&&plan.radius?lost*plan.damage_per_energy:0));
            check(w.resources(3)->energy==5-(applied&&plan.drain_each?std::min(5u,plan.amount):0));
            check(w.has_condition(target,combat::conditions::Kind::weakness,now)==(applied&&plan.weakness_ms));}
        check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==unsigned(applied));
        const auto body=*skills::particles::targeted_body_2022(victim,1,skill);
        check(std::count(packets.begin(),packets.end(),body)==unsigned(applied));
        if(applied){const auto published=box->drain();const auto finish=std::find(published.begin(),published.end(),casting::terminal(58,1));
            const auto effect=std::find(published.begin(),published.end(),body);check(finish<effect&&effect!=published.end());
            if(renewal){const auto heal=std::find_if(published.begin(),published.end(),[](const auto& packet){return packet.size()==18&&packet[0]==162&&packet[2]==55;});check(effect<heal&&heal!=published.end());}}

        if(exported.is_open()&&applied&&!npc&&!renewal&&energy==20){
            exported<<"{\"skill\":"<<skill<<",\"packets\":[";bool first=true;
            for(const auto& p:packets){if(!first)exported<<',';first=false;exported<<'"';for(auto byte:p){constexpr char hex[]="0123456789abcdef";exported<<hex[byte>>4]<<hex[byte&15];}exported<<'"';}
            exported<<"]}\n";}
    }
    {
        skills::Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[59]=1;
        skills::Slots slots{};slots[0]={59,0};attributes::Ranks ranks{};casting::Hooks h;unsigned calls=0;
        const effects::Scope target{1,2,1};h.events=[](const auto&){};
        h.resources=[](auto)->std::optional<vitals::Resources>{return vitals::Resources{900,1000,30,40};};
        h.target_scope=[&](auto)->std::optional<effects::Scope>{return target;};h.projectile_target_valid=[](auto,auto){return true;};
        h.consume_energy=[](auto cost){check(cost==0);return true;};
        h.energy_drain_completed=[](auto,auto,auto,auto)->casting::EffectCompletion{throw std::runtime_error("signet entered spell completion");};
        casting::Runtime unsupported(1);check(unsupported.start(100,{59,0,2},slots,policy,ranks,true,h)==casting::Reason::unsupported);
        h.energy_drain=[&](auto,const auto&,auto){++calls;return casting::EffectCompletion{true};};
        casting::Runtime signet(1);check(signet.start(100,{59,0,2},slots,policy,ranks,true,h)==casting::Reason::accepted);
        signet.update(100+static_cast<unsigned>(skills::find_skill(59)->activation*1000),h);check(calls==1);
    }
    unsigned signet_cases=0;
    for(bool npc:{false,true})for(unsigned energy:{0u,3u,20u})for(unsigned renewal:{0u,181u,2860u}){
        ++signet_cases;world::Instance w;
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={900,1000};d.damageable=d.conditions_allowed=true;
            d.spawn_energy=combat::npcs::Energy{energy,40};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});}
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{100,1000,50,100},100);w.public_professions(1,6,5);
        if(!npc){w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{900,1000,energy,40},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
        if(renewal){const auto r=*effects::ether_renewal(renewal,15);check(w.status_mutation(source,renewal,80,15,100,100+r.duration,true));}
        std::uint64_t now=100;unsigned calls=0;casting::Hooks h;h.events=[](const auto&){};
        h.resources=[&](auto actor){return w.resources(actor);};h.consume_energy=[&](auto cost){check(cost==0);return w.consume_energy(1,cost);};
        h.target_scope=[&](auto actor){return w.effect_scope(actor);};h.projectile_target_valid=[&](auto actor,auto range){return w.projectile_target_valid(source,actor,range,now);};
        h.before_spell_effect=[](auto,auto,auto)->bool{throw std::runtime_error("signet spell reaction");};
        h.energy_drain_completed=[](auto,auto,auto,auto)->casting::EffectCompletion{throw std::runtime_error("signet spell reward");};
        h.energy_drain=[&](auto scoped,const auto& plan,auto at){++calls;std::vector<world::Packet> visuals;
            return casting::EffectCompletion{w.energy_drain(source,scoped,plan,20,at,&visuals),std::move(visuals)};};
        skills::Policy policy;policy.primary=6;policy.secondary=5;policy.campaigns=255;policy.learned[59]=1;
        skills::Slots slots{};slots[0]={59,0};attributes::Ranks ranks{};ranks[2]=12;casting::Runtime runtime(1);
        check(runtime.start(100,{59,0,target.actor},slots,policy,ranks,true,h)==casting::Reason::accepted);
        now=2100;check(w.consume_energy(1,0,source,now));const auto baseline=w.resources(1)->energy;
        if(!npc){check(w.settle_resources(target,now));w.set_vitals(2,{900,1000,energy,40},100);}
        runtime.update(now,h);check(calls==1&&w.resources(1)->health==100&&w.resources(1)->energy==baseline);
        check((npc?w.npc_energy_snapshot(target)->current:w.resources(2)->energy)==energy-std::min(energy,effects::energy_drain(59,12)->amount));
        if(npc){combat::conditions::State weakness;check(weakness.apply(combat::conditions::Kind::weakness,1,now,10000));check(w.npc_combat_snapshot(target)->effects==weakness.presentation(0,now));}
        else check(w.has_condition(target,combat::conditions::Kind::weakness,now));
    }
    check(signet_cases==18);
    check(cases==540);std::cout<<"540 paid Energy-drain spell cases and18 signet cases PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
