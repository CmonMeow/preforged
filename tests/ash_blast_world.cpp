#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Ash Blast World line "+std::to_string(at.line()));
}
int main()try{
    const auto plan=*skills::effects::ash_blast(0);
    for(bool npc:{false,true})for(bool burning:{false,true})for(bool reversal:{false,true})for(bool low:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        const unsigned initial=low?10:500;
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(initial),500};
            d.earth_armor=60;d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="authored Ash Blast conversion fixture";
            n.combat_definition=d;w.configure_npcs({n});}
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
        if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(2,{initial,500,100,100},100);check(w.combat_armor(*w.effect_scope(2),60));}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
        if(burning){
            if(npc){check(w.apply_condition(source,source,combat::conditions::Kind::burning,100,5000));
                check(w.condition_transfer(source,target,*skills::effects::condition_transfer(154),100));}
            else check(w.apply_condition(source,target,combat::conditions::Kind::burning,100,5000));
        }
        if(npc)check(w.stage_npc_shield_absorption(target,12,100));else check(w.status_mutation(target,1399,80,12,100,6100,true));
        if(reversal){if(npc)check(w.stage_npc_reversal(target,12,100));else check(w.status_mutation(target,307,81,12,100,8100,true));}
        const auto health=[&](){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);};
        box->drain();std::function<void()> publish;
        check(w.stage_ash_blast(source,target,plan,20,100,&publish));
        const unsigned first=reversal?(low?40:500):(low?0:470);
        check(health()==first&&box->drain().empty());publish();check(!box->drain().empty());publish();check(box->drain().empty());
        const auto entries=w.status_snapshot(target,100);
        const bool has_hex=entries&&std::any_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==1085;});
        check(has_hex==(burning&&first>0));
        if(first){
            check(w.stage_ash_blast(source,target,plan,20,100));check(health()==first-25);
            if(npc){check(w.damage_npc(source,target,{combat::Kind::armor_ignoring,50,20,42},1248,100).reason==combat::Reason::accepted);
                check(health()==std::max(0.,double(first)-60));}
            else{check(w.energy_drain(source,target,*skills::effects::energy_drain(42,12),20,100));
                check(health()==std::max(0.,double(first)-82));}
        }
    }
    for(unsigned missing_armor:{0u,1u,2u}){
        world::Instance w;
        world::Instance::Npc npc;
        npc.id=npc.definition.id=100;npc.x=125;npc.y=0;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition definition;
        definition.binding.map=146;
        definition.binding.actor=definition.binding.definition=100;
        definition.relation=combat::npcs::Relation::hostile_to_players;
        definition.spawn_health=combat::Health{500,500};
        if(missing_armor!=2)definition.earth_armor=60;
        definition.damageable=definition.conditions_allowed=true;
        npc.combat_definition=definition;
        w.configure_npcs({npc});
        auto box=std::make_shared<world::Outbox>();
        for(unsigned id=1;id<=5;++id){
            const float x=id==1?0:id==2?100:id==3?250:id==4?120:400;
            const unsigned team=id==1||id==4?1:2;
            w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),
                0,{}, {},100,team);
            w.set_vitals(id,{500,500,100,100},100);
            if(id>=2&&id!=4&&!(missing_armor==1&&id==3))
                check(w.combat_armor(*w.effect_scope(id),60));
        }
        const auto caster=*w.effect_scope(1),center=*w.effect_scope(2);
        check(w.apply_condition(caster,center,combat::conditions::Kind::burning,
            200,5000));
        box->drain();std::function<void()> publish;
        const bool accepted=w.stage_ash_blast(caster,center,plan,20,200,&publish);
        check(accepted==(missing_armor==0));
        check(box->drain().empty());
        if(!accepted){
            check(!publish);
            check(w.resources(2)->health==500&&w.resources(3)->health==500);
            check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==500);
            continue;
        }
        check(bool(publish));
        check(w.resources(2)->health==465&&w.resources(3)->health==465);
        check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==465);
        check(w.resources(4)->health==500&&w.resources(5)->health==500);
        const auto has_ash=[&](unsigned id){
            const auto status=w.status_snapshot(*w.effect_scope(id),200);
            return status&&std::any_of(status->begin(),status->end(),
                [](const auto& e){return e.skill==1085;});
        };
        check(has_ash(2)&&!has_ash(3)&&!has_ash(4));
        publish();check(!box->drain().empty());
        publish();check(box->drain().empty());
        check(!w.stage_ash_blast(caster,caster,plan,20,300));
        check(!w.stage_ash_blast(caster,*w.effect_scope(4),plan,20,300));
        check(w.resources(2)->health==465);
    }
    for(bool missing_npc_armor:{false,true}){
        world::Instance w;
        world::Instance::Npc npc;
        npc.id=npc.definition.id=100;npc.x=100;npc.y=0;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition definition;
        definition.binding.map=146;
        definition.binding.actor=definition.binding.definition=100;
        definition.relation=combat::npcs::Relation::hostile_to_players;
        definition.spawn_health=combat::Health{500,500};
        if(!missing_npc_armor)definition.earth_armor=60;
        definition.damageable=definition.conditions_allowed=true;
        npc.combat_definition=definition;w.configure_npcs({npc});
        for(unsigned id:{1u,2u}){
            w.join(id,"Actor",id==1?0:125,0,std::make_shared<world::Outbox>(),
                0,{}, {},100,id);
            w.set_vitals(id,{500,500,100,100},100);
        }
        const auto caster=*w.effect_scope(1),foe=*w.effect_scope(2);
        check(w.combat_armor(foe,60));
        check(w.stage_ash_blast(caster,*w.effect_scope(100),plan,20,200)
            ==!missing_npc_armor);
        check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current
            ==(missing_npc_armor?500:465));
        check(w.resources(2)->health==(missing_npc_armor?500u:465u));
    }
    for(bool burning:{false,true}){
        world::Instance w;
        check(w.configure_projectiles({100,32}));
        w.configure_attack_miss_draws([]{return .1;});
        for(unsigned id:{1u,2u}){
            w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),
                0,{}, {},100,id);
            w.set_vitals(id,{500,500,100,100},100);
            check(w.combat_armor(*w.effect_scope(id),60));
        }
        const auto caster=*w.effect_scope(1),foe=*w.effect_scope(2);
        if(burning)check(w.apply_condition(caster,foe,
            combat::conditions::Kind::burning,200,5000));
        check(w.stage_ash_blast(caster,foe,plan,20,200));
        const combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
        check(w.ordinary_weapon(foe,wand,20)&&w.ordinary_attack(foe,1,300));
        w.tick(800);w.tick(2000);w.cancel_ordinary_attack(foe);
        check(w.resources(1)->health==(burning?500u:480u));
    }
    for(bool breaker_on_selected:{false,true}){
        world::Instance w;
        for(unsigned id=1;id<=3;++id){
            w.join(id,"Actor",id==1?0:id==2?100:125,0,
                std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{500,500,100,100},100);
            check(w.combat_armor(*w.effect_scope(id),60));
        }
        const auto caster=*w.effect_scope(1),selected=*w.effect_scope(2),adjacent=*w.effect_scope(3);
        check(w.hex_breaker(breaker_on_selected?selected:adjacent,
            *skills::effects::hex_breaker(12),20,100));
        check(w.apply_condition(caster,selected,combat::conditions::Kind::burning,150,5000));
        check(w.apply_condition(caster,adjacent,combat::conditions::Kind::burning,150,5000));
        check(w.stage_ash_blast(caster,selected,plan,20,200));
        const auto has_ash=[&](skills::effects::Scope scope){
            const auto status=w.status_snapshot(scope,200);
            return status&&std::any_of(status->begin(),status->end(),
                [](const auto& e){return e.skill==1085;});
        };
        check(has_ash(selected)==!breaker_on_selected);
        check(has_ash(adjacent));
        check(w.resources(2)->health<500&&w.resources(3)->health<500);
    }
    {
        world::Instance w;
        world::Instance::Npc npc;
        npc.id=npc.definition.id=100;npc.x=100;npc.y=0;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition definition;
        definition.binding.map=146;
        definition.binding.actor=definition.binding.definition=100;
        definition.relation=combat::npcs::Relation::hostile_to_players;
        definition.spawn_health=combat::Health{500,500};
        definition.spawn_energy=combat::npcs::Energy{20,100};
        definition.energy_pips=0;
        definition.energy_evidence=definition.energy_pips_evidence="test";
        definition.fire_armor=definition.earth_armor=60;
        definition.damageable=definition.conditions_allowed=true;
        npc.combat_definition=definition;w.configure_npcs({npc});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{500,500,80,100},100);
        check(w.energy_capacity(1,6,0,100,{80,-2}));
        const auto caster=*w.effect_scope(1),foe=*w.effect_scope(100);
        check(w.mind_burn(caster,foe,*skills::effects::mind_burn(185),20,100));
        check(w.has_condition(foe,combat::conditions::Kind::burning,200));
        box->drain();std::function<void()> publish;
        check(w.stage_ash_blast(caster,foe,plan,20,200,&publish));
        box->drain();publish();
        const auto packets=box->drain();
        const auto is_ash_effect=[](const world::Packet& p){
            return p.size()>=8&&p[0]==65&&p[1]==0&&
                p[2]==100&&p[3]==0&&p[4]==0&&p[5]==0&&
                p[6]==(1085&255)&&p[7]==(1085>>8);
        };
        check(std::count_if(packets.begin(),packets.end(),is_ash_effect)==1);
        check(w.stage_ash_blast(caster,foe,plan,20,300));
        const auto replacement=box->drain();
        check(std::count_if(replacement.begin(),replacement.end(),is_ash_effect)==1);
        check(std::count_if(replacement.begin(),replacement.end(),
            [](const world::Packet& p){return p.size()==10&&p[0]==67&&p[1]==0&&
                p[2]==100&&p[3]==0&&p[4]==0&&p[5]==0;})==1);
    }
    std::cout<<"Ash Blast trusted player batch damage/conditional hex and deferred publication pass; NPC/cast binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
