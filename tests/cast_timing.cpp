#include "gw_cast_timing.h"
#include "gw_skill_casting.h"
#include "gw_activation_modifiers.h"
#include <stdexcept>
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace fw::gw::skills;
void check(bool v,std::source_location at=std::source_location::current()){if(!v)throw std::runtime_error("cast timing regression line "+std::to_string(at.line()));}
int main()try{
    check(casting::Runtime::effect_supported(1727));
    check(timing::instant_action(*find_skill(1727)));
    for(unsigned rank=0;rank<=20;++rank)check(timing::stance(1727,rank)->duration==effects::natural_stride(rank)->duration);
    check(!timing::stance(1727,21));
    check(timing::quickening_zephyr_energy_cost(0,true)==0);
    check(timing::quickening_zephyr_energy_cost(5,true)==6);
    check(timing::quickening_zephyr_energy_cost(10,true)==13);
    check(timing::quickening_zephyr_energy_cost(15,true)==19);
    check(timing::quickening_zephyr_energy_cost(25,true)==32);
    check(timing::quickening_zephyr_energy_cost(25,false)==25);
    check(!timing::quickening_zephyr_energy_cost(10001,true));
    for(unsigned kind:{46u,49u,58u,59u}){
        const skills::Packet literal{158,0,static_cast<std::uint8_t>(kind),0,0,0,42,0,0,0,0,0,0,0};
        check(casting::terminal(kind,42)==literal);
    }
    check(casting::generic(48,42,123)==skills::Packet({158,0,48,0,0,0,42,0,0,0,123,0,0,0}));
    {bool rejected=false;try{casting::terminal(48,42);}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
    {
        combat::conditions::State conditions;check(conditions.apply(combat::conditions::Kind::dazed,1,100,60000));
        std::vector<effects::Entry> hexes{{1,1,53,12,effects::Category::hex,100,17100},{2,1,3183,12,effects::Category::hex,100,17100}};
        check(effects::activation_multiplier(*find_skill(53),hexes,conditions,100)==2.5);
        check(effects::activation_multiplier(*find_skill(132),hexes,conditions,100)==2.);
        check(effects::activation_multiplier(*find_skill(397),hexes,conditions,100)==1.);
        check(timing::activation(*find_skill(53),5,15,2.5)==2500); // Fast Casting lies outside the cap.
        check(timing::activation(*find_skill(53),4,0,2.5)==5000);
        check(!timing::activation(*find_skill(53),5,0,2.51)&&!timing::activation(*find_skill(53),5,0,.74));
        check(effects::activation_multiplier(*find_skill(53),hexes,conditions,17100)==2.);
        check(effects::activation_multiplier(*find_skill(132),hexes,conditions,17100)==1.);
        check(effects::degeneration_hex(53,12)->duration==17000&&effects::degeneration_hex(53,12,true)->duration==10000);
    }
    auto spell=*find_skill(28);spell.profession=5;spell.type=5;spell.activation=2;
    auto signet=spell;signet.type=7;
    for(unsigned rank=0;rank<=20;++rank){
        check(timing::activation(spell,5,rank)==std::uint64_t(std::llround(2000*std::exp2(-double(rank)/15))));
        check(timing::activation(signet,5,rank)==std::uint64_t(std::llround(2000*(1.-.03*rank))));
        check(timing::activation(spell,4,rank)==2000);
        check(timing::recharge(spell,5,rank,false,{},100,10)==unsigned(std::llround(10*(1.-.03*rank))));
        check(timing::recharge(spell,5,rank,true,{},100,10)==10);
        check(timing::recharge(signet,5,rank,false,{},100,10)==10);
    }
    spell.profession=4;spell.activation=1.99f;
    check(timing::activation(spell,5,15)==1990);
    spell.activation=2;check(timing::activation(spell,5,15)==1000);
    check(!timing::activation(spell,5,21));
    effects::Entry recovery{1,42,13,15,effects::Category::stance,100,20100};
    effects::Entry inscriptions{2,42,15,20,effects::Category::stance,100,32100};
    spell.profession=5;
    check(timing::recharge(spell,5,15,false,{recovery},100,10)==4);
    check(timing::recharge(spell,5,15,true,{recovery},100,10)==7);
    check(timing::recharge(spell,5,0,false,{recovery},20100,10)==10);
    check(timing::recharge(spell,5,0,false,{recovery},99,10)==10);
    check(timing::recharge(signet,5,15,false,{inscriptions},100,10)==5);
    check(timing::recharge(signet,5,15,false,{recovery},100,10)==10);
    check(timing::recharge(spell,5,0,false,{inscriptions},100,10)==10);
    // An authoritative World aura decision is supplied only at completion.
    check(timing::recharge(*find_skill(1814),0,0,false,{},100,15,1.,true)==8);
    check(timing::recharge(*find_skill(1814),0,0,false,{},100,15)==15);
    check(timing::stance(13,0)->duration==5000&&timing::stance(15,15)->duration==25000);
    {
        const auto plan=effects::deadly_paradox(0);
        check(plan&&plan->duration==5000&&plan->attack_disable==10000);
        check(effects::deadly_paradox(15)->duration==15000&&!effects::deadly_paradox(21));
        const effects::Entry paradox{82,25,572,0,effects::Category::stance,100,5100};
        combat::conditions::State clear;
        check(effects::activation_multiplier(*find_skill(769),{&paradox,1},clear,100)==.75);
        check(effects::activation_multiplier(*find_skill(769),{&paradox,1},clear,5100)==1.);
        check(timing::recharge(*find_skill(769),7,0,false,{paradox},100,10)==7);
        check(timing::recharge(*find_skill(572),7,0,false,{paradox},100,10)==10);
        check(effects::deadly_paradox_recharges(*find_skill(775),paradox,100));
        check(!effects::deadly_paradox_activates(*find_skill(775),paradox,100));
        check(!effects::deadly_paradox_recharges(*find_skill(769),paradox,5100));
        auto forged=paradox;forged.expires=5101;
        check(!effects::deadly_paradox_recharges(*find_skill(769),forged,100));
        const effects::Scope owner{1,25,1};
        effects::CooldownBarSnapshot bar{};
        bar.published={owner,1,100};bar.revision=1;
        bar.bar={Slot{572,0},Slot{775,0},Slot{776,0},Slot{777,0},Slot{769,0}};
        effects::DeadlyParadoxDisables empty(owner);
        const auto next=empty.propose(owner,*plan,bar,82,100);
        check(next&&next->disabled_until(owner,{775,0},100)==10100&&
              next->disabled_until(owner,{776,0},100)==10100&&
              next->disabled_until(owner,{777,0},100)==10100&&
              next->disabled_until(owner,{769,0},100)==0&&
              next->disabled_until(owner,{572,0},100)==0);
        check(!next->propose(owner,*plan,bar,82,101));
        check(next->disabled_until(owner,{775,0},10100)==0);
        const auto reset=next->propose_reset_slots(owner,{{775,0}},101);
        check(reset&&reset->disabled_until(owner,{775,0},101)==0&&
              reset->disabled_until(owner,{776,0},101)==10100);
        auto duplicate=bar;duplicate.bar[4]=duplicate.bar[1];
        check(!next->propose(owner,*plan,duplicate,83,101));
        auto missing=bar;missing.bar[0]={};
        check(!next->propose(owner,*plan,missing,83,101));
    }
    {
        using namespace casting;
        Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.learned[13]=policy.learned[15]=policy.learned[28]=1;
        Slots bar{};bar[0]={28,0};bar[1]={13,0};bar[2]={15,0};attributes::Ranks ranks{};
        effects::Scope scope{1,42,1};effects::Registry registry(scope);
        Hooks hooks;hooks.target_valid=[](unsigned id,float,bool self){return id==42&&self;};std::vector<Packet> packets;unsigned energy=100,applied=0;
        hooks.resources=[](unsigned)->std::optional<vitals::Resources>{return vitals::Resources{100,100,100,100};};
        hooks.consume_energy=[&](unsigned cost){energy-=cost;return true;};
        hooks.quickening_zephyr_aura=[](auto now)->std::optional<bool>{return now==101;};
        hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        hooks.status_mutation=[&](unsigned skill,unsigned id,unsigned rank,auto now,auto until,bool active){
            check((skill==13||skill==15)&&rank==0);if(active){check(until==now+5000);++applied;return registry.apply(scope,{id,42,skill,rank,effects::Category::stance,now,until},effects::Replace::category).accepted;}registry.remove(scope,id);return true;
        };
        Runtime runtime(42);
        check(runtime.start(100,{13,0,0},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
        check(energy==95&&applied==1);
        check(std::find(packets.begin(),packets.end(),generic(48,42,13))!=packets.end());
        check(std::find(packets.begin(),packets.end(),generic(58,42,0))==packets.end());
        check(runtime.start(101,{15,0,0},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
        check(energy==82&&applied==2); // 10 base + 3 Zephyr surcharge.
        check(registry.size()==1&&registry.active(scope,101).front().skill==15);
        check(runtime.start(102,{13,0,0},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
    }
    {
        using namespace casting;
        Policy policy;policy.primary=5;policy.campaigns=0x1e;
        policy.learned[13]=policy.learned[15]=policy.learned[26]=1;
        Slots bar{};bar[0]={26,0};bar[1]={13,0};bar[2]={15,0};attributes::Ranks ranks{};ranks[0]=15;
        effects::Scope owner{1,42,1},foe{1,43,1};effects::Registry registry(owner);
        Hooks h;unsigned hexes=0;std::vector<std::vector<Packet>> batches;
        h.events=[&](const auto& batch){batches.push_back(batch);};
        h.resources=[](unsigned)->std::optional<vitals::Resources>{return vitals::Resources{100,100,100,100};};
        h.consume_energy=[](unsigned){return true;};h.target_valid=[](unsigned,float,bool){return true;};
        h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==43?foe:owner;};
        h.projectile_target_valid=[](unsigned id,float){return id==43;};
        h.empathy=[&](auto scope,const auto&,auto){check(scope==foe);++hexes;return true;};
        h.status_mutation=[&](unsigned skill,unsigned id,unsigned rank,auto now,auto until,bool add){
            if(!add){registry.remove(owner,id);return true;}
            return registry.apply(owner,{id,42,skill,rank,effects::Category::stance,now,until},effects::Replace::category).accepted;
        };
        h.effect_snapshot=[&](auto now)->std::optional<std::vector<effects::Entry>>{return registry.active(owner,now);};
        Runtime runtime(42);const auto deadline=*timing::activation(*find_skill(26),5,15);
        bool zephyr=false;unsigned aura_queries=0;
        h.quickening_zephyr_aura=[&](auto now)->std::optional<bool>{
            if(now==deadline)++aura_queries;
            return zephyr&&now==deadline;
        };
        check(deadline>100);
        check(runtime.start(0,{26,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(batches.size()==1&&batches[0].size()==3);
        check(batches[0][1][0]==161&&batches[0][2]==activation(42,43,26));
        const auto bits=std::bit_cast<std::uint32_t>(float(deadline)/1000.f);
        for(unsigned n=0;n<4;++n)check(batches[0][1][10+n]==((bits>>(8*n))&255));
        check(runtime.start(100,{13,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
        runtime.update(deadline-1,h);check(!hexes&&!aura_queries);
        zephyr=true;
        runtime.update(deadline,h);check(hexes==1);
        check(aura_queries==1);
        const auto recharge=*timing::recharge(*find_skill(26),5,15,false,registry.active(owner,deadline),deadline,find_skill(26)->recharge,1.,true);
        check(std::find(batches.back().begin(),batches.back().end(),slot_event(228,42,{26,0},recharge))!=batches.back().end());
        check(runtime.start(deadline+1,{15,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(hexes==1); // An instant stance cannot repeat the completed spell.
    }
    std::cout<<"cast timing passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
