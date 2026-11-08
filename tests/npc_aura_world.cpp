#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC Aura World line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& packet,std::size_t at){return unsigned(packet[at])|unsigned(packet[at+1])<<8|
    unsigned(packet[at+2])<<16|unsigned(packet[at+3])<<24;}
struct Fixture {
    world::Instance w;skills::effects::Scope source,owner;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(unsigned health=150,unsigned energy=40){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=4;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=161;d.binding.actor=d.binding.definition=100;d.binding.level=4;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),200};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{energy,40};d.energy_pips=0;d.healing_allowed=true;d.healing_evidence="synthetic typed Aura self-healing fixture";
        d.knockdown_allowed=true;d.knockdown_evidence="synthetic knockdown fixture";n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Caster",100,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,40,40},100);
        source=*w.effect_scope(100);owner=*w.effect_scope(1);
        check(w.configure_npc_spellbook(source,{{{180,0},0,"synthetic Aura self",combat::ai::SpellTargetClass::self}}));
    }
    double health(){return w.npc_combat_snapshot(source)->health->current;}
};
int main()try{
    {Fixture f;const auto first=f.w.begin_npc_aura(f.source,*skills::effects::aura_restoration_plan(0),1000);
        check(first.has_value());check(f.w.npc_energy_snapshot(f.source)->current==35);
        const auto starts=f.box->drain();check(std::count_if(starts.begin(),starts.end(),[](const auto& packet){
            return packet.size()==14&&packet[0]==158&&packet[1]==0&&word(packet,2)==60&&word(packet,6)==100&&word(packet,10)==180;
        })==1); //untargeted start, never self disguised as a foe
        check(!f.w.complete_npc_action(*first,1250));check(!f.w.complete_npc_aura(*first,1249));
        check(f.w.complete_npc_aura(*first,1250));check(f.health()==150); //first Aura never triggers itself
        const auto old=f.w.npc_aura_snapshot(f.source,1250);check(old&&old->rank==0);
        check(!f.w.complete_npc_aura(*first,1250));check(f.health()==150);
        check(!f.w.begin_npc_aura(f.source,*skills::effects::aura_restoration_plan(0),2000));
        const auto renewal=f.w.begin_npc_aura(f.source,*skills::effects::aura_restoration_plan(0),22000);
        check(renewal.has_value());check(f.w.complete_npc_aura(*renewal,22250));check(f.health()==160);
        const auto current=f.w.npc_aura_snapshot(f.source,22250);check(current&&current->instance!=old->instance);
        check(f.w.npc_energy_snapshot(f.source)->current==30);
        f.w.tick(82250);check(!f.w.npc_aura_snapshot(f.source,82250));}
    {Fixture f;const auto cast=f.w.begin_npc_aura(f.source,*skills::effects::aura_restoration_plan(0),1000);check(cast.has_value());
        check(f.w.knockdown_npc(f.source,2000,1100)==combat::npcs::control::Knockdown::accepted);
        f.w.tick(1250);check(!f.w.npc_aura_snapshot(f.source,1250));check(f.health()==150);
        check(f.w.npc_energy_snapshot(f.source)->current==35);}
    {Fixture f;check(f.w.backfire(f.owner,f.source,*skills::effects::backfire(28,20),20,900));
        const auto cast=f.w.begin_npc_aura(f.source,*skills::effects::aura_restoration_plan(0),1000);check(cast.has_value());
        check(f.w.complete_npc_aura(*cast,1250));check(f.health()==0);check(!f.w.npc_aura_snapshot(f.source,1250));}
    {Fixture f(1,5);check(f.w.apply_mind_wrack(f.owner,f.source,12,false,900));
        const auto cast=f.w.begin_npc_aura(f.source,*skills::effects::aura_restoration_plan(0),1000);check(cast.has_value());
        check(f.health()==0&&f.w.npc_energy_snapshot(f.source)->current==0);
        check(!f.w.complete_npc_aura(*cast,1250));check(!f.w.npc_aura_snapshot(f.source,1250));}
    std::cout<<"NPC Aura ownership, renewal, interruption and lethal Backfire passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
