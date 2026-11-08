#include "gw_life_steal_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Life-steal lifecycle line "+std::to_string(at.line())+" "+context);
}
world::Instance::Npc body(unsigned id,unsigned health,combat::npcs::Relation relation){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=100;n.definition.level=20;
    n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding={146,id,id,0,0,0,0,20};
    d.relation=relation;d.spawn_health=combat::Health{double(health),1000};
    d.damageable=d.conditions_allowed=true;d.fleshy=true;
    d.fleshy_evidence="synthetic lifecycle condition recipient";n.combat_definition=d;return n;
}
struct Fixture {
    world::Instance w;effects::Scope source,target,guardian;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(unsigned health=500,float guardian_x=100,float enemy_x=100){
        auto guard=body(100,1000,combat::npcs::Relation::neutral);guard.x=guardian_x;
        w.configure_npcs({guard});
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.set_vitals(1,{100,1000,100,100},100);
        source=*w.effect_scope(1);guardian=*w.effect_scope(100);
        spawn(1,health,enemy_x);target=*w.effect_scope(60000);box->drain();
    }
    void spawn(unsigned cycle,unsigned health=500,float enemy_x=100){
        check(w.begin_wave_objective(source,guardian,73,cycle,3,1));
        auto enemy=body(101,health,combat::npcs::Relation::hostile_to_players);enemy.x=enemy_x;
        check(w.spawn_quest_wave(source,73,cycle,{enemy},[]{return true;}));
    }
};
int main()try{
    unsigned retired=0,deadline=0,moving=0;
    for(unsigned skill:{153u,156u,902u,1076u,1077u,2189u})for(unsigned mode=0;mode<4;++mode){
        context="retirement skill="+std::to_string(skill)+" mode="+std::to_string(mode);
        Fixture f;casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0;
        auto native=casting::make_life_steal_completion(f.w,[&]{return f.w.effect_scope(1);});
        h.resources=[&](auto id){return f.w.resources(id);};
        h.events=[&](const auto& packets){f.box->enqueue(packets);};
        h.target_scope=[&](auto id){return f.w.effect_scope(id);};
        h.projectile_target_valid=[](auto,float){return true;};
        h.target_valid=[](auto,float,bool){return true;};
        h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
        h.complete_life_steal=[&](auto target,const auto& plan,auto at){++calls;return native(target,plan,at);};
        Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;
        Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
        check(runtime.start(100,{skill,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
        std::optional<effects::Scope> replacement;
        if(mode==1||mode==2){
            check(f.w.clear_kill_objective(f.source,73,1,true));
            check(!f.w.effect_scope(f.target.actor)&&!f.w.npc_combat_snapshot(f.target));
            if(mode==2){f.spawn(2);replacement=f.w.effect_scope(60001);check(bool(replacement));}
        }
        if(mode==3){
            f.w.leave(1);check(!f.w.effect_scope(f.target.actor));
            f.w.join(1,"Replacement owner",0,0,f.box,0,{}, {},200,1);
            f.w.set_vitals(1,{100,1000,100,100},100);
        }
        const auto at=100+std::uint64_t(std::llround(find_skill(skill)->activation*1000));
        f.box->drain();runtime.update(at,h);runtime.update(at+100,h);
        check(calls==unsigned(mode==0));
        check(f.w.resources(1)->health==100+(mode==0?effects::life_steal(skill,0)->amount:0));
        if(replacement)check(f.w.npc_combat_snapshot(*replacement)->health->current==500);
        if(mode){
            const auto stale=native(f.target,*effects::life_steal(skill,0),at);
            check(!stale.accepted&&stale.visuals.empty());
        }
        ++retired;
    }
    for(unsigned skill:{153u,156u,902u,1076u,1077u,2189u})for(bool dense:{false,true}){
        context="due condition skill="+std::to_string(skill)+" dense="+std::to_string(dense);
        Fixture f(4);check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::poison,100,10000));
        if(dense)for(unsigned at=200;at<1100;at+=100)f.w.tick(at);
        f.box->drain();auto complete=casting::make_life_steal_completion(f.w,[&]{return std::optional{f.source};});
        const auto result=complete(f.target,*effects::life_steal(skill,0),1100);
        check(!result.accepted&&result.visuals.empty());
        check(f.w.resources(1)->health==100&&f.w.npc_combat_snapshot(f.target)->health->current==0);
        const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==1);
        check(f.w.take_npc_deaths(f.source).empty());++deadline;
    }
    for(unsigned skill:{153u,156u,902u,1076u,1077u,2189u})for(bool dense:{false,true})for(bool entering:{false,true}){
        context="moving recipient skill="+std::to_string(skill)+" dense="+std::to_string(dense)+" entering="+std::to_string(entering);
        const float goal=entering?100.f:2000.f;float observed=entering?2000.f:100.f;
        Fixture f(500,goal,observed);check(f.w.chase_quest_guardian(f.target,440,100));
        const auto observe=[&](const std::vector<world::Packet>& packets){for(const auto& p:packets){
            if(p.size()!=16||p[0]!=44||p[1])continue;
            const auto word=[&](unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);};
            if(word(2)==f.target.actor)observed=std::bit_cast<float>(word(6));
        }};
        if(dense)for(unsigned at=200;at<5100;at+=100)f.w.tick(at);
        observe(f.box->drain());auto complete=casting::make_life_steal_completion(f.w,[&]{return std::optional{f.source};});
        const auto result=complete(f.target,*effects::life_steal(skill,0),5100);
        observe(f.box->drain());check(std::abs(observed-goal)<.01f);
        check(result.accepted==entering);if(!entering)check(result.visuals.empty());
        const auto amount=entering?effects::life_steal(skill,0)->amount:0;
        check(f.w.resources(1)->health==100+amount&&f.w.npc_combat_snapshot(f.target)->health->current==500-amount);
        check(f.w.take_npc_deaths(f.source).empty());++moving;
    }
    std::cout<<"PASS "<<retired<<" real quest retirement/admission, "<<deadline
        <<" due-condition deaths and "<<moving<<" moving-recipient range cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
