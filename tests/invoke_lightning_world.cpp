#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Invoke Lightning line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;
    skills::effects::Scope caster;
    Fixture(){
        for(auto [id,x]:{std::pair{100u,350.f},std::pair{101u,950.f}}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;
            n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;
            d.binding.actor=d.binding.definition=id;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={1000,1000};d.damageable=true;
            d.lightning_armor=80;d.lightning_armor_evidence="fixture";
            n.combat_definition=d;npcs.push_back(n);
        }
        w.configure_npcs(npcs);
        for(auto [id,x]:{std::pair{1u,0.f},std::pair{2u,100.f},
                          std::pair{3u,600.f},std::pair{4u,850.f}}){
            w.join(id,"Player",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{1000,1000,100,100},100);
            check(w.lightning_armor(*w.effect_scope(id),80));
        }
        caster=*w.effect_scope(1);
    }
    std::vector<world::Instance::Npc> npcs;
    skills::effects::Scope scope(unsigned id){return *w.effect_scope(id);}
    double health(unsigned id){return id>=100?w.npc_combat_snapshot(scope(id))->health->current:w.resources(id)->health;}
};
int main()try{
    using namespace skills;
    const auto plan=effects::lightning(1664,12);
    check(plan&&plan->damage==74&&plan->secondary_damage==74&&
          plan->max_secondary==2&&plan->penetration==25);
    auto forged=*plan;forged.max_secondary=3;check(!effects::valid_lightning(forged));
    {
        Fixture f;std::vector<world::Packet> visuals;
        check(f.w.lightning(f.caster,f.scope(2),*plan,20,1000,&visuals));
        check(f.health(2)<1000&&f.health(100)<1000&&f.health(3)<1000);
        check(f.health(4)==1000&&f.health(101)==1000);
        // The third foe is outside the primary's 322-unit area but within
        // 322 units of the second foe: this must be a chain, not a fanout.
        for(unsigned id:{2u,100u,3u})
            check(std::count(visuals.begin(),visuals.end(),
                *particles::targeted_body_2022(id,1,1664))==1);
        auto stale=f.scope(100);++stale.generation;
        check(!f.w.lightning(f.caster,stale,*plan,20,1200));
    }
    {
        Fixture f;std::vector<world::Packet> visuals;
        check(f.w.lightning(f.caster,f.scope(100),*plan,20,1000,&visuals));
        check(f.health(100)<1000);
        check(std::count(visuals.begin(),visuals.end(),
            *particles::targeted_body_2022(100,1,1664))==1);
    }
    {
        Fixture f;casting::Hooks h;std::vector<world::Packet> packets;
        h.resources=[&](auto id){return f.w.resources(id);};
        h.target_scope=[&](auto id){return f.w.effect_scope(id);};
        h.projectile_target_valid=[&](auto id,float range){return id==2&&range==1248.f;};
        h.consume_energy_at=[&](auto cost,auto at){check(cost==10);return f.w.consume_energy(1,cost,f.caster,at);};
        h.consume_overcast_energy=[&](auto skill,auto amount,auto event,auto at){
            check(skill==1664&&amount==10&&event);return f.w.activate_overcast_energy(f.caster,skill,amount,event,at);};
        h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        h.lightning=[&](auto selected,const auto& quote,auto at){
            std::vector<world::Packet> visuals;
            const bool ok=f.w.lightning(f.caster,selected,quote,20,at,&visuals);
            return casting::EffectCompletion{ok,std::move(visuals)};};
        Slots bar{};bar[0]={1664,0};Policy policy;
        policy.primary=6;policy.campaigns=255;policy.learned[1664]=1;
        attributes::Ranks ranks{};ranks[8]=12;
        casting::Runtime cast(1);
        check(cast.start(500,{1664,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(f.w.resources(1)->energy==90&&f.health(2)==1000);
        cast.update(1500,h);
        check(f.health(2)<1000&&f.health(100)<1000&&f.health(3)<1000);
        const auto terminal=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
        const auto effect=std::find(packets.begin(),packets.end(),
            *particles::targeted_body_2022(2,1,1664));
        check(terminal!=packets.end()&&terminal<effect&&effect!=packets.end());
    }
    std::cout<<"PASS Invoke Lightning paid cast, 25% penetration, mixed chain and target effects\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
