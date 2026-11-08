#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Signet Sorrow World line "+std::to_string(at.line()));
}
int main()try{
    for(int corpse_flesh:{1,0,-1})for(float corpse_x:{551.f,552.f,552.1f}){
        world::Instance w;std::vector<world::Instance::Npc> npcs;
        for(unsigned id:{100u,101u}){
            world::Instance::Npc n;n.id=n.definition.id=id;
            n.definition.visual_adjustment={0,0,0,100};
            n.x=id==100?300:corpse_x;
            combat::npcs::Definition d;d.binding.map=146;
            d.binding.actor=d.binding.definition=id;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{100,100};d.damageable=true;
            if(id==101&&corpse_flesh>=0){d.fleshy=corpse_flesh!=0;d.fleshy_evidence="test authored";}
            n.combat_definition=d;npcs.push_back(n);
        }
        w.configure_npcs(npcs);
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(100),corpse=*w.effect_scope(101);
        auto facts=w.signet_sorrow_facts(caster,target,500);
        check(facts&&facts->target_is_foe&&facts->target_alive&&
              !facts->nearest_corpse_distance);
        check(w.damage_npc(caster,corpse,{combat::Kind::armor_ignoring,1000,20,194},1248,600).killed);
        facts=w.signet_sorrow_facts(caster,target,1000);
        check(facts&&bool(facts->nearest_corpse_distance)==(corpse_flesh==1));
        const auto hit=skills::effects::prepare_signet_sorrow(*skills::effects::signet_sorrow(12),20,*facts);
        check(hit&&hit->recharge_ms==(corpse_flesh==1&&corpse_x<=552.f?0u:30000u));
        check(!w.signet_sorrow_facts(caster,corpse,1000));
        box->drain();
        const auto completion=w.complete_signet_sorrow(caster,target,
            *skills::effects::signet_sorrow(12),20,1000);
        check(completion.accepted&&completion.recharge_ms==hit->recharge_ms);
        check(completion.visuals.empty());
        check(w.npc_combat_snapshot(target)->health->current==37);
        check(box->drain().empty());
        check(bool(completion.publication));completion.publication();
        check(!box->drain().empty());
        auto invalid=*skills::effects::signet_sorrow(12);++invalid.damage;
        check(!w.complete_signet_sorrow(caster,target,invalid,20,1100).accepted);
    }
    for(unsigned health:{50u,1000u}){
        world::Instance w;
        auto caster_box=std::make_shared<world::Outbox>();
        auto target_box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,caster_box,0,{}, {},100,1);
        w.join(2,"Enemy",300,0,target_box,0,{}, {},100,2);
        w.join(3,"Ally",301,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(4,"Second Enemy",551,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{500,500,100,100},100);
        w.set_vitals(2,{health,1000,100,100},100);
        w.set_vitals(3,{1000,1000,100,100},100);
        w.set_vitals(4,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(!w.signet_sorrow_facts(source,*w.effect_scope(3),500));
        check(!w.complete_signet_sorrow(source,*w.effect_scope(3),
            *skills::effects::signet_sorrow(12),20,1000).accepted);
        const auto facts=w.signet_sorrow_facts(source,target,500);
        check(facts&&facts->target_is_foe&&!facts->nearest_corpse_distance);
        caster_box->drain();target_box->drain();
        const auto completion=w.complete_signet_sorrow(source,target,
            *skills::effects::signet_sorrow(12),20,1000);
        check(completion.accepted&&completion.recharge_ms==30000);
        check(w.resources(2)->health==(health==50?0:health-63));
        check(caster_box->drain().empty()&&target_box->drain().empty());
        completion.publication();
        check(!caster_box->drain().empty()&&!target_box->drain().empty());
        if(health==50){
            check(!w.signet_sorrow_facts(source,target,1100));
            const auto second=*w.effect_scope(4);
            const auto corpse_facts=w.signet_sorrow_facts(source,second,1100);
            check(corpse_facts&&corpse_facts->nearest_corpse_distance&&
                  *corpse_facts->nearest_corpse_distance==251.f);
            const auto instant=skills::effects::prepare_signet_sorrow(
                *skills::effects::signet_sorrow(12),20,*corpse_facts);
            check(instant&&instant->recharge_ms==0);
        }
    }
    {
        world::Instance w;
        world::Instance::Npc pet;pet.id=pet.definition.id=101;
        pet.definition.visual_adjustment={0,0,0,100};pet.x=900;
        combat::npcs::Definition d;d.binding.map=146;
        d.binding.actor=d.binding.definition=101;
        d.relation=combat::npcs::Relation::hostile_to_players;
        d.spawn_health=combat::Health{40,40};d.damageable=true;
        pet.combat_definition=d;w.configure_npcs({pet});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.join(2,"Pet Owner",300,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.join(3,"Other Foe",400,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2),other=*w.effect_scope(3);
        const auto companion=*w.effect_scope(101);
        auto stale=companion;++stale.generation;
        check(!w.bind_pet_owner(owner,stale));
        auto stale_owner=owner;++stale_owner.generation;
        check(!w.bind_pet_owner(stale_owner,companion));
        check(w.bind_pet_owner(owner,companion));
        check(!w.bind_pet_owner(other,companion));
        check(!w.signet_sorrow_facts(caster,owner,200)->dead_pet);
        check(w.damage_npc(caster,companion,
            {combat::Kind::armor_ignoring,1000,20,194},1248,600).killed);
        const auto facts=w.signet_sorrow_facts(caster,owner,1000);
        check(facts&&facts->dead_pet&&!facts->nearest_corpse_distance);
        check(!w.signet_sorrow_facts(caster,other,1000)->dead_pet);
        check(skills::effects::prepare_signet_sorrow(
            *skills::effects::signet_sorrow(12),20,*facts)->recharge_ms==0);
    }
    for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
        world::Instance w;
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={double(hp),1000};d.damageable=true;d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";
            n.combat_definition=d;w.configure_npcs({n});
        }else{w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);}
        auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
        if(npc)check(w.stage_npc_reversal(target,rr,100));else check(w.status_mutation(target,307,5,rr,100,8100,true));
        box->drain();
        const auto plan=*skills::effects::signet_sorrow(12);const auto completion=w.complete_signet_sorrow(source,target,plan,20,1000);
        check(completion.accepted&&completion.recharge_ms==30000&&completion.visuals.empty()&&bool(completion.publication));
        auto publication=completion.publication;const double raw=plan.damage;
        const double converted=std::min(raw,double(skills::effects::reversal_fortune(rr)->maximum_healing));
        const double expected=std::max(0.,std::min(1000.,hp+converted)-(raw-converted));
        const double actual=npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;check(std::abs(actual-expected)<1.1);
        check(box->drain().empty());publication();check(!box->drain().empty());publication();check(box->drain().empty());
        if(npc)check(w.take_npc_deaths(source).size()==unsigned(expected<=0));
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
