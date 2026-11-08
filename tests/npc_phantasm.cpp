#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("NPC Phantasm line "+std::to_string(at.line()));
}
combat::npcs::Definition body(unsigned id,bool source,unsigned energy=50){
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
    d.relation=source?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
    d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;
    d.spawn_energy=combat::npcs::Energy{energy,50};d.energy_pips=0;d.healing_allowed=true;d.healing_evidence="authored fixture";return d;
}
struct Fixture {
    world::Instance w;Scope source,target,monk;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(unsigned energy=50,bool spellbook=true,float distance=100){
        std::vector<world::Instance::Npc> npcs;
        for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?0:distance;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};n.combat_definition=body(id,id==100,energy);npcs.push_back(n);}
        w.configure_npcs(npcs);w.join(1,"Monk",50,0,box,0,{{165,0,1,0,0,0,3,0}},{},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        source=*w.effect_scope(100);target=*w.effect_scope(101);monk=*w.effect_scope(1);
        if(spellbook)check(w.configure_npc_spellbook(source,{{{31,0},1248,"authored Conjure Phantasm fixture"}}));box->drain();
    }
};
int main()try{
    for(unsigned mode=0;mode<7;++mode){Fixture f(mode==1?9u:50u,mode!=2,mode==3?1250.f:100.f);
        const auto token=f.w.begin_npc_phantasm(f.source,f.target,12,100);
        if(mode>=1&&mode<=3){check(!token&&f.w.npc_energy_snapshot(f.source)->current==(mode==1?9u:50u));continue;}
        check(token&&token->completes==1100&&f.w.npc_energy_snapshot(f.source)->current==40);
        check(!f.w.begin_npc_phantasm(f.source,f.target,12,101)&&!f.w.complete_npc_phantasm(*token,1099));
        check(!f.w.complete_npc_action(*token,1100));
        if(mode==4){check(f.w.interrupt_skill(f.monk,f.source,*interrupt_skill(23,0),20,500));f.w.tick(2100);check(f.w.status_snapshot(f.target,2100)->empty());continue;}
        if(mode==5){check(f.w.damage_npc(f.monk,f.source,{combat::Kind::armor_ignoring,1000,20,133},1248,500).killed);f.w.tick(2100);check(f.w.status_snapshot(f.target,2100)->empty());continue;}
        if(mode==6){auto forged=*token;++forged.sequence;check(!f.w.complete_npc_phantasm(forged,1100));}
        f.w.tick(2100);const auto active=f.w.status_snapshot(f.target,2100);check(active&&active->size()==1&&active->front().skill==31&&active->front().cause==100);
        check(active->front().applied==1100&&active->front().expires==1100+degeneration_hex(31,12)->duration);
        check(std::abs(f.w.npc_combat_snapshot(f.target)->health->current-490)<1e-7);
        check(!f.w.complete_npc_phantasm(*token,2100));const auto packets=f.box->drain();const auto visual=*skills::particles::targeted_body_2022(101,100,31);
        check(std::count(packets.begin(),packets.end(),visual)==1&&std::count(packets.begin(),packets.end(),combat::detail::integer(58,100,0))==1);
        check(f.w.complete_convert_hexes(f.monk,f.target,*convert_hexes(12),0,2100));
        check(f.w.status_snapshot(f.target,2100)->empty()&&f.w.convert_hexes_armor(f.target,2100)==0); // Mesmer hex gives no Necromancer armor.
        f.w.tick(3100);check(std::abs(f.w.npc_combat_snapshot(f.target)->health->current-490)<1e-7);
        check(!f.w.begin_npc_phantasm(f.source,f.target,12,3100));check(f.w.begin_npc_phantasm(f.source,f.target,12,6100).has_value());
    }
    {Fixture f;auto stale=f.target;++stale.generation;check(!f.w.begin_npc_phantasm(f.source,stale,12,100));
        check(!f.w.begin_npc_phantasm(f.target,f.source,12,100));check(!f.w.begin_npc_phantasm(f.source,f.target,21,100));}
    for(unsigned rank=0;rank<=20;++rank){
        Fixture f;const auto plan=*degeneration_hex(31,rank);
        check(f.w.begin_npc_phantasm(f.source,f.target,rank,100).has_value());
        const auto end=1100+plan.duration;f.w.tick(end+1000);
        check(f.w.status_snapshot(f.target,end+1000)->empty());
        check(std::abs(f.w.npc_combat_snapshot(f.target)->health->current-(500.-10.*plan.duration/1000.))<1e-7);
        auto stale=f.target;++stale.generation;check(!f.w.status_snapshot(stale,end+1000));
    }
    {combat::npcs::LiveActor ally(body(101,false),{1,1,1,146,101});const auto life=ally.lifetime();
        check(!ally.apply_single_degeneration_hex(life,*degeneration_hex(31,12),100,1,100));
        auto stale=life;++stale.actor_generation;
        check(!ally.apply_conjure_phantasm_from_npc(stale,{1,100,1},12,1,100));
        check(!ally.apply_conjure_phantasm_from_npc(life,{2,100,1},12,1,100));
        check(!ally.apply_conjure_phantasm_from_npc(life,{1,101,1},12,1,100));
        check(ally.apply_conjure_phantasm_from_npc(life,{1,100,1},12,1,100).has_value());}
    {world::Instance w;world::Instance::Npc guard;guard.id=guard.definition.id=100;guard.definition.level=20;guard.x=1000;
        guard.definition.visual_adjustment={0,0,0,100};guard.combat_definition=body(100,false);
        guard.combat_definition->relation=combat::npcs::Relation::neutral;
        auto ally=guard;ally.id=ally.definition.id=101;ally.x=100;ally.combat_definition=body(101,false);
        w.configure_npcs({guard,ally});auto box=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        const auto owner=*w.effect_scope(1),guardian=*w.effect_scope(100),target=*w.effect_scope(101);
        auto enemy=guard;enemy.x=0;enemy.combat_definition=body(100,true);
        check(w.begin_wave_objective(owner,guardian,73,1,3,1)&&w.spawn_quest_wave(owner,73,1,{enemy},[]{return true;}));
        const auto source=*w.effect_scope(60000);
        check(w.chase_quest_guardian(source,100,100));
        check(w.configure_npc_spellbook(source,{{{31,0},1248,"authored moving caster"}}));box->drain();
        check(w.begin_npc_phantasm(source,target,12,600).has_value());
        const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),world::Packet{40,0,96,234,0,0})==1);
        w.tick(2600);check(w.status_snapshot(target,2600)->size()==1);
        const auto later=box->drain();
        check(std::none_of(later.begin(),later.end(),[](const auto& p){return p.size()>5&&p[0]==41&&p[2]==(60000&255)&&p[3]==(60000>>8);}));
    }
    {Fixture f;
        check(f.w.begin_npc_phantasm(f.source,f.target,12,100).has_value());f.w.tick(6100);
        const auto old=f.w.status_snapshot(f.target,6100)->front().instance;
        check(f.w.begin_npc_phantasm(f.source,f.target,0,6100).has_value());f.w.tick(7100);
        const auto active=*f.w.status_snapshot(f.target,7100);
        check(active.size()==1&&active.front().instance!=old&&active.front().rank==0);
        check(active.front().expires==9100&&f.w.npc_energy_snapshot(f.source)->current==30);
        f.w.tick(15100);check(f.w.status_snapshot(f.target,15100)->empty());
        check(std::abs(f.w.npc_combat_snapshot(f.target)->health->current-420)<1e-7); //6s original+2s replacement,never stacked.
    }
    {Fixture f;
        const auto pending=*f.w.begin_npc_phantasm(f.source,f.target,12,100);
        f.w.leave(1);f.w.configure_npcs({});f.w.tick(2100);
        check(!f.w.complete_npc_phantasm(pending,2100));
        check(!f.w.status_snapshot(f.target,2100)&&!f.w.npc_energy_snapshot(f.source));
    }
    std::cout<<"PASS NPC paid Phantasm token/timing/cooldown/interrupt/death/identity,refresh/retirement,exact sparse degeneration and real Convert Hexes removal on allied NPC;no Necromancer armor for Mesmer hex\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}




