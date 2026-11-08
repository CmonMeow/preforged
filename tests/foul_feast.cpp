#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <fstream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
void check(bool value,std::source_location where=std::source_location::current()){
    if(!value)throw std::runtime_error("Foul Feast regression line "+std::to_string(where.line()));
}
unsigned u32(const Packet& p,unsigned at){unsigned value=0;for(unsigned i=0;i<4;++i)value|=unsigned(p.at(at+i))<<(8*i);return value;}
int main(int argc,char** argv)try{
    check(argc<=2);std::ofstream exported;if(argc==2){exported.open(argv[1]);check(exported.good());}
    using combat::conditions::Kind;
    for(unsigned rank=0;rank<=20;++rank){
        const auto p=*effects::ally_cleansing(2057,rank,20);
        check(effects::valid_ally_cleansing(p)&&p.draw&&!p.self&&p.disease_half_recharge&&p.range==1248);
        check(p.health_gain==3*rank&&p.energy_gain==effects::interpolate(0,2,rank)&&!p.divine_heal&&!p.divine_rank);
    }
    check(!effects::ally_cleansing(2057,21));
    {
        world::Instance w;w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>());w.join(2,"Ally",100,0,std::make_shared<world::Outbox>());
        check(w.party_action(1,158,2,0)&&w.party_action(2,154,1,0));
        w.set_vitals(1,{100,1000,10,40},100);w.set_vitals(2,{100,1000,10,40},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);const auto plan=*effects::ally_cleansing(2057,12);
        check(w.apply_condition(source,target,Kind::disease,100,1000)&&w.apply_condition(source,target,Kind::weakness,100,2000));
        const auto result=w.ally_cleansing_outcome(source,target,plan,100);
        check(result&&result->conditions_transferred==2&&result->removed_disease);
        check(w.resources(1)->health==172&&w.resources(1)->energy==14&&w.resources(2)->health==100);
        check(w.has_condition(source,Kind::disease,1099)&&!w.has_condition(source,Kind::disease,1100));
        check(w.has_condition(source,Kind::weakness,2099)&&!w.has_condition(source,Kind::weakness,2100));
        check(!w.has_condition(target,Kind::disease,100)&&!w.has_condition(target,Kind::weakness,100));
        const auto empty=w.ally_cleansing_outcome(source,target,plan,100);
        check(empty&&!empty->conditions_transferred&&!empty->removed_disease&&w.resources(1)->health==172&&w.resources(1)->energy==14);
        check(!w.ally_cleansing_outcome(source,source,plan,100));
        auto stale=target;++stale.generation;check(!w.ally_cleansing_outcome(source,stale,plan,100));
        auto forged=plan;++forged.energy_gain;check(!w.ally_cleansing_outcome(source,target,forged,100));
        // A longer existing caster timer is retained, but the ally is cleansed.
        w.set_vitals(1,{900,1000,39,40},100);
        check(w.apply_condition(source,source,Kind::weakness,100,5000)&&w.apply_condition(source,target,Kind::weakness,100,500));
        const auto covered=w.ally_cleansing_outcome(source,target,plan,100);
        check(covered&&covered->conditions_transferred==1&&!covered->removed_disease&&w.resources(1)->energy==40&&w.resources(1)->health==936);
        check(w.has_condition(source,Kind::weakness,5099)&&!w.has_condition(target,Kind::weakness,100));
        // Disease on the caster is irrelevant. Expiry on the ally at the exact
        // completion boundary must not generate the half-recharge receipt.
        check(w.apply_condition(source,target,Kind::disease,100,100));
        const auto expired=w.ally_cleansing_outcome(source,target,plan,200);
        check(expired&&!expired->conditions_transferred&&!expired->removed_disease&&w.has_condition(source,Kind::disease,200));
    }
    {
        world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>());w.join(2,"Ally",100,0,std::make_shared<world::Outbox>());
        check(w.party_action(1,158,2,0)&&w.party_action(2,154,1,0));
        w.set_vitals(1,{400,500,0,40},100);w.set_vitals(2,{100,500,0,40},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.apply_condition(source,source,Kind::deep_wound,100,1000));
        check(w.apply_condition(source,target,Kind::weakness,100,1000));
        const auto result=w.ally_cleansing_outcome(source,target,*effects::ally_cleansing(2057,12),100);
        check(result&&result->conditions_transferred==1&&w.resources(1)->health==329); // 300+36*.8, displayed ceil.
    }
    for(bool disease:{false,true})for(bool recovery:{false,true}){
        using namespace casting;Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned[2057]=1;
        Slots bar{};bar[0]={2057,0};attributes::Ranks ranks{};ranks[6]=12;
        Hooks h;unsigned charged=0,applied=0;effects::Scope target{1,43,1};std::vector<Packet> packets;
        h.events=[&](const auto& p){packets.insert(packets.end(),p.begin(),p.end());};
        h.resources=[](auto actor)->std::optional<vitals::Resources>{if(actor!=42&&actor!=43)return {};return vitals::Resources{100,1000,50,50};};
        h.target_valid=[](auto actor,float range,bool self){return actor==43&&range==1248&&!self;};
        h.target_scope=[&](auto actor)->std::optional<effects::Scope>{return actor==43?std::optional(target):std::nullopt;};
        h.consume_energy=[&](auto n){check(n==5);charged+=n;return true;};
        h.ally_cleansing=[](auto,const auto&,auto){return true;};Runtime legacy(42);
        check(legacy.start(100,{2057,0,43},bar,policy,ranks,true,h)==casting::Reason::unsupported&&!charged);
        h.ally_cleansing_outcome=[&](auto scope,const auto& p,auto at)->std::optional<effects::CleansingOutcome>{check(scope==target&&p.skill==2057&&p.rank==12&&at==350);++applied;return effects::CleansingOutcome{2,disease};};
        if(recovery)h.effect_snapshot=[](auto)->std::optional<std::vector<effects::Entry>>{return std::vector<effects::Entry>{{1,42,13,0,effects::Category::stance,0,100000}};};
        Runtime runtime(42);check(runtime.start(100,{2057,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&charged==5);
        runtime.update(349,h);check(!applied);runtime.update(350,h);check(applied==1);
        const unsigned seconds=disease?2:recovery?3:4;
        const auto recharge=slot_event(228,42,{2057,0},seconds);
        check(std::count(packets.begin(),packets.end(),recharge)==1);
        const unsigned ready=350+seconds*1000;
        check(runtime.start(ready-1,{2057,0,43},bar,policy,ranks,true,h)==casting::Reason::recharge&&charged==5);
        runtime.update(ready,h);check(std::count(packets.begin(),packets.end(),slot_event(229,42,{2057,0}))==1);
        check(runtime.start(ready,{2057,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted&&charged==10);
        runtime.cancel(h);runtime.update(ready+1000,h);check(applied==1);
    }
    unsigned completed_cases=0;
    for(unsigned skill:{311u,2057u})for(unsigned renewal:{0u,181u,2860u})for(unsigned mode=0;mode<8;++mode){
        ++completed_cases;world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        const unsigned primary=skill==2057?4u:6u,secondary=skill==2057?6u:3u;
        w.public_professions(1,primary,secondary);w.set_vitals(1,{100,1000,50,100},100);w.set_vitals(2,{100,1000,30,30},100);
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        if(renewal){const auto r=*effects::ether_renewal(renewal,15);check(w.status_mutation(source,renewal,80,15,100,100+r.duration,true));}
        if(mode!=1){check(w.apply_condition(source,target,Kind::blind,100,10000));check(w.apply_condition(source,target,mode==2?Kind::disease:Kind::weakness,100,10000));}
        std::vector<Packet> packets;std::uint64_t event=0;bool stale=false;unsigned calls=0;
        casting::Hooks h;h.events=[&](const auto& ps){packets.insert(packets.end(),ps.begin(),ps.end());};
        h.resources=[&](auto actor){return w.resources(actor);};h.consume_energy=[&](auto n){return w.consume_energy(1,n);};
        h.target_valid=[&](auto actor,auto range,auto self){return w.valid_heal_target(1,actor,range,self);};
        h.target_scope=[&](auto actor){auto value=w.effect_scope(actor);if(value&&stale)++value->generation;return value;};
        h.before_spell_effect=[&](auto id,auto token,auto now){event=token;return mode!=7&&w.before_spell_effect(source,id,token,now);};
        h.ally_cleansing=[](auto,auto,auto)->bool{throw std::runtime_error("cleansing fallback");};
        h.ally_cleansing_completed=[&](auto scope,const auto& plan,auto token,auto now){
            ++calls;check(token==event);std::function<void()> publish;
            const auto result=w.ally_cleansing_outcome(source,scope,plan,now,token,&publish);
            if(!result)return casting::CleansingCompletion{};
            check(!w.ally_cleansing_outcome(source,scope,plan,now,token));
            std::vector<Packet> visuals;const auto body=skill==311?skills::particles::targeted_body_2022(scope.actor,1,skill):skills::particles::target_body_2022(scope.actor,skill);if(body)visuals.push_back(*body);
            return casting::CleansingCompletion{{true,std::move(visuals),{},std::move(publish),true},*result};
        };
        Policy policy;policy.primary=primary;policy.secondary=secondary;policy.campaigns=255;policy.learned[skill]=1;
        Slots slots{};slots[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};ranks[find_skill(skill)->attribute]=12;
        {
            auto missing=h;missing.target_scope=[](auto)->std::optional<effects::Scope>{return {};};
            casting::Runtime rejected(1);check(rejected.start(100,{skill,0,2},slots,policy,ranks,true,missing)==casting::Reason::target&&w.resources(1)->energy==50);
        }
        casting::Runtime runtime(1);check(runtime.start(100,{skill,0,2},slots,policy,ranks,true,h)==casting::Reason::accepted);
        const auto paid=w.resources(1)->energy;
        if(mode==3)h.ally_cleansing_completed={};if(mode==4)h.before_spell_effect={};if(mode==5)runtime.cancel(h);if(mode==6)stale=true;
        const auto finish=100+static_cast<unsigned>(find_skill(skill)->activation*1000);runtime.update(finish,h);
        const bool applied=mode<3;const auto plan=*effects::ally_cleansing(skill,12);
        check(calls==unsigned(applied));check(w.resources(1)->health==100+(applied?unsigned(renewal?20:0)+(mode==1?0:2*plan.health_gain):0));
        check(w.resources(1)->energy==paid+(applied?unsigned(renewal?effects::ether_renewal(renewal,15)->energy_per_enchantment:0)+(mode==1?0:2*plan.energy_gain):0));
        if(mode!=1){check(w.has_condition(target,Kind::blind,finish)==!applied);check(w.has_condition(source,Kind::blind,finish)==applied);}
        const auto body=skill==311?skills::particles::targeted_body_2022(2,1,skill):skills::particles::target_body_2022(2,skill);if(body)check(std::count(packets.begin(),packets.end(),*body)==unsigned(applied));
        if(skill==311)check(std::count(packets.begin(),packets.end(),*skills::particles::target_body_2022(2,skill))==0);
        if(exported.is_open()&&!renewal&&mode==0){
            exported<<"{\"skill\":"<<skill<<",\"packets\":[";bool first=true;
            for(const auto& packet:packets){if(!first)exported<<',';first=false;exported<<'"';
                for(auto byte:packet){constexpr char hex[]="0123456789abcdef";exported<<hex[byte>>4]<<hex[byte&15];}exported<<'"';}
            exported<<"]}\n";
        }
        if(applied&&skill==2057)check(std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{skill,0},mode==2?2:4))==1);
    }
    check(completed_cases==48);
    std::cout<<"Foul Feast transfer receipts, health/energy gain, Disease half recharge,48 scoped paid completions,cast/cancel and expiry PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
