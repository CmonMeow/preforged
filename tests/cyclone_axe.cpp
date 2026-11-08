#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace combat::npcs;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto plan=*combat::weapon::attack_skill(330,12);check(plan.bonus==10&&plan.required_type==2,"Cyclone plan");
    const std::string hash(64,'a');
    std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 -166 0 0 - -\n");
    auto population=load_npc_catalog(appearance);
    std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+
        " tests/cyclone_axe.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 1 test test test - test 60 60 test test 60 test\n");
    const auto catalog=load_catalog(metadata,hash);bind_combat_catalog(population,catalog,hash);
    for(unsigned scenario=0;scenario<3;++scenario){const bool blind=scenario==1;
        Instance w;w.configure_attack_miss_draws([]{return .5;});w.configure_npcs(population.for_map(146));
        w.join(1,"Warrior",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        w.join(2,"Adjacent",166,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.join(3,"Outside",167,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.join(4,"Ally",0,10,std::make_shared<Outbox>(),0,{}, {},100,1);
        for(unsigned id:{1u,2u,3u,4u})w.set_vitals(id,{100,100,20,20},100);
        const auto source=*w.effect_scope(1),victim=*w.effect_scope(2),npc=*w.effect_scope(100);
        combat::weapon::Owned axe{1,1,{20,20,1},12,false,false,2};check(w.ordinary_weapon(source,axe,20),"owned axe");
        check(w.melee_target(source,1,100,330),"Cyclone self-centered target");
        check(!w.melee_attack(source,source,axe,plan,100)&&w.npc_combat_snapshot(npc)->health->current==100,"unknown adjacent armor partially damaged NPC");
        check(w.take_attack_outcomes(source).empty(),"rejected attack leaked outcome");
        check(w.physical_armor(victim,60),"fixture armor");
        // Recipient2 resolves first;recipient5 then fails its armor evidence
        // gate. Neither Health nor an event ID may commit from that plan.
        w.join(5,"Unreviewed",0,20,std::make_shared<Outbox>(),0,{}, {},100,2);w.set_vitals(5,{100,100,20,20},100);
        check(!w.melee_attack(source,source,axe,plan,100)&&w.resources(2)->health==100&&w.npc_combat_snapshot(npc)->health->current==100&&w.take_attack_outcomes(source).empty(),"late invalid recipient partially committed");
        w.leave(5);
        if(blind)check(w.apply_condition(victim,source,combat::conditions::Kind::blind,100,1000),"Blind fixture");
        if(scenario==2)w.move(2,600,0,100);
        check(w.melee_attack(source,source,axe,plan,scenario==2?1100:100),"Cyclone attack rejected");
        check(w.resources(2)->health==(scenario?100u:70u)&&w.npc_combat_snapshot(npc)->health->current==(blind?100:70),"mixed adjacent targets or Blind result");
        check(w.resources(3)->health==100&&w.resources(4)->health==100,"out-of-range/friendly hit");
        const auto outcomes=w.take_attack_outcomes(source);check(outcomes.size()==(scenario==2?1u:2u),"missing recipient outcomes");
        for(std::size_t i=0;i<outcomes.size();++i){const auto& result=outcomes[i];
            check(result.attacker==source&&result.event==i+1&&result.released==(scenario==2?1100u:100u)&&result.at==result.released,"outcome identity/time or rejected ID leaked");
            check(result.target==(scenario==2||i==1?npc:victim)&&result.outcome==(blind?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit),"wrong recipient/outcome");
        }
        check(w.take_attack_outcomes(source).empty(),"duplicate recipient outcomes");
        auto forged=plan;++forged.bonus;check(!w.melee_attack(source,source,axe,forged,100),"forged AoE damage");
        axe.item_type=5;check(!w.melee_attack(source,source,axe,plan,100),"bow accepted for axe attack");
    }
    {
        using namespace skills;using namespace skills::casting;
        Policy policy;policy.primary=1;policy.campaigns=0x1e;policy.learned[330]=1;Slots bar{};bar[0]={330,0};attributes::Ranks ranks{};ranks[18]=12;
        Hooks hooks;vitals::Resources self{100,100,20,20};unsigned hits=0,cost=0;
        effects::Scope scope{1,42,1};hooks.events=[](const auto&){};hooks.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(self):std::nullopt;};
        hooks.consume_energy=[&](unsigned amount){cost+=amount;return true;};hooks.target_valid=[](auto,float,bool){return false;};hooks.melee_target=[](auto,auto){return false;};
        hooks.weapon_snapshot=[](){return std::optional<combat::weapon::Owned>{{1,1,{20,20,1},12,false,false,2}};};hooks.attack_activity=[](auto){};
    hooks.weapon_attack_timing=[](auto){return std::optional<combat::weapon::Timing>{{500,1330,1200}};};
        hooks.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==42?std::optional(scope):std::nullopt;};
        hooks.attack_skill_target=[](unsigned id,unsigned skill,auto){return id==42&&skill==330;};
        hooks.melee_attack=[&](auto target,const auto&,const auto& hit,auto){check(target==scope&&hit.skill==330,"selected foe displaced caster AoE center");++hits;return true;};
        Runtime cast(42);check(cast.start(100,{330,0,999},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&cost==5,"Cyclone activation or energy");
        cast.update(600,hooks);check(hits==1,"Cyclone did not complete");
    }
    std::cout<<"Cyclone caster-centered adjacent mixed-target damage, atomic evidence gates and casting verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

