#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Mixed Energy-drain area line "+std::to_string(at.line()));
}
int main()try{
    unsigned cases=0;
    for(unsigned skill:{39u,42u,59u})for(bool npc_target:{false,true})
    for(unsigned renewal:{0u,181u,2860u})for(unsigned mode=0;mode<5;++mode){
        ++cases;world::Instance w;std::vector<world::Instance::Npc> npcs;
        for(unsigned id=100;id<=106;++id){world::Instance::Npc n;n.id=n.definition.id=id;
            n.x=id==102?312:id==103?313:id==101?70:id==106?80:60;n.plane=id==104?1:0;
            n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
            d.binding.map=146;d.binding.actor=d.binding.definition=id;
            d.relation=id==105?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
            d.spawn_health={mode==2&&id==101?20.:900.,1000};d.damageable=true;d.conditions_allowed=!(mode==3&&id==101);
            if(!(mode==1&&id==106)&&!(mode==4&&id==100))d.spawn_energy=combat::npcs::Energy{id==100?3u:id==101?5u:7u,40};
            if(d.spawn_energy)d.energy_pips=0;n.combat_definition=d;npcs.push_back(n);}
        w.configure_npcs(npcs);
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{id==1?100u:900u,1000,id==1?50u:id==2?3u:5u,100},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(npc_target?100:2);
        if(renewal){const auto r=*effects::ether_renewal(renewal,15);check(w.status_mutation(source,renewal,80,15,100,100+r.duration,true));}
        const auto plan=*effects::energy_drain(skill,12);const bool spell=skill!=59;
        if(spell)check(w.before_spell_effect(source,skill,1,100));
        const bool expected=!(mode==1&&skill==59)&&!(mode==4&&(npc_target||skill==59));
        const bool applied=w.energy_drain(source,target,plan,20,100,nullptr,spell?std::optional<std::uint64_t>(1):std::nullopt);
        check(applied==expected);
        const unsigned damage=applied?3*plan.damage_per_energy:0;
        check(w.resources(1)->health==100+(applied&&spell&&renewal?20:0));
        check(w.resources(1)->energy==50+(applied&&spell&&renewal?effects::ether_renewal(renewal,15)->energy_per_enchantment:0));
        check(w.resources(2)->health==900-(!npc_target||plan.radius?damage:0));
        check(w.resources(3)->health==900-(plan.radius?damage:0));
        check(w.resources(2)->energy==3-(applied&&(!npc_target||plan.drain_each)?3:0));
        check(w.resources(3)->energy==5-(applied&&plan.drain_each?std::min(5u,plan.amount):0));
        combat::conditions::State weakness;check(weakness.apply(combat::conditions::Kind::weakness,1,100,10000));
        for(unsigned id=100;id<=106;++id){const auto scope=*w.effect_scope(id);const auto actor=*w.npc_combat_snapshot(scope);
            const bool included=id<=102||id==106;const bool hit=applied&&(id==(npc_target?100:2)||plan.radius&&included);
            const unsigned initial=mode==2&&id==101?20:900;
            check(actor.health->current==std::max(0.,double(initial)-(hit?damage:0)));
            if(const auto energy=w.npc_energy_snapshot(scope)){
                const unsigned before=id==100?3:id==101?5:7;
                const bool drain=applied&&(npc_target&&id==100||plan.drain_each&&included);
                check(energy->current==before-(drain?std::min(before,plan.amount):0));}
            if(applied&&skill==59&&included&&actor.health->current>0&&!(mode==3&&id==101))
                check(actor.effects==weakness.presentation(0,100));
            if(id==103||id==104||id==105)check(actor.effects==0);
        }
        if(applied&&spell){const auto before=w.resources(1)->energy;
            check(!w.energy_drain(source,target,plan,20,100,nullptr,1));check(w.resources(1)->energy==before);}
    }
    check(cases==90);std::cout<<"90 mixed player/NPC Energy-drain area/unknown/kill/immunity cases PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
