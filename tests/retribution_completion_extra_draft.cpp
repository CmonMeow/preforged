// DETACHED: intentionally unregistered; parent must review/build before adoption.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Retribution extra draft line "+std::to_string(at.line()));
}
int main()try{
    for(bool self:{false,true})for(bool deferred:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.public_professions(1,3,0);
        w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Enemy",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{self?10u:100u,1000,100,100},100);
        w.set_vitals(2,{10,1000,100,100},100);w.set_vitals(3,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(self?1:2),enemy=*w.effect_scope(3);
        check(w.apply_condition(enemy,target,combat::conditions::Kind::deep_wound,100,10000));
        if(self){const auto r=*ether_renewal(181,15);check(w.status_mutation(source,181,80,15,100,100+r.duration,true));}
        // Same event, wrong admitted skill cannot satisfy paid Retribution.
        check(w.before_spell_effect(source,257,1,100));box->drain();
        check(!w.complete_retribution(source,target,*retribution(12),12,100,1).accepted);
        check(w.maintained_enchantment_count(source,100)==0&&box->drain().empty());
        check(w.before_spell_effect(source,248,2,100));box->drain();
        const auto epoch=*w.control_epoch(target);
        std::vector<world::Packet> visuals;std::function<void()> release;
        const auto result=w.complete_retribution(source,target,*retribution(12),12,100,2,deferred?&visuals:nullptr,deferred?&release:nullptr);
        check(result.accepted&&result.completion_prefix_published&&visuals.empty());
        check(w.resources(target.actor)->health==0&&*w.control_epoch(target)==epoch+1);
        check(w.maintained_enchantment_count(source,100)==0);
        if(deferred){check(box->drain().empty()&&bool(release));release();}
        const auto packets=box->drain();
        const auto finish=combat::detail::integer(58,source.actor,0);
        const auto visual=self?skills::particles::target_body_2022(target.actor,248):skills::particles::targeted_body_2022(target.actor,source.actor,248);
        check(visual&&std::count(packets.begin(),packets.end(),finish)==1&&std::count(packets.begin(),packets.end(),*visual)==1);
        if(release){release();check(box->drain().empty());}
        check(!w.complete_retribution(source,target,*retribution(12),12,100,2).accepted);
        w.tick(101);check(*w.control_epoch(target)==epoch+1);
    }
    for(bool deferred:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={10,1000};d.damageable=true;
        d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="detached lethal fixture";
        d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=4;n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,100,100},100);w.public_professions(1,3,0);
        w.join(2,"Enemy",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100),enemy=*w.effect_scope(2);
        check(w.apply_condition(enemy,target,combat::conditions::Kind::deep_wound,100,10000));
        check(w.before_spell_effect(source,248,1,100));box->drain();
        std::vector<world::Packet> visuals;std::function<void()> release;
        const auto result=w.complete_retribution(source,target,*retribution(12),12,100,1,deferred?&visuals:nullptr,deferred?&release:nullptr);
        check(result.accepted&&result.completion_prefix_published&&visuals.empty());
        check(w.npc_combat_snapshot(target)->health->current==0&&w.maintained_enchantment_count(source,100)==0);
        if(deferred){check(box->drain().empty()&&bool(release));release();}
        const auto packets=box->drain();const auto finish=combat::detail::integer(58,source.actor,0);
        const auto visual=skills::particles::targeted_body_2022(target.actor,source.actor,248);
        check(visual&&std::count(packets.begin(),packets.end(),finish)==1&&std::count(packets.begin(),packets.end(),*visual)==1);
        if(release){release();check(box->drain().empty());}
        check(!w.complete_retribution(source,target,*retribution(12),12,100,1).accepted);
    }
    for(bool npc_target:{false,true})for(bool renewal:{false,true})for(bool deferred:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();std::vector<world::Instance::Npc> population;
        world::Instance::Npc corpse;corpse.id=corpse.definition.id=101;corpse.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition dead;dead.binding.map=146;dead.binding.actor=dead.binding.definition=101;
        dead.relation=combat::npcs::Relation::hostile_to_players;dead.spawn_health={100,100};dead.damageable=true;
        dead.fleshy=true;dead.fleshy_evidence="detached well corpse";corpse.combat_definition=dead;population.push_back(corpse);
        if(npc_target){auto ally=corpse;ally.id=ally.definition.id=100;ally.x=60;auto d=dead;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={100,1000};d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=4;
            d.healing_allowed=true;d.healing_evidence="detached well ally";ally.combat_definition=d;population.push_back(ally);}
        w.configure_npcs(population);
        w.join(1,"Monk",500,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,10,100},100);w.public_professions(1,3,0);
        w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{100,1000,100,100},100);
        w.join(3,"Enemy",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(npc_target?100:2),enemy=*w.effect_scope(3);
        if(renewal){const auto r=*ether_renewal(181,15);check(w.status_mutation(source,181,80,15,100,100+r.duration,true));}
        check(w.damage_npc(enemy,*w.effect_scope(101),{combat::Kind::armor_ignoring,1000,20,194},1248,101).killed);
        check(w.claim_well_corpse(enemy,*well_cast_plan(94,12),1,102,252,[](const auto&){return true;}));
        check(w.before_spell_effect(source,248,1,252));box->drain();const auto before=*w.resources(1);
        std::vector<world::Packet> visuals;std::function<void()> release;
        const auto result=w.complete_retribution(source,target,*retribution(12),12,252,1,deferred?&visuals:nullptr,deferred?&release:nullptr);
        check(result.accepted&&result.completion_prefix_published&&visuals.empty());
        check(w.maintained_enchantment_count(source,252)==0);
        const auto statuses=w.status_snapshot(target,252);check(statuses&&std::none_of(statuses->begin(),statuses->end(),[](const auto& entry){return entry.skill==248;}));
        check((npc_target?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health))==100);
        check(w.resources(1)->health==before.health+(renewal?20u:0u));
        check(w.resources(1)->energy==before.energy+(renewal?ether_renewal(181,15)->energy_per_enchantment:0u));
        if(deferred){check(box->drain().empty()&&bool(release));release();}
        const auto packets=box->drain();const auto finish=combat::detail::integer(58,source.actor,0);
        const auto visual=skills::particles::targeted_body_2022(target.actor,source.actor,248);
        check(visual&&std::count(packets.begin(),packets.end(),finish)==1&&std::count(packets.begin(),packets.end(),*visual)==1);
        if(release){release();check(box->drain().empty());}
        check(!w.complete_retribution(source,target,*retribution(12),12,252,1).accepted);
    }
    // Self replacement must rebuild against the post-Renewal live registry.
    for(bool deferred:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,10,100},100);w.public_professions(1,3,0);
        const auto source=*w.effect_scope(1);const auto r=*ether_renewal(181,15);
        check(w.status_mutation(source,181,80,15,100,100+r.duration,true));
        check(w.stage_retribution(source,source,*retribution(0),100));
        const auto prior=w.status_snapshot(source,100);check(prior&&prior->size()==2);
        check(w.before_spell_effect(source,248,1,100));box->drain();
        std::vector<world::Packet> visuals;std::function<void()> release;
        const auto result=w.complete_retribution(source,source,*retribution(12),12,100,1,deferred?&visuals:nullptr,deferred?&release:nullptr);
        check(result.accepted&&result.completion_prefix_published==!deferred);
        check(w.resources(1)->health==178&&w.resources(1)->energy==10+2*r.energy_per_enchantment);
        check(w.maintained_enchantment_count(source,100)==1);
        const auto active=w.status_snapshot(source,100);check(active&&active->size()==2);
        check(std::count_if(active->begin(),active->end(),[](const auto& entry){return entry.skill==181;})==1);
        check(std::count_if(active->begin(),active->end(),[](const auto& entry){return entry.skill==248&&entry.rank==12;})==1);
        if(deferred){check(box->drain().empty()&&visuals.size()==1&&bool(release));release();}
        check(!box->drain().empty());if(release){release();check(box->drain().empty());}
    }
    std::cout<<"PASS detached Retribution wrong-skill/player-NPC lethal/Profane/self replacement publication draft\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
