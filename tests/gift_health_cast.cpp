#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;
using namespace fw::gw::skills;

void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Gift of Health cast line "+std::to_string(at.line()));
}

int main()try{
    for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<6;++mode){
        world::Instance w;
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,box,0,{}, {},100,1);
        w.join(2,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{500,500,100,100},100);
        w.set_vitals(2,{200,500,100,100},100);
        w.set_vitals(3,{200,500,100,100},100);
        const auto source=*w.effect_scope(1),ally=*w.effect_scope(2);
        Slots bar{};bar[0]={1121,0};bar[1]={1119,0};bar[2]={1120,0};bar[3]={887,0};
        check(w.configure_adrenaline(source,bar,100));
        std::vector<world::Packet> sent;
        casting::Hooks h;
        h.events=[&](const auto& packets){
            sent.insert(sent.end(),packets.begin(),packets.end());
            box->enqueue(packets);
        };
        h.resources=[&](unsigned actor){return w.resources(actor);};
        h.consume_energy_at=[&](unsigned amount,auto now){
            check(amount==5);return w.consume_energy(1,amount,source,now);
        };
        h.target_scope=[&](unsigned actor){return w.effect_scope(actor);};
        h.gift_health_target_valid=[&](auto target,auto now){
            return w.valid_gift_health_target(source,target,now);
        };
        if(version){const auto renewal=*effects::ether_renewal(version,15);
            w.set_vitals(1,{100,500,100,100},100);
            check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));}
        std::uint64_t reaction=0;
        h.before_spell_effect=[&](auto skill,auto event,auto now){reaction=event;return w.before_spell_effect(source,skill,event,now);};
        unsigned completed=0;
        h.gift_health_completed=[&](auto target,const auto& plan,auto divine,auto event,auto spell_event,auto now){
            ++completed;check(spell_event==reaction&&spell_event!=0);
            std::function<void()> publication;
            const bool accepted=w.stage_gift_health_player(source,target,plan,divine,event,now,&publication,spell_event);
            return casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        };
        h.prepare_skill_use=[&](auto slot,auto event,auto now){
            return w.prepare_diversion_skill_use(source,slot,event,now);
        };
        h.finalize_skill_use=[&](auto event,auto now,auto base,auto existing,bool instant){
            return w.finalize_diversion_skill_use(source,event,now,base,existing,instant);
        };
        h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(source,event);};
        h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
        h.skill_disabled_slot=[&](auto slot,auto now){return w.skill_disabled_slot(source,slot,now);};
        Policy policy;policy.primary=3;policy.campaigns=255;
        for(unsigned skill:{1121u,1119u,1120u,887u})policy.learned[skill]=1;
        attributes::Ranks ranks{};ranks[13]=12;
        casting::Runtime cast(1);
        check(casting::Runtime::effect_supported(1121));
        check(!w.valid_gift_health_target(source,source,1000));
        check(!w.valid_gift_health_target(source,*w.effect_scope(3),1000));
        check(cast.start(1000,{1121,0,1},bar,policy,ranks,true,h)==casting::Reason::target);
        check(cast.start(1000,{1121,0,3},bar,policy,ranks,true,h)==casting::Reason::target);
        check(w.resources(1)->energy==100);
        check(cast.start(1000,{1121,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(1)->energy==95);
        if(mode==1)cast.cancel(h);
        if(mode==2)cast.interrupt(1200,h);
        if(mode==3){w.leave(2);w.join(2,"Replacement",40,0,
            std::make_shared<world::Outbox>(),0,{}, {},1200,1);
            w.set_vitals(2,{200,500,100,100},100);}
        if(mode==4)h.gift_health_completed={};
        if(mode==5)h.before_spell_effect={};
        h.gift_health=[](auto,auto,auto,auto,auto)->casting::EffectCompletion{throw std::runtime_error("Gift route fell back");};
        cast.update(1749,h);check(completed==0);
        box->drain();cast.update(1750,h);cast.update(1751,h);
        check(completed==(mode==0?1u:0u));
        if(version){check(w.resources(1)->health==100+(mode==0?20:0));
            check(w.resources(1)->energy==95+(mode==0?effects::ether_renewal(version,15)->energy_per_enchantment:0));}
        check(w.resources(2)->health==(mode==0?200+effects::gift_health(12)->healing:200));
        check(w.skill_disabled_slot(source,{1119,0},1750)==(mode==0?1750+effects::gift_health(12)->disable:0));
        if(mode==0){
            check(w.skill_disabled_slot(source,{1120,0},1750)==1750+effects::gift_health(12)->disable);
            check(w.skill_disabled_slot(source,{1121,0},1750)==0);
            const auto packets=box->drain();
            const auto recharge=casting::slot_event(228,1,{1121,0},5);
            const auto body=casting::skill_success_body_effect(1,1121);
            check(body.has_value());
            const auto r=std::find(packets.begin(),packets.end(),recharge);
            const auto b=std::find(packets.begin(),packets.end(),*body);
            const auto health=std::find(packets.begin(),packets.end(),
                skills::particles::health_change_2022(2,1,effects::gift_health(12)->healing,500));
            const auto disable=std::find(packets.begin(),packets.end(),
                casting::slot_event(228,1,{1119,0},
                    unsigned(effects::gift_health(12)->disable/1000)));
            check(r!=packets.end()&&b!=packets.end()&&health!=packets.end()&&disable!=packets.end());
            check(r<b&&b<health&&health<disable);
            check(cast.start(2500,{1121,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
        }
    }
    for(unsigned version:{0u,181u,2860u}){
        world::Instance w;
        auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc npc;
        npc.id=npc.definition.id=100;npc.x=60;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition definition;
        definition.binding.map=146;
        definition.binding.actor=definition.binding.definition=100;
        definition.relation=combat::npcs::Relation::allied_to_players;
        definition.spawn_health={200,500};
        definition.damageable=true;
        definition.healing_allowed=true;
        definition.healing_evidence="fixture";
        npc.combat_definition=definition;
        w.configure_npcs({npc});
        w.join(1,"Monk",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{500,500,100,100},100);
        const auto source=*w.effect_scope(1),ally=*w.effect_scope(100);
        Slots bar{};bar[0]={1121,0};bar[1]={1119,0};
        check(w.configure_adrenaline(source,bar,100));
        casting::Hooks h;
        h.events=[&](const auto& packets){box->enqueue(packets);};
        h.resources=[&](unsigned actor){return w.resources(actor);};
        h.consume_energy_at=[&](unsigned amount,auto now){
            return w.consume_energy(1,amount,source,now);
        };
        h.target_scope=[&](unsigned actor){return w.effect_scope(actor);};
        h.gift_health_target_valid=[&](auto target,auto now){
            return w.valid_gift_health_target(source,target,now);
        };
        h.gift_health=[&](auto target,const auto& plan,auto divine,auto event,auto now){
            std::function<void()> publication;
            const bool accepted=w.stage_gift_health_npc(source,target,plan,divine,event,now,&publication);
            return casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        };
        if(version){
            const auto renewal=*effects::ether_renewal(version,15);
            w.set_vitals(1,{100,500,100,100},100);
            check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));
            h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(source,skill,event,now);};
            h.gift_health_completed=[&](auto target,const auto& plan,auto divine,auto event,auto spell_event,auto now){
                std::function<void()> publication;
                const bool accepted=w.stage_gift_health_npc(source,target,plan,divine,event,now,&publication,spell_event);
                return casting::EffectCompletion{accepted,{}, {},std::move(publication)};
            };
        }
        h.prepare_skill_use=[&](auto slot,auto event,auto now){
            return w.prepare_diversion_skill_use(source,slot,event,now);
        };
        h.finalize_skill_use=[&](auto event,auto now,auto base,auto existing,bool instant){
            return w.finalize_diversion_skill_use(source,event,now,base,existing,instant);
        };
        h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(source,event);};
        h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
        h.skill_disabled_slot=[&](auto slot,auto now){return w.skill_disabled_slot(source,slot,now);};
        Policy policy;policy.primary=3;policy.campaigns=255;
        policy.learned[1121]=policy.learned[1119]=1;
        attributes::Ranks ranks{};ranks[13]=12;
        casting::Runtime cast(1);
        check(w.valid_gift_health_target(source,ally,1000));
        check(cast.start(1000,{1121,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted);
        box->drain();cast.update(1750,h);
        check(w.npc_combat_snapshot(ally)->health->current==200+effects::gift_health(12)->healing);
        if(version){check(w.resources(1)->health==120);
            check(w.resources(1)->energy==95+effects::ether_renewal(version,15)->energy_per_enchantment);}
        check(w.skill_disabled_slot(source,{1119,0},1750)==1750+effects::gift_health(12)->disable);
        const auto packets=box->drain();
        check(std::find(packets.begin(),packets.end(),
            skills::particles::health_change_2022(100,1,effects::gift_health(12)->healing,500))!=packets.end());
    }
    std::cout<<"Gift of Health cast target, payment, cancellation, heal, disable and native body ordering pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
