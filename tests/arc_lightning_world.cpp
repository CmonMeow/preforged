#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,const char* why="",std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Arc Lightning World line "+std::to_string(at.line())+" "+why);
}
struct Fixture {
    world::Instance w;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope caster;
    Fixture(){
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=115;n.plane=0;
        n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;
        d.spawn_health={1000,1000};d.damageable=true;d.lightning_armor=80;d.lightning_armor_evidence="test";
        n.combat_definition=d;w.configure_npcs({n});
        for(unsigned id=1;id<=6;++id){
            const float x=id==1?0:id==2?100:id==3?110:id==4?120:id==5?130:400;
            w.join(id,"Player",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{1000,1000,100,100},100);
            check(w.lightning_armor(*w.effect_scope(id),80));
        }
        caster=*w.effect_scope(1);box->drain();
    }
    skills::effects::Scope scope(unsigned id){return *w.effect_scope(id);}
    double health(unsigned id){return id==100?w.npc_combat_snapshot(scope(id))->health->current:w.resources(id)->health;}
    void overcast(){check(w.activate_overcast_energy(caster,866,10,1,100));}
};
int main()try{
    using namespace skills;
    const auto plan=effects::lightning(842,12);check(plan&&plan->damage==33&&plan->secondary_damage==71&&plan->max_secondary==2);
    auto forged=*plan;forged.secondary_damage=100;check(!effects::valid_lightning(forged));
    {
        Fixture f;std::vector<world::Packet> visuals;
        check(f.w.lightning(f.caster,f.scope(2),*plan,20,1000,&visuals),"primary without Overcast");
        check(f.health(2)<1000&&f.health(3)==1000&&f.health(100)==1000&&f.health(4)==1000,
              "unmodified cast hits primary only");
        check(std::count(visuals.begin(),visuals.end(),*particles::targeted_body_2022(2,1,842))==1);
        check(std::count(visuals.begin(),visuals.end(),*particles::arc_lightning_chain_2022(2,1))==1,
              "primary resource989 appears without Overcast");
        check(visuals.size()>=2&&visuals[0]==*particles::targeted_body_2022(2,1,842)&&
              visuals[1]==*particles::arc_lightning_chain_2022(2,1),"primary visual pair order");
    }
    {
        Fixture f;f.overcast();std::vector<world::Packet> visuals;
        check(f.w.lightning(f.caster,f.scope(2),*plan,20,1000,&visuals),"Overcast fanout");
        check(f.health(2)<1000&&f.health(3)<f.health(2)&&f.health(100)<f.health(2),
              "stronger splash reaches nearest player and NPC");
        check(f.health(4)==1000&&f.health(5)==1000&&f.health(6)==1000,
              "two-secondary cap and out-of-range exclusion");
        check(std::count(visuals.begin(),visuals.end(),*particles::arc_lightning_chain_2022(2,1))==1,
              "captured resource989 published once");
        check(std::count(visuals.begin(),visuals.end(),*particles::arc_lightning_chain_2022(3,1))==1&&
              std::count(visuals.begin(),visuals.end(),*particles::arc_lightning_chain_2022(100,1))==1&&
              std::find(visuals.begin(),visuals.end(),*particles::targeted_body_2022(3,1,842))==visuals.end(),
              "secondary foes receive 989, not primary-only 988");
    }
    {
        Fixture f;f.overcast();std::vector<world::Packet> visuals;
        check(f.w.lightning(f.caster,f.scope(100),*plan,20,1000,&visuals),"NPC primary");
        check(f.health(100)<1000&&f.health(3)<1000&&f.health(4)<1000&&f.health(2)==1000,
              "NPC primary and two nearest mixed recipients");
        check(visuals.size()>=2&&visuals[0]==*particles::targeted_body_2022(100,1,842)&&
              visuals[1]==*particles::arc_lightning_chain_2022(100,1),"NPC primary visual pair precedes splash");
        auto stale=f.scope(100);++stale.generation;
        check(!f.w.lightning(f.caster,stale,*plan,20,1100),"stale NPC rejected");
    }
    {
        Fixture f;f.overcast();
        casting::Hooks h;std::vector<world::Packet> events;
        h.resources=[&](unsigned id){return f.w.resources(id);};
        h.target_scope=[&](unsigned id){return f.w.effect_scope(id);};
        h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248.f;};
        h.consume_energy_at=[&](unsigned cost,auto now){return f.w.consume_energy(1,cost,f.caster,now);};
        h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());};
        h.lightning=[&](auto target,const auto& quote,auto now)->casting::EffectCompletion{
            std::vector<world::Packet> visuals;const bool ok=f.w.lightning(f.caster,target,quote,20,now,&visuals);
            return {ok,std::move(visuals)};
        };
        Slots slots{};slots[0]={842,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[842]=1;
        attributes::Ranks ranks{};ranks[8]=12;
        casting::Runtime cast(1);
        check(cast.start(1000,{842,0,2},slots,policy,ranks,true,h)==casting::Reason::accepted);
        cast.update(2499,h);check(f.health(2)==1000,"1.5-second cast not early");
        cast.update(2500,h);check(f.health(2)<1000&&
              std::find(events.begin(),events.end(),*particles::arc_lightning_chain_2022(2,1))!=events.end(),
              "normal cast reaches Overcast World and chain body");
    }
    std::cout<<"Arc Lightning World passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
