#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
using fw::gw::world::Packet;
void check(bool b,std::source_location at=std::source_location::current()){
    if(!b)throw std::runtime_error("Slippery Ground World line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned mode=0;mode<3;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        for(unsigned id=1;id<=5;++id){
            const float x=id==1?0:id==2?100:id==3?266:id==4?266.01f:100;
            w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
            w.set_vitals(id,{500,500,100,100},100);
        }
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.blindness(caster,*w.effect_scope(3),*blindness(220,12),100));
        check(w.blindness(caster,*w.effect_scope(4),*blindness(220,12),100));
        box->drain();
        auto addressed=target;if(mode==1)++addressed.generation;
        auto plan=*slippery_ground(5);if(mode==2)++plan.knockdown_ms;
        std::vector<Packet> deferred;
        check(w.slippery_ground(caster,addressed,plan,101,&deferred)==(mode==0));
        check(!w.knocked_down(target,101));
        check(w.knocked_down(*w.effect_scope(3),101)==(mode==0));
        check(!w.knocked_down(*w.effect_scope(4),101)&&!w.knocked_down(*w.effect_scope(5),101));
        if(!mode){
            check(w.knocked_down(*w.effect_scope(3),2100)&&!w.knocked_down(*w.effect_scope(3),2101));
            check(std::count(deferred.begin(),deferred.end(),combat::knockdown_2022(3,2))==1);
            check(std::count(deferred.begin(),deferred.end(),*skills::particles::target_body_2022(2,2191))==1);
        }else check(deferred.empty());
    }
    for(unsigned mode=0;mode<3;++mode){
        world::Instance w;std::vector<world::Instance::Npc> population;
        for(unsigned id:{100u,101u}){
            world::Instance::Npc npc;npc.id=npc.definition.id=id;npc.definition.visual_adjustment={0,0,0,100};npc.x=100;npc.y=0;
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
            d.spawn_health=combat::Health{500,500};d.relation=combat::npcs::Relation::hostile_to_players;
            d.damageable=true;d.conditions_allowed=true;d.knockdown_allowed=true;d.knockdown_evidence="test";
            if(id==101&&mode==1)d.knockdown_allowed=false;
            if(id==101&&mode==2)d.knockdown_allowed.reset();
            npc.combat_definition=d;population.push_back(npc);
        }
        w.configure_npcs(population);auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
        const auto caster=*w.effect_scope(1),first=*w.effect_scope(100),second=*w.effect_scope(101);
        check(w.blindness(caster,first,*blindness(220,12),100));
        check(w.blindness(caster,second,*blindness(220,12),100));box->drain();
        std::vector<Packet> deferred;
        check(w.slippery_ground(caster,first,*slippery_ground(5),101,&deferred)==(mode!=2));
        check(w.knocked_down(first,101)==(mode!=2));
        check(w.knocked_down(second,101)==(mode==0));
        if(mode==2)check(deferred.empty());
    }
    std::cout<<"Slippery Ground World passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
