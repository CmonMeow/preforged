#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
using namespace fw::gw::skills;

static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Spirit Light cast line "+std::to_string(at.line()));
}

int main()try{
    for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<7;++mode){
        world::Instance w;
        if(mode==1||mode==3||mode==4){
            world::Instance::Npc npc;
            npc.id=npc.definition.id=100;npc.x=80;
            npc.definition.level=20;npc.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition body;
            body.binding.map=146;body.binding.actor=body.binding.definition=100;
            body.binding.level=20;body.spawn_health=combat::Health{20,100};
            body.damageable=body.conditions_allowed=true;
            body.relation=mode==4?combat::npcs::Relation::allied_to_players:
                combat::npcs::Relation::hostile_to_players;
            if(mode!=3){
                body.creature_type=mode==1?"spirit":"human";
                body.creature_type_evidence="fixture";
            }
            if(mode==4){body.healing_allowed=true;body.healing_evidence="fixture";}
            npc.combat_definition=body;w.configure_npcs({npc});
        }
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Ritualist",0,0,box,0,{}, {},100,1);
        w.join(2,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Foe",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{id==1?100u:20u,100,50,50},100);
        const auto source=*w.effect_scope(1);
        Slots bar{};bar[0]={915,0};
        Policy policy;policy.primary=8;policy.campaigns=255;policy.learned[915]=1;
        attributes::Ranks ranks{};ranks[33]=12;
        if(version){w.set_vitals(1,{60,100,50,50},100);
            const auto renewal=*effects::ether_renewal(version,15);
            check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));}
        unsigned stages=0,late_sacrifices=0;
        std::vector<world::Packet> sent;
        casting::Hooks h;
        h.events=[&](const auto& packets){
            sent.insert(sent.end(),packets.begin(),packets.end());box->enqueue(packets);
        };
        h.resources=[&](unsigned id){return w.resources(id);};
        h.consume_energy_at=[&](unsigned amount,auto now){
            check(amount==5);return w.consume_energy(1,amount,source,now);
        };
        h.health_sacrifice=[&](unsigned,auto)->std::optional<sacrifice::Loss>{
            ++late_sacrifices;return {};
        };
        h.target_scope=[&](unsigned id){return w.effect_scope(id);};
        h.spirit_light_target_valid=[&](auto target,auto now){
            return w.valid_gift_health_target(source,target,now);
        };
        h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(source,skill,event,now);};
        h.spirit_light_completed=[&](auto target,const auto& plan,auto event,auto now){
            ++stages;std::function<void()> publish;
            const auto loss=w.resources(target.actor)?
                w.stage_spirit_light_player(source,target,plan,now,&publish,event):
                w.stage_spirit_light_npc(source,target,plan,now,&publish,event);
            if(!loss)return casting::EffectCompletion{};
            // The matching reaction event can be consumed only once.
            check(!(w.resources(target.actor)?
                w.stage_spirit_light_player(source,target,plan,now,nullptr,event):
                w.stage_spirit_light_npc(source,target,plan,now,nullptr,event)));
            const auto visual=particles::targeted_body_2022(target.actor,1,915);
            check(bool(visual));
            return casting::EffectCompletion{true,{*visual},*loss,std::move(publish)};
        };
        casting::Runtime cast(1);
        check(casting::Runtime::effect_supported(915));
        auto missing=h;missing.spirit_light_completed={};
        casting::Runtime gated(1);
        check(gated.start(1000,{915,0,2},bar,policy,ranks,true,missing)==
            casting::Reason::unsupported);
        check(w.resources(1)->energy==50);
        check(cast.start(1000,{915,0,1},bar,policy,ranks,true,h)==casting::Reason::target);
        check(cast.start(1000,{915,0,3},bar,policy,ranks,true,h)==casting::Reason::target);
        check(w.resources(1)->energy==50);
        const auto chosen=mode==4?100u:2u;
        check(cast.start(1000,{915,0,chosen},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(1)->energy==45);
        if(mode==2)cast.cancel(h);
        if(mode==5)h.spirit_light_completed={};
        if(mode==6)h.before_spell_effect={};
        h.spirit_light=[](auto,auto,auto)->casting::EffectCompletion{throw std::runtime_error("Spirit Light fallback");};
        cast.update(1999,h);check(stages==0);
        box->drain();cast.update(2000,h);
        check(stages==(mode==2||mode>=5?0u:1u));
        check(late_sacrifices==0);
        const bool applied=mode!=2&&mode!=3&&mode<5;
        const unsigned after=(version?60u:100u)+(applied&&version?20u:0u);
        check(w.resources(1)->health==after-(applied&&mode!=1?17u:0u));
        check(w.resources(1)->energy==45+(applied&&version?effects::ether_renewal(version,15)->energy_per_enchantment:0));
        if(mode==4)check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==
            (applied?100:20));
        else check(w.resources(2)->health==(applied?100u:20u));
        const auto visual=particles::targeted_body_2022(chosen,1,915);
        check((std::find(sent.begin(),sent.end(),*visual)!=sent.end())==applied);
    }
    std::cout<<"Spirit Light Runtime target, payment, staged Health, cancellation and target visual pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
