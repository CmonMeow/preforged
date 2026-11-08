#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("NPC shared aggro World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,first,second;bool elemental;
    Fixture(bool use_elemental):elemental(use_elemental){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
        // No available Energy isolates encounter selection from casts/aftercast.
        d.spawn_energy=combat::npcs::Energy{0,30};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
        join(1,100,100);join(2,500,100);source=*w.effect_scope(100);first=*w.effect_scope(1);second=*w.effect_scope(2);
        combat::ai::FlarePolicy p;p.source=source;p.level=20;p.fire_magic=12;p.speed=1;p.acquire=1000;p.follow=2000;p.leash=3000;
        p.flight={100,32};p.evidence="synthetic shared-aggro fixture, authored radii and zero current Energy";
        if(elemental)check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,12),p.evidence}},4,1000));
        else check(w.configure_npc_flare_ai(p,1000));
    }
    void join(unsigned id,float x,std::uint64_t now){
        w.join(id,"Target",x,0,std::make_shared<world::Outbox>(),0,{}, {},now,id);
        w.set_vitals(id,{1000,1000,0,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));
    }
    skills::effects::Scope selected(){return elemental?w.npc_elemental_ai_snapshot(source)->target:w.npc_flare_ai_snapshot(source)->policy.target;}
    bool engaged(){return elemental?w.npc_elemental_ai_snapshot(source)->engaged:w.npc_flare_ai_snapshot(source)->engaged;}
    bool returning(){return elemental?w.npc_elemental_ai_snapshot(source)->returning:w.npc_flare_ai_snapshot(source)->returning;}
    void move_second(){
        // move() starts real player travel rather than teleporting. Let B
        // reach1500 while A remains selected; the slow authored NPC permits
        // A to subsequently create a gap larger than follow range.
        // The authoritative player speed is288, so1000 units finish at4573.
        w.move(2,1500,0,1100);w.tick(4600);
        const auto x=w.position(2,4600).first;const auto target=selected();
        if(std::abs(x-1500)>=.01f||target!=first)throw std::runtime_error(
            std::string(elemental?"elemental":"Flare")+" move_second: B x="+std::to_string(x)+
            " expected1500; selected="+std::to_string(target.actor)+":"+std::to_string(target.generation)+
            " expected="+std::to_string(first.actor)+":"+std::to_string(first.generation));
    }
    void lose_first(){move_second();w.move(1,2500,0,4600);w.tick(13000);
        check(std::abs(w.position(1,13000).first-2500)<.01f);w.tick(13100);}
};
int main()try{
    for(bool elemental:{false,true}){
        {Fixture f(elemental);
            combat::ai::ordinary::Reviewed profile{10,10,0,5,true,combat::ai::ordinary::Delivery::ranged,
                500,1500,1000,{100,32},"authored shared-aggro ordinary fallback fixture"};
            check(f.w.configure_npc_ordinary_ai(f.source,profile,1000));f.w.tick(1000);
            check(f.w.npc_ordinary_pending(f.source).has_value());
            f.lose_first();
            check(f.engaged()&&!f.returning()&&f.selected()==f.second);}
        {Fixture f(elemental);f.w.tick(1000);check(f.engaged()&&f.selected()==f.first);
            f.lose_first();check(f.engaged()&&!f.returning()&&f.selected()==f.second);
            check(f.w.npc_energy_snapshot(f.source)->current==0);}
        {Fixture f(elemental);f.w.tick(1000);
            // B is remembered while A remains selected, rather than only
            // being discovered at the instant A disappears.
            f.move_second();check(f.selected()==f.first);
            f.w.move(1,2500,0,4600);f.w.tick(13000);
            check(std::abs(f.w.position(1,13000).first-2500)<.01f);
            check(f.engaged()&&f.selected()==f.second);}
        {Fixture f(elemental);f.w.tick(1000);f.lose_first();
            f.join(3,1300,13200);check(f.w.kill(f.second,13300));f.w.tick(13300);
            // A new foe outside acquire does not join the remembered encounter.
            check(!f.engaged()&&f.returning());}
        {Fixture f(elemental);f.w.tick(1000);f.move_second();
            f.w.leave(2);f.join(2,1500,4600);const auto replacement=*f.w.effect_scope(2);check(replacement!=f.second);
            check(f.w.kill(f.first,4700));f.w.tick(4700);
            check(!f.engaged()&&f.returning()&&f.selected()!=replacement);}
        {Fixture f(elemental);f.w.tick(1000);f.move_second();
            // move_second already consumed the4600 scheduler tick. The death
            // must be followed by a new due tick, rather than reusing4600.
            check(f.w.kill(f.first,4700));f.w.tick(4700);
            if(!f.engaged()||f.returning()||f.selected()!=f.second)throw std::runtime_error(
                std::string(elemental?"elemental":"Flare")+" death switch at4700: engaged="+std::to_string(f.engaged())+
                " returning="+std::to_string(f.returning())+" selected="+std::to_string(f.selected().actor)+
                " expected="+std::to_string(f.second.actor));}
    }
    std::cout<<"NPC Flare/elemental remembered encounter selection passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
