#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Restful Breeze World line "+std::to_string(at.line()));
}
bool active(world::Instance& w,skills::effects::Scope target,std::uint64_t now){
    const auto status=w.status_snapshot(target,now);
    return status&&std::any_of(status->begin(),status->end(),[](const auto& entry){return entry.skill==886;});
}
int main()try{
    for(unsigned mode=0;mode<2;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Healer",0,0,box,0,{}, {},100,1);
        w.join(2,"Resting Ally",40,0,box,0,{}, {},100,1);
        w.join(3,"Foe",80,0,box,0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{100,500,100,100},100);
        const auto caster=*w.effect_scope(1),recipient=*w.effect_scope(2),foe=*w.effect_scope(3);
        const auto plan=*skills::regeneration::reviewed(886,12);
        check(w.apply_health_regeneration(caster,2,plan,100,16100,1248,true,recipient));
        if(mode==0){const auto initial=box->drain();
            check(std::find(initial.begin(),initial.end(),*skills::particles::target_body_2022(2,886))!=initial.end());}
        check(active(w,recipient,100));
        if(mode==0){
            skills::Slots bar{};bar[0]={344,0};check(w.configure_adrenaline(recipient,bar,100));
            auto stale=recipient;++stale.generation;
            check(!w.accepted_skill_start(stale,{344,0},1000)&&active(w,recipient,1000));
            check(!w.accepted_skill_start(recipient,{344,1},1000)&&active(w,recipient,1000));
            check(w.accepted_skill_start(recipient,{344,0},1000));
        }else{
            check(w.physical_armor(foe,60));
            combat::weapon::Owned sword{1,1,{20,20,2},0,false,false,27};
            check(w.ordinary_weapon(recipient,sword,20));
            check(w.ordinary_attack(recipient,foe.actor,1000));
        }
        check(!active(w,recipient,1000));
        w.tick(2000);
        check(w.resources(2)->health==118);
    }
    for(unsigned mode=0;mode<4;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=mode==2?2000.f:100.f;
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition def;def.binding.map=146;def.binding.actor=def.binding.definition=100;
        def.relation=mode==1?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
        def.spawn_health=combat::Health{100,500};def.damageable=def.conditions_allowed=true;
        npc.combat_definition=def;w.configure_npcs({npc});
        w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(100);
        const auto plan=*skills::regeneration::reviewed(886,12);box->drain();
        auto selected=target;if(mode==3)++selected.generation;
        const bool applied=w.apply_health_regeneration(caster,100,plan,1000,17000,1248,true,selected);
        check(applied==(mode==0));
        if(mode){check(std::abs(w.npc_combat_snapshot(target)->health->current-100)<1e-7);continue;}
        const auto packets=box->drain();
        check(std::find(packets.begin(),packets.end(),*skills::particles::target_body_2022(100,886))!=packets.end());
        const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&p[0]==65;});
        check(status!=packets.end());
        const auto effect=unsigned((*status)[12])|(unsigned((*status)[13])<<8)|(unsigned((*status)[14])<<16)|(unsigned((*status)[15])<<24);
        w.tick(2000);check(std::abs(w.npc_combat_snapshot(target)->health->current-120)<1e-7);
        const auto token=w.begin_npc_action(target,combat::npcs::control::Action::attack,2000,2500);
        check(bool(token));w.tick(3000);
        check(std::abs(w.npc_combat_snapshot(target)->health->current-120)<1e-7);
        const auto ending=box->drain();
        check(std::find(ending.begin(),ending.end(),skills::casting::effect_remove(100,effect))!=ending.end());
    }
    {world::Instance w;auto box=std::make_shared<world::Outbox>();
     world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=100;
     npc.definition.visual_adjustment={0,0,0,100};
     combat::npcs::Definition def;def.binding.map=146;def.binding.actor=def.binding.definition=100;
     def.relation=combat::npcs::Relation::allied_to_players;def.spawn_health=combat::Health{100,500};
     def.damageable=def.conditions_allowed=true;npc.combat_definition=def;w.configure_npcs({npc});
     w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
     const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
     std::vector<world::Packet> visuals;std::function<void()> publish;box->drain();
     const auto plan=*skills::regeneration::reviewed(886,0);
     check(w.apply_health_regeneration(source,100,plan,1000,9000,1248,true,target,0,&visuals,&publish));
     check(visuals==std::vector<world::Packet>{*skills::particles::target_body_2022(100,886)});
     check(box->drain().empty()&&bool(publish));publish();
     const auto after=box->drain();check(std::any_of(after.begin(),after.end(),[](const auto& p){return p.size()==20&&p[0]==65;}));
     publish();check(box->drain().empty());
    }
    std::cout<<"PASS staged Restful Breeze ranked regeneration ends on accepted skill use or actual ordinary attack; target lifetime and settled Health verified\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
