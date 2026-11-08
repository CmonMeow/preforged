#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Ward177 World line "+std::to_string(at.line()));
}
int main()try{
    {
        // Sparse held-keyboard updates must cross the field at the slowed
        // authoritative rate; the click-route consumer alone is insufficient.
        world::Instance held;auto events=std::make_shared<world::Outbox>();
        held.join(10,"Caster",0,0,events,0,{}, {},100,1);
        held.join(11,"Foe",500,0,events,0,{}, {},100,2);
        held.join(12,"Ally",500,20,events,0,{}, {},100,1);
        for(unsigned id:{10u,11u,12u})held.set_vitals(id,{1000,1000,100,100},100);
        check(held.apply_ward_foes(*held.effect_scope(10),*ward_foes_plan(0),100));
        held.steer(11,-1,0,200);held.steer(12,-1,0,200);
        events->drain();
        held.tick(1000);
        const auto [foe_x,foe_y]=held.position(11,1000);
        const auto [ally_x,ally_y]=held.position(12,1000);
        check(foe_x>290.f&&foe_x<301.f&&std::abs(foe_y)<.01f);
        check(ally_x>268.f&&ally_x<272.f&&std::abs(ally_y-20.f)<.01f);
        const auto entry=events->drain();
        check(std::count(entry.begin(),entry.end(),
            agent::movement_base_speed_2022(11,movement::player_run_speed*.5f))>=1);
        auto spectator=std::make_shared<world::Outbox>();
        const auto snapshots=held.join(13,"Spectator",700,0,spectator,0,{}, {},1000,1);
        const auto foe_snapshot=std::find_if(snapshots.begin(),snapshots.end(),
            [](const auto& snapshot){return snapshot.id==11;});
        check(foe_snapshot!=snapshots.end());
        check(std::count(foe_snapshot->presentation.begin(),foe_snapshot->presentation.end(),
            agent::movement_base_speed_2022(11,movement::player_run_speed*.5f))>=1);
        held.tick(6000);
        const auto exit=events->drain();
        check(std::count(exit.begin(),exit.end(),
            agent::movement_base_speed_2022(11,movement::player_run_speed))>=1);
    }
    {
        world::Instance held;auto events=std::make_shared<world::Outbox>();
        held.join(10,"Caster",0,0,events,0,{}, {},100,1);
        held.join(11,"Foe",300,0,events,0,{}, {},100,2);
        held.set_vitals(10,{1000,1000,100,100},100);
        held.set_vitals(11,{1000,1000,100,100},100);
        held.steer(11,-1,0,200);
        events->drain();
        check(held.apply_ward_foes(*held.effect_scope(10),*ward_foes_plan(0),500));
        const auto cast=events->drain();
        check(std::count(cast.begin(),cast.end(),
            agent::movement_base_speed_2022(11,movement::player_run_speed*.5f))>=1);
        held.tick(1000);
        const auto [x,y]=held.position(11,1000);
        check(x>138.f&&x<145.f&&std::abs(y)<.01f);
    }
    {
        world::Instance held;auto events=std::make_shared<world::Outbox>();
        held.join(10,"Caster",0,0,events,0,{}, {},100,1);
        held.join(11,"Reverse Foe",100,0,events,0,{}, {},100,2);
        held.set_vitals(10,{1000,1000,100,100},100);
        held.set_vitals(11,{1000,1000,100,100},100);
        check(held.apply_ward_foes(*held.effect_scope(10),*ward_foes_plan(0),100));
        events->drain();held.steer(11,-1,0,200,4);
        const auto start=events->drain();
        check(std::count(start.begin(),start.end(),
            agent::movement_base_speed_2022(11,movement::player_run_speed*.5f))>=1);
        check(std::count(start.begin(),start.end(),
            agent::movement_speed_2022(11,.66f,4))>=1);
        held.tick(1200);
        const auto [x,y]=held.position(11,1200);
        check(x>3.f&&x<7.f&&std::abs(y)<.01f);
    }
    {
        world::Instance moving;auto events=std::make_shared<world::Outbox>();
        moving.join(10,"Caster",0,0,events,0,{}, {},100,1);
        moving.join(11,"Foe",500,0,events,0,{}, {},100,2);
        moving.set_vitals(10,{1000,1000,100,100},100);
        moving.set_vitals(11,{1000,1000,100,100},100);
        check(moving.apply_ward_foes(*moving.effect_scope(10),*ward_foes_plan(0),100));
        events->drain();
        moving.move(11,0,0,200);
        events->drain();
        moving.tick(1500);
        const auto [x,y]=moving.position(11,1500);
        check(std::abs(x-223.8f)<2.f&&std::abs(y)<.01f);
        const auto packets=events->drain();
        check(std::count(packets.begin(),packets.end(),
            agent::movement_base_speed_2022(11,movement::player_run_speed*.5f))>=1);
        moving.tick(3200);events->drain();
        moving.move(11,500,0,3300);
        const auto start=events->drain();
        check(std::count(start.begin(),start.end(),
            agent::movement_base_speed_2022(11,movement::player_run_speed*.5f))>=1);
        moving.tick(5600);
        const auto exit=events->drain();
        check(std::count(exit.begin(),exit.end(),
            agent::movement_base_speed_2022(11,movement::player_run_speed))>=1);
    }
    {
        world::Instance moving;auto events=std::make_shared<world::Outbox>();
        moving.join(10,"Caster",0,0,events,0,{}, {},100,1);
        moving.join(11,"Foe",500,0,events,0,{}, {},100,2);
        moving.set_vitals(10,{1000,1000,100,100},100);
        moving.set_vitals(11,{1000,1000,100,100},100);
        moving.move(11,0,0,200);
        check(moving.apply_ward_foes(*moving.effect_scope(10),*ward_foes_plan(0),500));
        moving.tick(2100);
        const auto [x,y]=moving.position(11,2100);
        check(std::abs(x-137.4f)<2.f&&std::abs(y)<.01f);
    }
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    for(unsigned id=1;id<=3;++id){
        w.join(id,"Actor",id==3?500:0,0,box,0,{}, {},100,id==3?2:1);
        w.set_vitals(id,{1000,1000,100,100},100);
    }
    const auto caster=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);
    const auto plan=ward_foes_plan(0);check(bool(plan));
    std::vector<world::Packet> visuals;check(w.apply_ward_foes(caster,*plan,100,&visuals));
    check(visuals.size()==2);check(!w.ward_foes_multiplier(caster,100));
    check(!w.ward_foes_multiplier(ally,100));
    check(!w.ward_foes_multiplier(foe,100));
    w.move(3,200,0,200);w.tick(2000);
    check(w.ward_foes_multiplier(foe,2000)==.5f);
    w.move(3,500,0,2100);w.tick(4000);
    check(!w.ward_foes_multiplier(foe,4000));
    w.move(3,200,0,4100);w.tick(6000);
    check(w.ward_foes_multiplier(foe,6000)==.5f);
    w.leave(1);w.join(1,"Reused",500,0,box,0,{}, {},6100,1);
    w.set_vitals(1,{1000,1000,100,100},100);
    check(w.ward_foes_multiplier(foe,6100)==.5f);
    check(!w.ward_foes_multiplier(foe,8100));
    std::cout<<"Ward177 World field, team eligibility, entry/exit and expiry passed; movement binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
