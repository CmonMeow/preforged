#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills::effects;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Stamina World line "+std::to_string(at.line()));
}

int main()try{
    world::Instance world;
    auto box=std::make_shared<world::Outbox>();
    world.join(1,"Warrior",0,0,box,0,{}, {},100,1);
    world.set_vitals(1,{400,500,100,100},100);
    world.join(2,"Foe",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    world.set_vitals(2,{500,500,100,100},100);
    const auto owner=*world.effect_scope(1),foe=*world.effect_scope(2);
    box->drain();std::function<void()> publish;
    check(world.stage_stamina_signet_health(owner,*stamina_signet(0),100,&publish));
    check(world.resources(1)->max_health==550&&world.resources(1)->health==450);
    check(box->drain().empty()&&bool(publish));publish();
    const auto first=box->drain();
    check(std::any_of(first.begin(),first.end(),[](const auto& p){return p.size()>1&&p[0]==64&&p[1]==0;}));
    check(world.status_snapshot(owner,100)->size()==1);
    const auto newcomer=world.join(3,"Late",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,3);
    const auto owner_snapshot=std::find_if(newcomer.begin(),newcomer.end(),[](const auto& entry){return entry.id==1;});
    check(owner_snapshot!=newcomer.end()&&
          std::any_of(owner_snapshot->presentation.begin(),owner_snapshot->presentation.end(),
                      [](const auto& p){return p.size()>1&&p[0]==64&&p[1]==0;}));
    check(world.stage_stamina_signet_health(owner,*stamina_signet(12),200));
    check(world.resources(1)->max_health==500+stamina_signet(12)->health_bonus&&
          world.status_snapshot(owner,200)->size()==1);
    check(world.health_capacity(owner,600,300));
    check(world.resources(1)->max_health==600+stamina_signet(12)->health_bonus);
    check(world.physical_armor(foe,60));
    world.configure_attack_miss_draws([]{return .99;});
    world.configure_attack_block_draws([]{return .99;});
    combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,27};
    check(world.ordinary_weapon(owner,weapon,20));
    box->drain();check(world.ordinary_attack(owner,2,400));
    const auto timing=world.weapon_attack_timing(owner,400);check(timing.has_value());
    world.tick(400+timing->windup_ms);
    const auto status=world.status_snapshot(owner,400+timing->windup_ms);
    check(world.resources(1)->max_health==600&&status&&
          std::none_of(status->begin(),status->end(),[](const auto& e){return e.skill==1411;}));
    const auto ended=box->drain();
    check(std::any_of(ended.begin(),ended.end(),[](const auto& p){return p.size()>1&&p[0]==67&&p[1]==0;}));
    {
        world::Instance missed;missed.join(1,"Miss",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        missed.set_vitals(1,{400,500,100,100},100);
        missed.join(2,"Foe",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        missed.set_vitals(2,{500,500,100,100},100);
        const auto actor=*missed.effect_scope(1),target=*missed.effect_scope(2);
        check(missed.stage_stamina_signet_health(actor,*stamina_signet(0),100));
        check(missed.physical_armor(target,60)&&missed.ordinary_weapon(actor,weapon,20));
        check(missed.apply_condition(target,actor,combat::conditions::Kind::blind,100,5000));
        missed.configure_attack_miss_draws([]{return 0.;});
        check(missed.ordinary_attack(actor,2,200));
        const auto swing=missed.weapon_attack_timing(actor,200);check(swing.has_value());
        missed.tick(200+swing->windup_ms);
        check(missed.resources(1)->max_health==550&&missed.status_snapshot(actor,200+swing->windup_ms)->size()==1);
        missed.set_vitals(1,{300,600,100,100},100);
        check(missed.resources(1)->max_health==600&&missed.status_snapshot(actor,200+swing->windup_ms)->empty());
    }
    std::cout<<"PASS Signet of Stamina World capacity, indefinite status, recast, permanent resize and actual-hit ending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
