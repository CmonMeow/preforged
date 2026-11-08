#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Blood Drinker line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope source,target,npc;
    Fixture(unsigned health=251){
        const std::string hash(64,'a');
        std::istringstream p("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
        std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/blood_drinker.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 80 1 1 test test test - test 60 60 test test\n");
        auto catalog=world::load_npc_catalog(p);world::bind_combat_catalog(catalog,combat::npcs::load_catalog(m,hash),hash);
        w.configure_npcs(catalog.for_map(146));
        w.join(1,"Necromancer",0,0,box,0,{},0,100,1);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{},0,100,2);
        w.set_vitals(1,{health,500,100,100},100);w.set_vitals(2,{500,500,100,100},100);
        source=*w.effect_scope(1);target=*w.effect_scope(2);npc=*w.effect_scope(100);box->drain();
    }
};
int main()try{
    using namespace skills;using namespace skills::effects;using combat::conditions::Kind;
    check(!blood_drinker_bleeds({0,500})&&!blood_drinker_bleeds({501,500}));
    for(unsigned rank=0;rank<=20;++rank)for(unsigned health:{249u,250u,251u,500u})for(bool npc:{false,true}){
        Fixture f(health);const auto target=npc?f.npc:f.target;const auto plan=*life_steal(1076,rank);
        std::vector<world::Packet> visuals;check(f.w.life_steal(f.source,target,plan,100,&visuals));
        check(f.w.resources(1)->health==std::min(500u,health+plan.amount));
        check((npc?f.w.npc_combat_snapshot(target)->health->current:double(f.w.resources(2)->health))==500-plan.amount);
        check(f.w.has_condition(f.source,Kind::bleeding,100)==(health>250));
        check(visuals==std::vector<world::Packet>{*particles::targeted_body_2022(target.actor,1,1076)});
        if(health>250){check(f.w.has_condition(f.source,Kind::bleeding,10099));check(!f.w.has_condition(f.source,Kind::bleeding,10100));}
    }
    for(unsigned mode=0;mode<4;++mode){
        Fixture f;auto target=f.target;auto plan=*life_steal(1076,12);
        if(mode==0)++target.generation;if(mode==1)target=f.source;if(mode==2)++plan.amount;if(mode==3)target.actor=65000;
        std::vector<world::Packet> visuals;check(!f.w.life_steal(f.source,target,plan,100,&visuals));
        check(visuals.empty()&&f.w.resources(1)->health==251&&!f.w.has_condition(f.source,Kind::bleeding,100));
    }
    {Fixture f;check(f.w.apply_condition(f.source,f.source,Kind::bleeding,100,20000));
        check(f.w.life_steal(f.source,f.target,*life_steal(1076,0),100));
        check(f.w.has_condition(f.source,Kind::bleeding,20099)&&!f.w.has_condition(f.source,Kind::bleeding,20100));}
    for(bool npc:{false,true})for(bool lethal:{false,true}){
        Fixture f;const auto target=npc?f.npc:f.target;
        if(lethal)f.w.set_vitals(1,{4,6,100,100},100);
        check(f.w.degeneration_hex(f.target,f.source,*degeneration_hex(19,20),100));
        check(f.w.life_steal(f.source,target,*life_steal(1076,12),100));
        check(f.w.resources(1)->health==(lethal?0u:282u));
        check((npc?f.w.npc_combat_snapshot(target)->health->current:double(f.w.resources(2)->health))==(lethal?500:444));
    }
    for(unsigned mode=0;mode<5;++mode){
        Fixture f;casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0,payments=0;
        std::vector<world::Packet> events;
        h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
        h.consume_energy_at=[&](unsigned cost,auto at){++payments;check(cost==5);return f.w.consume_energy(1,cost,f.source,at);};
        h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[](auto id,float range){return id==2&&range==1248;};
        h.life_steal=[](auto,const auto&,auto){throw std::runtime_error("Blood Drinker used plain life-steal hook");return false;};
        if(mode==0)h.life_steal={}; // No dependency on an unused callback.
        if(mode!=3)h.complete_life_steal=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
            ++calls;std::vector<world::Packet> visuals;
            const bool accepted=f.w.life_steal(f.source,target,plan,at,&visuals);return {accepted,std::move(visuals)};};
        Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[1076]=1;
        Slots bar{};bar[0]={1076,0};attributes::Ranks ranks{};ranks[4]=12;
        check(runtime.start(100,{1076,0,2},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
        if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(500,h);if(mode==4)h.complete_life_steal={};
        runtime.update(2099,h);check(calls==0);runtime.update(2100,h);
        check(calls==unsigned(mode==0)&&payments==unsigned(mode!=3));
        check(f.w.resources(2)->health==(mode==0?444u:500u));
        check(f.w.has_condition(f.source,Kind::bleeding,2100)==(mode==0));
        if(mode==0){
            const std::vector<world::Packet> expected{casting::generic(58,1,0),*casting::skill_success_body_effect(1,1076),*particles::targeted_body_2022(2,1,1076)};
            std::vector<world::Packet> actual;for(const auto& packet:events)if(packet==expected[0]||packet==expected[1]||packet==expected[2])actual.push_back(packet);
            check(actual==expected);runtime.update(2100,h);check(calls==1&&payments==1);
        }
    }
    std::cout<<"Blood Drinker ranks,pre-transfer threshold,player/NPC transfers,condition lifetime and cast controls passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
