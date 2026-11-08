#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC ordinary pursuit World line "+std::to_string(at.line()));
}
std::uint32_t word(const world::Packet& p,std::size_t i){return std::uint32_t(p[i])|std::uint32_t(p[i+1])<<8|
    std::uint32_t(p[i+2])<<16|std::uint32_t(p[i+3])<<24;}
struct Fixture {
    world::Instance w;skills::effects::Scope source,target;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    explicit Fixture(unsigned kind){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=160;d.binding.actor=d.binding.definition=100;d.binding.level=1;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{80,80};d.damageable=true;
        if(kind==1){d.spawn_energy=combat::npcs::Energy{0,30};d.energy_pips=0;}
        n.combat_definition=d;if(kind!=2)w.configure_npcs({n});
        w.join(1,"Moving target",100,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,0,100},100);
        target=*w.effect_scope(1);check(w.physical_armor(target,0));check(w.elemental_armor(target,0,0));
        if(kind==2){
            check(w.spawn_kill_encounter(target,75,1,{n},[]{return true;}));source=*w.effect_scope(60000);
            check(w.arm_grawl_melee(target,1,1000));
            check(w.damage_npc(target,source,{combat::Kind::armor_ignoring,1,20,194},1248,1000).reason==combat::Reason::accepted);
        }else{
            source=*w.effect_scope(100);combat::ai::FlarePolicy p;p.source=source;p.level=1;p.speed=220;
            p.acquire=1000;p.follow=1500;p.leash=2000;p.flight={1000,32};p.evidence="synthetic authored cooldown-pursuit fixture";
            combat::ai::ordinary::Reviewed attack{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence};
            if(kind==1){check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,0),p.evidence}},4,1000));
                check(w.configure_npc_ordinary_ai(source,attack,1000));}
            else check(w.configure_npc_hostile_ordinary(p,attack,1000));
        }
        box->drain();
    }
    std::optional<float> moved_x(){std::optional<float> x;
        for(const auto& p:box->drain())if(p.size()==16&&p[0]==44&&p[1]==0&&word(p,2)==source.actor)x=std::bit_cast<float>(word(p,6));return x;}
};
int main()try{
    for(unsigned kind:{0u,1u,2u}){
      {
        Fixture f(kind);f.w.tick(1000);check(f.w.npc_ordinary_pending(f.source).has_value());
        f.w.tick(1750);check(!f.w.npc_ordinary_pending(f.source));check(f.w.resources(1)->health<1000);
        const auto health=f.w.resources(1)->health;f.box->drain();
        // Observe the new pursuit goal before allowing time to travel it.
        f.w.move(1,1000,0,1750);f.w.tick(1850);f.w.tick(2250);
        const auto player_x=f.w.position(1,2250).first;const auto moved=f.moved_x();
        if(!(player_x>128)||!moved||*moved<=0)
            std::cerr<<"pursuit kind="<<kind<<" player_x="<<player_x<<" npc_move_x="
                     <<(moved?std::to_string(*moved):"absent")<<'\n';
        check(player_x>128);
        check(moved&&*moved>0);
        // Moving during recovery must not authorize an early second swing.
        check(!f.w.npc_ordinary_pending(f.source));check(f.w.resources(1)->health==health);
        f.w.tick(2400);check(f.moved_x().has_value());check(!f.w.npc_ordinary_pending(f.source));
        check(f.w.kill(f.target,2500));
        for(unsigned at=2500;at<=5000;at+=100)f.w.tick(at);
        check(!f.w.npc_ordinary_pending(f.source));
        if(kind==1){const auto state=f.w.npc_elemental_ai_snapshot(f.source);check(state&&!state->engaged&&!state->returning);}
        else{const auto state=f.w.npc_flare_ai_snapshot(f.source);check(state&&!state->engaged&&!state->returning);}
      }
      {
        Fixture f(kind);f.w.tick(1000);f.box->drain();f.w.move(1,1000,0,1100);f.w.tick(1500);
        check(f.w.npc_ordinary_pending(f.source).has_value());check(!f.moved_x());
        f.w.tick(1750);check(!f.w.npc_ordinary_pending(f.source));check(f.w.resources(1)->health==1000);
        // Recovery first observes the pursuit goal; travel uses the next interval.
        f.box->drain();f.w.tick(1850);check(!f.moved_x());
        f.w.tick(1950);check(f.moved_x().has_value());check(!f.w.npc_ordinary_pending(f.source));
      }
    }
    std::cout<<"NPC ordinary pursuit during attack cooldown and target-death return passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
