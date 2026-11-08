#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("To the Limit World line "+std::to_string(at.line()));
}
world::Instance::Npc make_npc(unsigned id,float x,bool hostile=true,unsigned health=1000){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
    d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::protected_ally;
    d.spawn_health=combat::Health{double(health),1000};d.damageable=hostile;d.conditions_allowed=hostile;
    n.combat_definition=d;return n;
}
int main()try{
    for(unsigned rank:{0u,12u,15u})for(unsigned mode=0;mode<5;++mode){
        world::Instance w;auto owner_box=std::make_shared<world::Outbox>(),peer_box=std::make_shared<world::Outbox>();
        // Only two of these NPCs qualify: boundary and nearby hostiles.
        w.configure_npcs({make_npc(100,1012),make_npc(101,1012.1f),make_npc(102,20,false),make_npc(103,30,true,1),make_npc(104,100)});
        w.join(1,"Caster",0,0,owner_box,0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);
        w.join(2,"Boundary foe",1012,0,peer_box,0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);
        w.join(3,"Far foe",1012.1f,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{500,500,100,100},100);
        w.join(4,"Ally",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(4,{500,500,100,100},100);
        w.join(5,"Dead foe",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(5,{0,500,100,100},100);
        w.join(6,"Unknown team",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,0);w.set_vitals(6,{500,500,100,100},100);
        const auto source=*w.effect_scope(1);check(w.life_steal(source,*w.effect_scope(103),*life_steal(153,0),100));w.set_vitals(1,{400,500,100,100},100);
        skills::Slots bar{};bar[0]={336,0};bar[1]={336,1};check(w.configure_adrenaline(source,bar,100));
        if(mode==1||mode==3)check(w.for_great_justice(source,*for_great_justice(343,mode==3),100));
        if(mode==2)check(w.status_mutation(source,370,900,0,100,100+berserker_stance(0)->duration,true));
        if(mode==4)check(w.status_mutation(source,56,900,0,100,1000,true));
        owner_box->drain();peer_box->drain();const auto plan=*to_limit(rank);
        check(w.complete_to_limit(source,plan,200));check(w.resources(1)->health==400+plan.health_bonus&&w.resources(1)->max_health==500+plan.health_bonus);
        const unsigned halves=mode==4?0:25*std::min(3u,plan.maximum_foes)*(mode==1?4u:mode==2?3u:2u);
        const auto pools=*w.adrenaline(source);for(const auto& p:pools)check(p.units==halves/2&&p.half==halves%2);
        const auto owner_packets=owner_box->drain(),peer_packets=peer_box->drain();
        const auto body=combat::detail::integer(21,1,562);check(std::count(owner_packets.begin(),owner_packets.end(),body)==1&&std::count(peer_packets.begin(),peer_packets.end(),body)==1);
        const auto charge=combat::adrenaline::snapshot_2022(1,pools[0]);check(std::count(owner_packets.begin(),owner_packets.end(),charge)==1&&std::count(peer_packets.begin(),peer_packets.end(),charge)==0);
        check(w.settle_resources(source,200+plan.duration)&&w.resources(1)->health==400&&w.resources(1)->max_health==500);
    }
    {world::Instance w;w.join(1,"Solo",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);const auto source=*w.effect_scope(1);
        skills::Slots bar{};bar[0]={336,0};check(w.configure_adrenaline(source,bar,100));check(w.complete_to_limit(source,*to_limit(0),200));
        check(w.adrenaline(source)->front().units==0&&w.resources(1)->max_health==510);
        check(w.complete_to_limit(source,*to_limit(0),201)&&w.resources(1)->health==410&&w.status_snapshot(source,201)->size()==1);
        auto stale=source;++stale.generation;check(!w.complete_to_limit(stale,*to_limit(0),202));auto bad=*to_limit(0);bad.radius=2000;check(!w.complete_to_limit(source,bad,202));}
    {world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);
        w.join(2,"Moving foe",1012,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);
        const auto source=*w.effect_scope(1);skills::Slots bar{};bar[0]={336,0};check(w.configure_adrenaline(source,bar,100));w.steer(2,0,1,100);
        check(w.complete_to_limit(source,*to_limit(0),200)&&w.adrenaline(source)->front().units==0);}
    {world::Instance w;std::vector<world::Instance::Npc> enemies;for(unsigned id=100;id<108;++id)enemies.push_back(make_npc(id,100));w.configure_npcs(enemies);
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);const auto source=*w.effect_scope(1);
        skills::Slots bar{};bar[0]={336,0};check(w.configure_adrenaline(source,bar,100));check(w.complete_to_limit(source,*to_limit(15),200)&&w.adrenaline(source)->front().units==150);}
    std::cout<<"PASS To the Limit World mixed NPC/player counting,boundary/dead/ally filters,modifiers,Health/Adrenaline publication,expiry/refresh and stale scopes;normal casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
