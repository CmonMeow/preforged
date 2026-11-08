#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Freezing Gust World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope caster,target;
    explicit Fixture(bool npc,bool armor=true){
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
            if(armor){d.cold_armor=60;d.cold_armor_evidence="test";}
            n.combat_definition=d;w.configure_npcs({n});
        }
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);
        if(!npc){
            w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(2,{1000,1000,100,100},100);
            check(w.elemental_armor(*w.effect_scope(2),{},armor?std::optional<int>{60}:std::nullopt));
        }
        caster=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
    }
    double health()const{return target.actor==100?w.npc_combat_snapshot(target)->health->current:
        double(w.resources(2)->health);}
    bool slowed(std::uint64_t at)const{return target.actor==100?
        bool(w.npc_combat_snapshot(target)->effects&0x800):!w.status_snapshot(target,at)->empty();}
    bool cast(unsigned rank=12,std::uint64_t at=1000,std::vector<world::Packet>* out=nullptr){
        return w.freezing_gust(caster,target,*skills::effects::movement_hex(1382,rank),20,at,out);
    }
};
int main()try{
    using namespace skills;using namespace effects;
    for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u}){
        Fixture f(npc);std::vector<world::Packet> out;
        check(f.cast(rank,1000,&out));check(f.health()==1000&&f.slowed(1000));
        check(std::find(out.begin(),out.end(),*particles::targeted_body_2022(f.target.actor,1,1382))!=out.end());
        if(rank==12){check(f.cast(rank,1500));check(std::abs(f.health()-(1000-freezing_gust(rank)->damage))<1.0);}
        f.w.tick(1000+freezing_gust(rank)->snare_ms);check(!f.slowed(1000+freezing_gust(rank)->snare_ms));
    }
    for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u}){
        Fixture f(npc);check(f.w.movement_hex(f.caster,f.target,*movement_hex(210,12),500));
        std::vector<world::Packet> out;check(f.cast(rank,1000,&out));
        check(std::abs(f.health()-(1000-freezing_gust(rank)->damage))<1.0);
        check(f.slowed(1000));
        if(!npc){const auto status=*f.w.status_snapshot(f.target,1000);
            check(status.size()==1&&status.front().skill==210);}
        check(std::find(out.begin(),out.end(),*particles::targeted_body_2022(f.target.actor,1,1382))!=out.end());
    }
    for(bool npc:{false,true}){
        Fixture f(npc,false);check(f.w.movement_hex(f.caster,f.target,*movement_hex(210,12),500));
        check(!f.cast()&&f.health()==1000);
    }
    {
        Fixture f(false);auto forged=*movement_hex(1382,12);forged.conditional_damage+=1;
        check(!f.w.freezing_gust(f.caster,f.target,forged,20,1000));
        const auto stale=f.target;f.w.leave(2);
        check(!f.w.freezing_gust(f.caster,stale,*movement_hex(1382,12),20,1000));
    }
    for(bool hexed:{false,true}){
        Fixture f(false);if(hexed)check(f.w.movement_hex(f.caster,f.target,*movement_hex(210,12),500));
        casting::Hooks h;unsigned pays=0,commits=0;
        h.resources=[&](unsigned id){return f.w.resources(id);};
        h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
        h.consume_energy_at=[&](unsigned cost,auto at){++pays;return f.w.consume_energy(1,cost,f.caster,at);};
        h.target_scope=[&](unsigned id){return f.w.effect_scope(id);};
        h.projectile_target_valid=[&](unsigned id,float range){return id==2&&range==1248;};
        h.target_valid=[&](unsigned id,float,bool self){return self&&id==1;};
        h.movement_hex=[&](auto target,const auto& value,auto at)->casting::EffectCompletion{
            ++commits;std::vector<world::Packet> out;
            const bool ok=f.w.freezing_gust(f.caster,target,value,20,at,&out);
            return {ok,std::move(out)};
        };
        Slots bar{};bar[0]={1382,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1382]=1;
        attributes::Ranks ranks{};ranks[11]=12;casting::Runtime cast(1);
        const auto reason=cast.start(1000,{1382,0,2},bar,policy,ranks,true,h);
        if(reason!=casting::Reason::accepted)throw std::runtime_error("Freezing Gust cast start reason "+std::to_string(int(reason)));
        check(pays==1&&f.w.resources(1)->energy==90);
        cast.update(1999,h);check(!commits);cast.update(2000,h);check(commits==1);
        check(hexed?f.health()<1000:f.health()==1000);
        const auto events=f.box->drain();
        const auto body=*particles::targeted_body_2022(2,1,1382);
        check(std::find(events.begin(),events.end(),casting::terminal(58,1))<
              std::find(events.begin(),events.end(),body));
    }
    std::cout<<"Freezing Gust World and paid cast passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
