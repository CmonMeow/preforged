#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC disengagement route line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,first,second;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    bool elemental;float x=0;unsigned stops=0;
    Fixture(bool use_elemental,bool unknown,float first_x=1000):elemental(use_elemental){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,1};
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,100};d.damageable=true;
        if(elemental){d.spawn_energy=combat::npcs::Energy{0,30};d.energy_pips=0;}
        n.combat_definition=d;w.configure_npcs({n});
        join(1,first_x);first=*w.effect_scope(1);
        if(unknown){join(2,1100);second=*w.effect_scope(2);}
        source=*w.effect_scope(100);combat::ai::FlarePolicy p;p.source=source;p.level=1;p.speed=220;
        p.acquire=1200;p.follow=1500;p.leash=2000;p.flight={1000,32};
        p.evidence="synthetic retained-chase retirement; authored geometry and timing";
        combat::ai::ordinary::Reviewed attack{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence};
        if(elemental){check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,0),p.evidence}},4,1000));
            check(w.configure_npc_ordinary_ai(source,attack,1000));}
        else check(w.configure_npc_hostile_ordinary(p,attack,1000));
        box->drain();
    }
    void join(unsigned id,float at){w.join(id,"Target",at,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
        w.set_vitals(id,{1000,1000,0,100},100);check(w.physical_armor(*w.effect_scope(id),0));}
    void tick(unsigned at){w.tick(at);for(const auto& p:box->drain()){
        if(p.size()<6)continue;
        const auto word=[&](unsigned i){return unsigned(p[i])|unsigned(p[i+1])<<8|unsigned(p[i+2])<<16|unsigned(p[i+3])<<24;};
        if(word(2)!=source.actor)continue;
        if(p.size()==6&&p[0]==40&&!p[1])++stops;
        if(p.size()==16&&p[0]==44&&!p[1])x=std::bit_cast<float>(word(6));
    }}
    bool returning(){return elemental?w.npc_elemental_ai_snapshot(source)->returning:w.npc_flare_ai_snapshot(source)->returning;}
    bool engaged(){return elemental?w.npc_elemental_ai_snapshot(source)->engaged:w.npc_flare_ai_snapshot(source)->engaged;}
};
int main()try{
    for(bool elemental:{false,true}){
        Fixture f(elemental,false,2500);
        check(f.w.configure_npc_wander(f.source,{{{80,0},{0,80}},1200,6000,"synthetic idle-preservation control"},1000));
        f.tick(1000);f.tick(2200);f.tick(2300);check(std::abs(f.x-22)<.01&&f.stops==0);
        f.tick(2400);check(std::abs(f.x-44)<.01&&f.stops==0&&!f.engaged());
    }
    for(bool elemental:{false,true})for(bool unknown:{false,true}){
        Fixture f(elemental,unknown);f.tick(1000);check(f.engaged());
        if(unknown)check(f.w.settle_resources(f.second,4000));
        check(f.w.kill(f.first,1100));f.tick(1100);
        // The previous interval still belongs to the old chase. Retirement
        // is forward-only and must not erase legitimate travel before death.
        check(std::abs(f.x-22)<.01);const auto at_loss=f.x;
        check(!f.engaged()&&f.returning()==!unknown);check(f.stops==1);
        f.tick(2100);
        // Previously the retained eastward route advanced another 220 units
        // after the driver had already rejected its target at 1100.
        check(std::abs(f.x-at_loss)<.01);check(f.stops==1);
        if(unknown){
            f.tick(3100);check(std::abs(f.x-at_loss)<.01&&!f.returning());check(f.stops==1);
            f.tick(4000);check(f.engaged());check(std::abs(f.x-at_loss)<.01);
            f.tick(4100);check(f.x>at_loss);
        }else{
            f.tick(2300);check(f.x<at_loss&&!f.returning());
        }
        check(!f.w.npc_ordinary_pending(f.source));
    }
    std::cout<<"Hostile ordinary/elemental stale chase retires once on lost/unknown targets; prior travel and resumed pursuit preserved\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
