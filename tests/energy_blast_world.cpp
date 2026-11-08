#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Energy Blast World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope caster,target;
    explicit Fixture(bool npc,unsigned energy=20){
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{1000,1000};d.damageable=true;
            n.combat_definition=d;w.configure_npcs({n});
        }
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,energy,100},100);
        if(!npc){
            w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(2,{1000,1000,100,100},100);
        }
        caster=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
    }
    double health()const{return target.actor==100?w.npc_combat_snapshot(target)->health->current:
        double(w.resources(2)->health);}
    bool cast(unsigned rank=12,std::uint64_t at=1000,std::vector<world::Packet>* out=nullptr){
        return w.energy_blast(caster,target,*skills::effects::energy_blast(rank),20,at,out);
    }
};
int main()try{
    using namespace skills;using namespace effects;
    for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u}){
        Fixture f(npc,20);std::vector<world::Packet> out;
        check(f.cast(rank,1000,&out));
        check(f.health()==1000-20*energy_blast(rank)->damage_per_energy);
        check(std::find(out.begin(),out.end(),*particles::targeted_body_2022(f.target.actor,1,2193))!=out.end());
    }
    {Fixture f(false,100);check(f.cast()&&f.health()==870);}
    {
        Fixture f(false,0);check(f.cast()&&f.health()==1000);
        auto forged=*energy_blast(12);++forged.maximum_damage;
        check(!f.w.energy_blast(f.caster,f.target,forged,20,1500));
        const auto stale=f.target;f.w.leave(2);
        check(!f.w.energy_blast(f.caster,stale,*energy_blast(12),20,1500));
    }
    {
        Fixture f(false,20);casting::Hooks h;unsigned pays=0,commits=0;
        h.resources=[&](unsigned id){return f.w.resources(id);};
        h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
        h.consume_energy_at=[&](unsigned cost,auto at){++pays;return f.w.consume_energy(1,cost,f.caster,at);};
        h.target_scope=[&](unsigned id){return f.w.effect_scope(id);};
        h.projectile_target_valid=[&](unsigned id,float range){return id==2&&range==1248;};
        h.energy_blast=[&](auto target,const auto& value,auto at)->casting::EffectCompletion{
            ++commits;std::vector<world::Packet> out;
            const bool ok=f.w.energy_blast(f.caster,target,value,20,at,&out);
            return {ok,std::move(out)};
        };
        Slots bar{};bar[0]={2193,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[2193]=1;
        attributes::Ranks ranks{};ranks[12]=12;casting::Runtime cast(1);
        check(cast.start(1000,{2193,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(pays==1&&f.w.resources(1)->energy==10);
        cast.update(2499,h);check(!commits);cast.update(2500,h);check(commits==1);
        // The World advances natural Energy regeneration to completion time;
        // damage must use that settled, post-payment value.
        check(f.health()==1000-2*f.w.resources(1)->energy);
        const auto events=f.box->drain();
        check(std::find(events.begin(),events.end(),casting::terminal(58,1))<
              std::find(events.begin(),events.end(),*particles::targeted_body_2022(2,1,2193)));
    }
    std::cout<<"Energy Blast World and paid cast passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
