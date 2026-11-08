// Detached until the friendly World provider is included and registered.
#include "gw_world_simulation.h"
#include "gw_npc_friendly_heal.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("friendly heal World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,target,player;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(){
        std::vector<world::Instance::Npc> npcs;
        for(unsigned id:{100u,101u}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=4;n.definition.visual_adjustment={0,0,0,100};n.x=id==100?0:100;
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=4;
            d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{id==100?200.:150.,200};
            d.damageable=true;d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
            d.healing_allowed=true;d.healing_evidence="synthetic friendly source/recipient fixture";n.combat_definition=d;npcs.push_back(n);
        }
        w.configure_npcs(std::move(npcs));w.join(1,"Ally",200,0,box,0,{}, {},100,1);
        w.set_vitals(1,{50,100,40,40},100);source=*w.effect_scope(100);target=*w.effect_scope(101);player=*w.effect_scope(1);
        check(w.configure_npc_friendly_healer(source,{286,0,0,"synthetic rank0 Healing and DivineFavor with observed spell capability"}));
    }
};
int main()try{
    {Fixture f;f.w.set_vitals(1,{95,100,40,40},100);
        const auto token=f.w.begin_npc_friendly_heal(f.source,f.player,1000);check(token.has_value());f.box->drain();
        f.w.tick(token->completes);check(f.w.resources(1)->health==100);
        const auto packets=f.box->drain();const auto actual=skills::particles::health_change_2022(1,100,5,100);
        check(std::count(packets.begin(),packets.end(),actual)==1);}
    {Fixture f;f.w.join(2,"Hex source",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        f.w.set_vitals(2,{100,100,40,40},100);const auto enemy=*f.w.effect_scope(2);
        const auto token=f.w.begin_npc_friendly_heal(f.source,f.player,1000);check(token.has_value());
        check(f.w.apply_condition(enemy,f.player,combat::conditions::Kind::deep_wound,1100,10000));
        check(f.w.degeneration_hex(enemy,f.player,*skills::effects::degeneration_hex(785,12),1100));
        check(f.w.resources(1)->health==30&&f.w.resources(1)->max_health==80);f.box->drain();
        f.w.tick(token->completes);check(f.w.resources(1)->health==51); //35 * floor(.67*.8,.6) =21
        const auto packets=f.box->drain();const auto actual=skills::particles::health_change_2022(1,100,21,80);
        check(std::count(packets.begin(),packets.end(),actual)==1);}
    {Fixture f;const auto token=f.w.begin_npc_friendly_heal(f.source,f.target,1000);check(token.has_value());
        check(f.w.npc_energy_snapshot(f.source)->current==30);
        f.w.tick(token->completes);check(f.w.npc_combat_snapshot(f.target)->health->current==185);
        f.w.tick(token->completes+1);check(f.w.npc_combat_snapshot(f.target)->health->current==185);
        check(!f.w.begin_npc_friendly_heal(f.source,f.target,token->completes));}
    {Fixture f;const auto token=f.w.begin_npc_friendly_heal(f.source,f.player,1000);check(token.has_value());
        f.w.tick(token->completes);check(f.w.resources(1)->health==85);check(f.w.npc_energy_snapshot(f.source)->current==30);}
    {Fixture f;const auto token=f.w.begin_npc_friendly_heal(f.source,f.player,1000);check(token.has_value());
        f.w.set_vitals(1,{100,100,40,40},100);f.w.tick(token->completes);
        check(f.w.resources(1)->health==100);check(f.w.npc_energy_snapshot(f.source)->current==30);
        const auto packets=f.box->drain();
        check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162&&p[2]==55;}));
        check(!f.w.begin_npc_friendly_heal(f.source,f.player,token->completes));}
    {Fixture f;const auto token=f.w.begin_npc_friendly_heal(f.source,f.player,1000);check(token.has_value());
        f.w.leave(1);f.w.join(1,"Replacement",200,0,std::make_shared<world::Outbox>(),0,{}, {},1100,1);
        f.w.set_vitals(1,{50,100,40,40},100);check(*f.w.effect_scope(1)!=f.player);
        f.w.tick(token->completes);check(f.w.resources(1)->health==50);}
    std::cout<<"Friendly NPC shared direct healing and recipient lifetime passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
