#include "gw_ray_of_judgment.h"
#include "gw_damage_fields.h"
#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace fw::gw;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Ray of Judgment plan line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned rank:{0u,12u,15u,20u}){
        const auto plan=skills::effects::ray_of_judgment(rank);check(bool(plan));
        check(plan->pulses==5&&plan->interval_ms==1000&&plan->duration_ms==5000);
        const auto ticks=skills::effects::ray_of_judgment_pulses(*plan,10000);
        check(ticks&&*ticks==std::array<std::uint64_t,5>{11000,12000,13000,14000,15000});
    }
    check(skills::effects::ray_of_judgment(0)->damage==5);
    check(skills::effects::ray_of_judgment(12)->damage==37);
    check(skills::effects::ray_of_judgment(15)->damage==45);
    check(skills::effects::ray_of_judgment(12)->burning_seconds==3);
    check(!skills::effects::ray_of_judgment(21));
    auto forged=*skills::effects::ray_of_judgment(12);++forged.damage;
    check(!skills::effects::ray_of_judgment_pulses(forged,10000));
    check(!skills::effects::ray_of_judgment_pulses(
        *skills::effects::ray_of_judgment(12),UINT64_MAX));
    {
        using namespace skills::effects;
        const auto field=damage_field(830,12);
        check(field&&field->kind==FieldDamage::holy&&field->damage==37&&
              field->pulses==5&&field->radius==166&&field->burning_ms==3000);
        check(!damage_field(830,12,true));
        DamageFieldSchedule schedule;
        const Scope caster{9,42,1};
        check(schedule.add(caster,20,125.f,-70.f,0,*field,10000));
        auto forged=*field;++forged.damage;
        check(!schedule.add(caster,20,125.f,-70.f,0,forged,10000));
        std::vector<std::uint64_t> deadlines;
        schedule.advance(10999,[&](const auto&){check(false);return true;});
        schedule.advance(15000,[&](const auto& pulse){
            check(pulse.caster==caster&&pulse.x==125.f&&pulse.y==-70.f&&
                  pulse.plan.burning_ms==3000);
            deadlines.push_back(pulse.at);return true;
        });
        check(deadlines==std::vector<std::uint64_t>{11000,12000,13000,14000,15000}&&schedule.empty());
    }
    {
        using namespace world;
        const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
        auto catalog=load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 1 "+hash+"\nsource test authored "+hash+" tests/ray_of_judgment.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test\n");
        bind_combat_catalog(catalog,combat::npcs::load_catalog(metadata,hash),hash);
        auto npcs=catalog.for_map(146);
        npcs.front().combat_definition->holy_traits={false,true};
        Instance w;w.configure_npcs(npcs);
        const auto box=std::make_shared<Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{100,100,40,40},100);
        const auto caster=*w.effect_scope(1),foe=*w.effect_scope(100);
        const auto plan=*skills::effects::ray_of_judgment(12);
        auto forged=plan;++forged.damage;
        check(!w.ray_of_judgment(caster,foe,forged,20,100));
        check(w.ray_of_judgment(caster,foe,plan,20,100));
        const auto selected=*skills::particles::targeted_body_2022(foe.actor,caster.actor,830);
        const auto completion=box->drain();
        check(std::count(completion.begin(),completion.end(),selected)==1);
        w.tick(1099);check(w.npc_combat_snapshot(foe)->health->current==500);
        w.tick(1200);
        check(w.npc_combat_snapshot(foe)->health->current<500);
        const auto first=box->drain();
        check(std::count(first.begin(),first.end(),skills::particles::ray_of_judgment_pulse_2022(foe.actor,caster.actor))==1);
        check(w.has_condition(foe,combat::conditions::Kind::burning,1200));
        w.tick(2200);check(w.has_condition(foe,combat::conditions::Kind::burning,4100));
    }
    {
        using namespace world;
        Instance w;
        w.join(1,"Caster",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        w.join(2,"Enemy",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{100,100,40,40},100);
        w.set_vitals(2,{500,500,40,40},100);
        const auto caster=*w.effect_scope(1),foe=*w.effect_scope(2);
        check(w.player_holy_equipment(foe,0,100));
        check(w.ray_of_judgment(caster,foe,*skills::effects::ray_of_judgment(12),20,100));
        w.move(2,1000,0,100);
        w.tick(5200);
        check(w.resources(2)->health==500);
        check(!w.has_condition(foe,combat::conditions::Kind::burning,5200));
    }
    {
        using namespace world;
        Instance w;auto observer=std::make_shared<Outbox>();
        w.join(1,"Monk",250,0,observer,0,{}, {},100,1);
        w.join(2,"Seed owner",100,0,std::make_shared<Outbox>(),0,{}, {},100,1);
        w.join(3,"Caster",0,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{500,1000,100,100},100);
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
        const auto monk=*w.effect_scope(1),owner=*w.effect_scope(2),caster=*w.effect_scope(3);
        check(w.complete_healing_seed(monk,owner,*skills::effects::healing_seed(0),0,100));
        check(w.player_holy_equipment(owner,0,100));
        check(w.ray_of_judgment(caster,owner,*skills::effects::ray_of_judgment(0),20,100));
        observer->drain();
        w.tick(1100);
        check(w.resources(1)->health==503&&w.resources(2)->health==498);
        const auto packets=observer->drain();
        const auto heal=skills::particles::health_change_2022(1,1,3,1000);
        const auto damage=std::find_if(packets.begin(),packets.end(),[](const auto& packet){
            return !packet.empty()&&packet[0]==162;
        });
        const auto healed=std::find(packets.begin(),packets.end(),heal);
        check(damage!=packets.end()&&healed!=packets.end()&&damage<healed);
    }
    for(unsigned mode=0;mode<5;++mode){
        using namespace world;using namespace skills;
        Instance w;auto out=std::make_shared<Outbox>();
        w.join(1,"Caster",0,0,out,0,{}, {},100,1);
        w.join(2,"Enemy",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{100,100,40,40},100);w.set_vitals(2,{500,500,40,40},100);
        const auto caster=*w.effect_scope(1),foe=*w.effect_scope(2);
        check(w.player_holy_equipment(foe,0,100));out->drain();
        Slots bar{};bar[0]={830,0};Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[830]=1;
        attributes::Ranks ranks{};ranks[14]=12;
        casting::Hooks hooks;std::vector<combat::Packet> sent;unsigned commits=0;
        hooks.resources=[&](unsigned id){return w.resources(id);};
        hooks.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
        hooks.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(1,cost,caster,at);};
        hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};
        hooks.projectile_target_valid=[&](unsigned id,float range){return id==2&&range==1248.f;};
        if(mode!=4)hooks.complete_ray_of_judgment=[&](auto target,const auto& plan,auto at)->std::optional<std::vector<combat::Packet>>{
            ++commits;if(mode==3)return {};
            std::vector<combat::Packet> visuals;
            if(!w.ray_of_judgment(caster,target,plan,20,at,&visuals))return {};
            return visuals;
        };
        casting::Runtime cast(1);
        check(casting::Runtime::effect_supported(830));
        check(cast.start(1000,{830,0,2},bar,policy,ranks,true,hooks)==
              (mode==4?casting::Reason::unsupported:casting::Reason::accepted));
        if(mode==1)cast.cancel(hooks);
        if(mode==2)cast.interrupt(1500,hooks);
        cast.update(2999,hooks);check(!commits);
        cast.update(3000,hooks);check(commits==unsigned(mode==0||mode==3));
        const auto target_body=*particles::targeted_body_2022(2,1,830);
        if(mode==0){
            const auto finish=std::find(sent.begin(),sent.end(),casting::generic(58,1,0));
            const auto caster_body=std::find(sent.begin(),sent.end(),*casting::success_body_effect(1,830));
            const auto selected=std::find(sent.begin(),sent.end(),target_body);
            check(finish!=sent.end()&&caster_body!=sent.end()&&selected!=sent.end()&&finish<caster_body&&caster_body<selected);
            check(std::count(sent.begin(),sent.end(),target_body)==1);
        }else check(std::count(sent.begin(),sent.end(),target_body)==0);
        check(w.resources(1)->energy==(mode==4?40u:30u));
        w.tick(4100);
        check((w.resources(2)->health<500)==(mode==0));
    }
    std::cout<<"PASS pinned Ray of Judgment rank and five-pulse schedule\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
