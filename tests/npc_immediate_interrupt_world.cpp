#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Immediate NPC lifecycle line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,target;unsigned skill;
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
    void join(unsigned id,float x,unsigned now){w.join(id,"Target",x,0,std::make_shared<world::Outbox>(),0,{}, {},now,1);w.set_vitals(id,{1000,1000,40,40},100);}
    auto cast(){return w.npc_flare_ai_snapshot(source)->cast;}
};
int main()try{
 for(unsigned skill:{23u,57u}){
  Fixture f(155);f.w.tick(1000);const auto token=*f.cast();const auto hp=f.w.npc_combat_snapshot(f.source)->health->current;
  if(skill==23){check(f.w.interrupt_skill(f.target,f.source,*skills::effects::interrupt_skill(23,12),20,1100));
   check(f.w.npc_combat_snapshot(f.source)->health->current==hp);check(f.w.npc_flare_ai_snapshot(f.source)->cast==token);
   f.w.tick(token.completes);check(f.w.resources(1)->health==980);
  }else{check(f.w.cry_frustration(f.target,f.source,*skills::effects::cry_frustration(12),20,1100));
   check(f.w.npc_combat_snapshot(f.source)->health->current<hp);
   f.w.tick(token.completes);check(f.w.resources(1)->health==1000&&!f.cast());
  }
 }
 std::cout<<"NPC155 retains non-spell immunity to Power Spike and skill interruptibility to Cry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
