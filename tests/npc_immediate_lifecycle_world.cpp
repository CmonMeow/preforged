#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Immediate NPC lifecycle line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,target;unsigned skill;
    std::shared_ptr<world::Outbox> observer=std::make_shared<world::Outbox>();
    Fixture(unsigned id,unsigned health=500,unsigned energy=40):skill(id){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=2;n.definition.flags=4;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding={145,100,100,0,0,0,4,2};d.relation=combat::npcs::Relation::hostile_to_players;
        d.spawn_health=combat::Health{double(health),health};d.spawn_energy=combat::npcs::Energy{energy,energy};d.energy_pips=0;d.damageable=true;
        n.combat_definition=d;w.configure_npcs({n});join(1,100,100);join(2,120,100);
        source=*w.effect_scope(100);target=*w.effect_scope(1);
        combat::ai::FlarePolicy p;p.source=source;p.level=2;p.speed=220;p.acquire=200;p.follow=1500;p.leash=1000;
        p.flight={1000,32};p.evidence="synthetic lifecycle fixture";
        check(w.configure_npc_immediate_ai(p,{{{static_cast<std::uint16_t>(id),0},*skills::effects::shadow_damage(id,0),p.evidence}},1000));
        check(w.configure_npc_wander(source,{{{80,0}},200,4000,p.evidence},1000));
    }
    void join(unsigned id,float x,unsigned now){w.join(id,"Target",x,0,id==1?observer:std::make_shared<world::Outbox>(),0,{}, {},now,1);w.set_vitals(id,{1000,1000,40,40},100);}
    auto cast(){return w.npc_flare_ai_snapshot(source)->cast;}
};
int main()try{
    for(unsigned skill:{155u}){
        Fixture f(skill);f.w.tick(1000);const auto old=f.cast();check(old.has_value());
        const auto paid=f.w.npc_energy_snapshot(f.source)->current;
        f.w.leave(1);f.join(1,100,1100);const auto replacement=*f.w.effect_scope(1);
        check(f.w.settle_resources(replacement,old->completes+3000));f.w.tick(old->completes);
        check(!f.cast()&&f.w.npc_energy_snapshot(f.source)->current==paid);
        check(f.w.resources(1)->health==1000);f.w.tick(old->completes+100);
        check(f.cast().has_value());check(f.w.npc_flare_ai_snapshot(f.source)->policy.target==*f.w.effect_scope(2));
    }
    {
        Fixture f(155);f.w.tick(1000);const auto old=*f.cast();
        f.w.move(1,1000,0,1100);f.w.tick(old.completes);
        check(!f.cast()&&f.w.resources(1)->health==1000);
        const auto paid=f.w.npc_energy_snapshot(f.source)->current;
        f.w.tick(old.completes+100);check(f.w.npc_pursuit_movement_snapshot(f.source)->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
        check(f.w.npc_energy_snapshot(f.source)->current==paid&&!f.cast());
        f.observer->drain();f.w.tick(old.completes+200);
        // Old intent travels first; selecting the moving target's newer pose
        // then records a fresh zero-budget decision, not poll-total travel.
        bool moved=false;
        const auto word=[](const world::Packet& packet,unsigned offset){return std::uint32_t(packet[offset])|
            (std::uint32_t(packet[offset+1])<<8)|(std::uint32_t(packet[offset+2])<<16)|(std::uint32_t(packet[offset+3])<<24);};
        for(const auto& packet:f.observer->drain())if(packet.size()==16&&packet[0]==44&&packet[1]==0&&word(packet,2)==f.source.actor){
            check(std::abs(std::bit_cast<float>(word(packet,6))-22.f)<.01f);
            check(std::abs(std::bit_cast<float>(word(packet,10)))<.01f);moved=true;
        }
        check(moved&&!f.cast());
        check(f.w.npc_energy_snapshot(f.source)->current==paid);
    }
    {
        Fixture f(155,20,10);check(f.w.apply_mind_wrack(f.target,f.source,12,false,1000));
        f.w.tick(1000);check(f.w.npc_combat_snapshot(f.source)->health->current==0);
        check(!f.cast()&&f.w.npc_ai_coverage().wandering==0);
    }
    std::cout<<"Immediate NPC failed target, range and source reaction retirement\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
