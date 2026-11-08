#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
unsigned scenario=0;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("pet Runtime scenario "+std::to_string(scenario)+" line "+std::to_string(at.line()));
}
int main()try{
    check(!casting::Runtime::effect_supported(411)&&!casting::Runtime::effect_supported(436));
    for(scenario=0;scenario<15;++scenario){
        const effects::Scope owner{1,1,5},pet{1,100,8};std::uint64_t generation=5,target_generation=8;
        pets::CharmLedger ledger;pets::CharmFacts facts{owner,pet,1,5,true,true,true,true,false,true,true,true,false,1000};
        vitals::Resources resources{100,100,30,30};unsigned paid=0,refunds=0,completions=0,reserved_cancel=0,active_cancel=0;
        std::vector<combat::Packet> packets;casting::Hooks h;
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==1?std::optional(resources):std::nullopt;};
        h.actor_generation=[&]{return std::optional(generation);};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{if(id==1)return effects::Scope{1,1,generation};if(id==100)return effects::Scope{1,100,target_generation};return {};};
        h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        h.restore_energy=[&](unsigned){++refunds;return true;};
        h.pet_charm_reserve=[&](auto a,auto b,auto now){check(a==owner&&b==pet);return ledger.reserve(facts,{},0,now);};
        h.pet_charm_cancel_reservation=[&](const auto& receipt){++reserved_cancel;ledger.cancel(receipt);};
        h.pet_charm_cancel=[&](const auto& receipt){++active_cancel;ledger.cancel(receipt);};
        h.pet_charm_accept=[&](const auto& reservation,auto sequence,auto starts,auto duration){check(duration==10000);return ledger.accept(reservation,facts,{},0,sequence,starts,duration);};
        const combat::Packet added{177,0,100,0,0,0};
        h.pet_charm_completed=[&](const auto& receipt,auto now)->casting::EffectCompletion{
            ++completions;const auto status=ledger.finish(receipt,facts,true,true,[&](const auto&){return scenario==12?pets::CommitStatus::conflict:scenario==13?pets::CommitStatus::unavailable:pets::CommitStatus::committed;},now);
            return {status==pets::CharmCompletion::adopted,{added}};
        };
        h.consume_energy_at=[&](unsigned cost,auto now){
            check(cost==10&&now==1000);++paid;
            // Reentrant World-equivalent calls see the exact reservation before
            // payment; no second owner or cast can reserve the same animal.
            auto other=facts;++other.owner.actor;check(!ledger.reserve(other,{},0,now));
            if(scenario==9)throw std::runtime_error("payment exception");
            if(scenario==1)return false;
            resources.energy-=cost;
            if(scenario==2){resources.health=0;facts.owner_alive=false;}
            if(scenario==3)facts.in_range=false;
            return true;
        };
        if(scenario==14)h.pet_charm_reserve={};
        auto runtime=std::make_unique<casting::Runtime>(1);casting::ActorContext actor{1,5,2};
        casting::Reason result=casting::Reason::policy;
        try{result=runtime->start_trusted(1000,{411,0,100},actor,true,h);check(scenario!=9);}
        catch(const std::runtime_error& error){if(scenario!=9||std::string(error.what())!="payment exception")throw;}
        const bool admitted=scenario!=1&&scenario!=2&&scenario!=3&&scenario!=9&&scenario!=14;
        check((result==casting::Reason::accepted)==admitted&&paid==(scenario==14?0u:1u)&&refunds==0);
        if(scenario==2)check(resources.health==0&&resources.energy==20);
        if(scenario==3)check(resources.energy==20);
        if(admitted){
            check(ledger.current(owner).has_value());runtime->update(10999,h);check(completions==0);
            if(scenario==4)runtime->cancel(h,11000);
            if(scenario==5)runtime->interrupt(11000,h);
            if(scenario==6)++generation;
            if(scenario==7)h.pet_charm_completed={};
            if(scenario==8)runtime.reset();
            if(scenario==10){++generation;runtime->cancel(h,11000);}
            if(scenario==11)++target_generation;
            if(runtime)runtime->update(11000,h);
            if(runtime)runtime->update(12000,h);
        }
        check(!ledger.current(owner));
        const bool success=scenario==0;
        const auto finish=casting::terminal(58,1);const auto body=*particles::targeted_body_2022(100,1,411);
        check(std::count(packets.begin(),packets.end(),body)==unsigned(success));
        if(success){const auto f=std::find(packets.begin(),packets.end(),finish),b=std::find(packets.begin(),packets.end(),body),a=std::find(packets.begin(),packets.end(),added);check(f<b&&b<a&&a!=packets.end()&&completions==1&&active_cancel==0);}
        auto clean=facts;clean.owner_alive=clean.in_range=true;
        const auto retry=ledger.reserve(clean,{},0,13000);check(bool(retry));check(ledger.cancel(*retry));
        check(refunds==0);if(admitted)check(resources.energy==20);
    }
    for(unsigned dead:{0u,1u}){
        scenario=20+dead;const effects::Scope owner{1,1,5},pet{1,100,8};unsigned paid=0,heals=0,published=0;
        std::vector<combat::Packet> packets;casting::Hooks h;
        h.resources=[&](unsigned id)->std::optional<vitals::Resources>{
            if(id==1)return vitals::Resources{100,100,30,30};
            if(id==100)return vitals::Resources{dead?0u:100u,100,0,0};return {};
        };
        h.actor_generation=[]{return std::optional<std::uint64_t>(5);};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{if(id==1)return owner;if(id==100)return pet;return {};};
        h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        h.consume_energy_at=[&](unsigned cost,auto){check(cost==5);++paid;return true;};
        h.pet_owned_scope=[&](auto a){check(a==owner);return std::optional(pet);};
        h.pet_comfort_ready=[&](auto a,auto b,auto){check(a==owner&&b==pet);return true;};
        const combat::Packet health{36,0,100,0,0,0};
        h.pet_comfort_completed=[&](auto a,auto b,unsigned rank,auto sequence,auto now)->casting::EffectCompletion{check(a==owner&&b==pet&&rank==12&&sequence&&now==2000);++heals;return {true,{health},{},[&]{
            ++published;check(std::find(packets.begin(),packets.end(),casting::generic(21,1,747))!=packets.end());
            check(std::find(packets.begin(),packets.end(),health)!=packets.end());
        }};};
        casting::Runtime runtime(1);casting::ActorContext actor{1,5,2};actor.effective[22]=12;
        check(runtime.start_trusted(1000,{436,0,999},actor,true,h)==casting::Reason::accepted&&paid==1);
        runtime.update(1999,h);check(!heals);runtime.update(2000,h);runtime.update(2000,h);check(heals==1&&published==1);
        const auto f=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
        const auto a=std::find(packets.begin(),packets.end(),casting::generic(21,1,747));
        const auto b=std::find(packets.begin(),packets.end(),casting::generic(21,1,748));
        const auto hp=std::find(packets.begin(),packets.end(),health);check(f<a&&a<b&&b<hp&&hp!=packets.end());
        check(std::count(packets.begin(),packets.end(),casting::generic(21,1,747))==1);
        check(runtime.start_trusted(2750,{436,0,0},actor,true,h)==casting::Reason::recharge);
    }
    for(unsigned primary:{2u,3u})for(unsigned skill:{411u,436u}){
        scenario=30+primary+skill;casting::Hooks h;const effects::Scope owner{1,1,5},pet{1,100,8};unsigned paid=0;
        h.events=[](const auto&){};h.resources=[](unsigned)->std::optional<vitals::Resources>{return vitals::Resources{100,100,100,100};};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==1?std::optional(owner):id==100?std::optional(pet):std::nullopt;};
        h.consume_energy_at=[&](unsigned cost,auto){paid=cost;return true;};
        h.pet_charm_reserve=[&](auto a,auto b,auto){return std::optional(pets::CharmReservation{1,a,b});};
        h.pet_charm_cancel_reservation=[](const auto&){};h.pet_charm_cancel=[](const auto&){};
        h.pet_charm_accept=[&](const auto&,auto sequence,auto starts,auto duration){return std::optional(pets::CharmAttempt{1,sequence,starts,starts+duration,owner,pet,{0,{},pets::Profile{pets::Pet{}}}});};
        h.pet_charm_completed=[](const auto&,auto)->casting::EffectCompletion{return false;};
        h.pet_owned_scope=[&](auto){return std::optional(pet);};h.pet_comfort_ready=[](auto,auto,auto){return true;};
        h.pet_comfort_completed=[](auto,auto,unsigned,auto,auto)->casting::EffectCompletion{return false;};
        Slots bar{};bar[0]={skill,0};Policy policy;policy.primary=primary;policy.secondary=primary==2?3:2;policy.campaigns=255;policy.learned[skill]=1;
        attributes::Ranks ranks{};ranks[23]=12;casting::Runtime runtime(1);
        check(runtime.start(1000,{skill,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(paid==(primary==2?(skill==411?5u:3u):(skill==411?10u:5u)));runtime.cancel(h,1001);
    }
    std::cout<<"Staged paid pet Runtime costs, leases, stale scopes, reaction failures, native completion ordering and recharge passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
