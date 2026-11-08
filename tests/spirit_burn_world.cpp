#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;
using namespace skills;
using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Spirit Burn World line "+std::to_string(at.line()));
}
int main()try{
    for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
        world::Instance w;
        std::vector<world::Instance::Npc> npcs;
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
            n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;
            d.binding.actor=d.binding.definition=100;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={500,500};d.damageable=true;d.conditions_allowed=true;
            d.lightning_armor=60;d.lightning_armor_evidence="fixture";
            if(mode!=2){d.creature_type="human";d.creature_type_evidence="fixture";}
            n.combat_definition=d;npcs.push_back(n);
        }
        if(mode==1||mode>=3||(mode==2&&!npc)){
            world::Instance::Npc n;n.id=n.definition.id=101;n.x=mode==3?1013:mode==4?1012:100;if(mode==5)n.plane=1;
            n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;
            d.binding.actor=d.binding.definition=101;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={100,100};d.damageable=true;
            if(mode==1||mode>=3){d.creature_type="spirit";d.creature_type_evidence="fixture";}
            n.combat_definition=d;npcs.push_back(n);
        }
        if(!npcs.empty())w.configure_npcs(npcs);
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Ritualist",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{500,500,50,100},100);
        if(!npc){w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(2,{500,500,50,100},100);
            check(w.lightning_armor(*w.effect_scope(2),60));}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
        auto plan=*elemental(919,15);
        check(plan.kind==combat::Kind::lightning&&plan.burning_ms==5000);
        box->drain();std::vector<Packet> visuals;
        const auto accepted=w.elemental(source,target,plan,20,200,&visuals);
        if(accepted!=(mode!=2))throw std::runtime_error("accept mismatch npc="+std::to_string(npc)+" mode="+std::to_string(mode));
        check(w.resources(1)->energy==50u);
        check(w.has_condition(target,combat::conditions::Kind::burning,200)==(mode==1||mode==4));
        check(!w.has_condition(target,combat::conditions::Kind::burning,5200));
        if(!accepted){
            const auto health=npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);check(health==500);
        }
        if(accepted){
            const auto health=npc?w.npc_combat_snapshot(target)->health->current:
                double(w.resources(2)->health);
            check(health<500);
            check(std::count(visuals.begin(),visuals.end(),
                *particles::targeted_body_2022(target.actor,source.actor,919))==1);
        }
    }
    for(bool spirit:{false,true}){
        world::Instance w;
        if(spirit){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
            n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;
            d.binding.actor=d.binding.definition=100;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={100,100};d.damageable=true;
            d.creature_type="spirit";d.creature_type_evidence="fixture";
            n.combat_definition=d;w.configure_npcs({n});
        }
        w.join(1,"Ritualist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{500,500,50,100},100);
        w.set_vitals(2,{500,500,50,100},100);
        check(w.lightning_armor(*w.effect_scope(2),60));
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        Policy policy;policy.primary=8;policy.campaigns=255;policy.learned[919]=1;
        Slots bar{};bar[0]={919,0};attributes::Ranks ranks{};ranks[34]=15;
        casting::Hooks h;std::vector<Packet> packets;unsigned commits=0;
        h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        h.resources=[&](auto id){return w.resources(id);};
        h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(source,id,range,1500);};
        h.consume_energy_at=[&](auto cost,auto at){check(cost==5);return w.consume_energy(1,cost,source,at);};
        h.elemental=[&](auto selected,const auto& plan,auto at){
            ++commits;std::vector<Packet> visuals;
            const bool ok=w.elemental(source,selected,plan,20,at,&visuals);
            return casting::EffectCompletion{ok,std::move(visuals)};};
        casting::Runtime cast(1);
        check(cast.start(500,{919,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(1)->energy==45&&commits==0);
        cast.update(1500,h);
        check(commits==1&&w.resources(1)->energy==45u);
        check(w.resources(2)->health<500);
        check(w.has_condition(target,combat::conditions::Kind::burning,1500)==spirit);
        check(!w.has_condition(target,combat::conditions::Kind::burning,6500));
        cast.update(1500,h);check(commits==1&&w.resources(1)->energy==45);
        const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
        const auto body=std::find(packets.begin(),packets.end(),
            *particles::targeted_body_2022(target.actor,source.actor,919));
        check(finish!=packets.end()&&finish<body&&body!=packets.end());
    }
    std::cout<<"PASS Spirit Burn paid cast, player/NPC lightning and completion-time conditional Burning and replay\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
