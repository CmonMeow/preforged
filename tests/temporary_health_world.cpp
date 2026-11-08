#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("temporary Health World line "+std::to_string(at.line()));}
int main()try{
    {world::Instance w;w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{5,500,100,100},100);w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);check(!w.status_mutation(owner,316,7,0,100,10100,true));check(w.stage_to_limit_health(owner,*to_limit(0),100));check(w.life_steal(foe,owner,*life_steal(153,0),200));check(w.resources(1)->health==0&&w.resources(1)->max_health==500&&w.status_snapshot(owner,200)->empty());}
    {world::Instance w;w.join(1,"Capacity rollback",0,0,std::make_shared<world::Outbox>());w.set_vitals(1,{400,500,20,20},100);const auto owner=*w.effect_scope(1);check(w.reviewed_health_capacity(owner,500,0,0)&&w.energy_capacity(1,1,0,0));check(w.stage_to_limit_health(owner,*to_limit(0),100));check(!w.reviewed_health_capacity(owner,999995,0,100));check(w.honeycomb(owner,100,true,[]{return true;})&&w.resources(1)->max_health==535);}
    {world::Instance w;w.join(1,"Morale",0,0,std::make_shared<world::Outbox>());w.set_vitals(1,{400,500,30,66},100);const auto owner=*w.effect_scope(1);check(w.reviewed_health_capacity(owner,480,20,0)&&w.energy_capacity(1,6,12,0,{10,2}));check(w.stage_to_limit_health(owner,*to_limit(0),100));
        check(!w.honeycomb(owner,100,true,[]{return false;})&&w.resources(1)->max_health==510);check(w.honeycomb(owner,100,true,[]{return true;}));check(w.resources(1)->max_health==534&&w.resources(1)->health==434);check(w.settle_resources(owner,10100)&&w.resources(1)->max_health==524&&w.resources(1)->health==424);}
    {world::Instance w;w.join(1,"Refresh",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);const auto owner=*w.effect_scope(1);check(w.stage_to_limit_health(owner,*to_limit(0),100)&&w.stage_to_limit_health(owner,*to_limit(0),200));check(w.resources(1)->health==410&&w.status_snapshot(owner,200)->size()==1);check(w.health_capacity(owner,500,10200)&&w.resources(1)->max_health==500&&w.resources(1)->health==400);}
    {world::Instance w;w.join(1,"Old",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);const auto old=*w.effect_scope(1);check(w.stage_to_limit_health(old,*to_limit(0),100));w.leave(1);w.join(1,"New",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);const auto owner=*w.effect_scope(1);check(old!=owner&&!w.stage_to_limit_health(old,*to_limit(0),100)&&w.resources(1)->max_health==500);}
    for(bool dense:{false,true})for(bool wound:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Player",0,0,box,0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);
        const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);check(w.stage_to_limit_health(owner,*to_limit(0),100));check(w.resources(1)->max_health==510&&w.resources(1)->health==410);
        if(wound)check(w.apply_condition(foe,owner,combat::conditions::Kind::deep_wound,100,20000));
        check(w.health_capacity(owner,530,200));check(w.resources(1)->max_health==(wound?440u:540u)&&w.resources(1)->health==(wound?340u:440u));
        if(dense)for(unsigned at=300;at<10100;at+=137)check(w.settle_resources(owner,at));check(w.settle_resources(owner,10100));check(w.resources(1)->max_health==(wound?430u:530u)&&w.resources(1)->health==(wound?330u:430u));
        const auto status=*w.status_snapshot(owner,10100);check(std::none_of(status.begin(),status.end(),[](const auto& entry){return entry.skill==316;}));
    }
    {world::Instance w;w.join(1,"Player",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{5,500,100,100},100);w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);check(w.stage_to_limit_health(owner,*to_limit(12),100));check(w.life_steal(foe,owner,*life_steal(153,0),200));check(w.settle_resources(owner,18100));check(w.resources(1)->health==1&&w.resources(1)->max_health==500);check(w.heal_target(1,1,1,1248,true,18100));check(w.resources(1)->health==0);}
    {world::Instance w;w.join(1,"Reset",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);const auto owner=*w.effect_scope(1);check(w.stage_to_limit_health(owner,*to_limit(0),100));w.set_vitals(1,{300,600,100,100},100);check(w.resources(1)->max_health==600&&w.resources(1)->health==300&&w.status_snapshot(owner,100)->empty());check(w.settle_resources(owner,20100)&&w.resources(1)->max_health==600);}
    for(bool dense:{false,true})for(bool wound:{false,true})for(unsigned rank:{0u,4u,12u,20u}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Mixed health",0,0,box,0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);
        const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);const auto pain=*endure_pain(rank);
        check(!w.status_mutation(owner,347,9,rank,100,100+pain.duration,true));
        check(w.stage_endure_pain_health(owner,pain,100)&&w.stage_to_limit_health(owner,*to_limit(0),100));
        if(wound)check(w.apply_condition(foe,owner,combat::conditions::Kind::deep_wound,100,40000));
        check(w.resources(1)->max_health==500+pain.health_bonus+10-(wound?100:0));
        if(dense)for(unsigned at=101;at<23100;at+=137)check(w.settle_resources(owner,at));check(w.settle_resources(owner,23100));
        check(w.resources(1)->health==(wound?300u:400u)&&w.resources(1)->max_health==(wound?400u:500u));
        const auto status=*w.status_snapshot(owner,23100);check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==347||e.skill==316;}));
    }
    for(unsigned mode=0;mode<3;++mode){world::Instance w;w.join(1,"Cleanup",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{400,500,100,100},100);const auto owner=*w.effect_scope(1);
        check(w.stage_endure_pain_health(owner,*endure_pain(20),100)&&w.stage_to_limit_health(owner,*to_limit(0),100));
        if(mode==0){check(w.kill(owner,200));check(w.resources(1)->health==0&&w.resources(1)->max_health==500);}
        else if(mode==1){w.set_vitals(1,{300,600,100,100},100);check(w.resources(1)->health==300&&w.resources(1)->max_health==600);}
        else{w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>());w.set_vitals(1,{300,600,100,100},100);check(!w.stage_endure_pain_health(owner,*endure_pain(0),200));}
        const auto current=*w.effect_scope(1);check(w.status_snapshot(current,200)->empty());check(w.settle_resources(current,30100));
    }
    {world::Instance w;w.join(1,"Pain refresh",0,0,std::make_shared<world::Outbox>());w.set_vitals(1,{5,500,100,100},100);const auto owner=*w.effect_scope(1);
        check(w.stage_endure_pain_health(owner,*endure_pain(20),100)&&w.stage_to_limit_health(owner,*to_limit(0),100));
        check(w.stage_endure_pain_health(owner,*endure_pain(0),200));check(w.resources(1)->health==105&&w.resources(1)->max_health==600);
        check(w.settle_resources(owner,7200)&&w.resources(1)->health==15&&w.resources(1)->max_health==510);
        check(w.settle_resources(owner,10100)&&w.resources(1)->health==5&&w.resources(1)->max_health==500);
    }
    {world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Pain debt",0,0,box,0,{}, {},100,1);w.set_vitals(1,{5,500,100,100},100);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);
        check(w.stage_endure_pain_health(owner,*endure_pain(0),100));const auto entry=w.status_snapshot(owner,100)->front();box->drain();
        check(w.life_steal(foe,owner,*life_steal(153,0),200)&&w.settle_resources(owner,7100));
        check(w.resources(1)->health==1&&w.resources(1)->max_health==500);
        world::Packet end{67,0};combat::detail::put(end,1);combat::detail::put(end,entry.instance);const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),end)==1);
        check(w.heal_target(1,1,1,1248,true,7100)&&w.resources(1)->health==0); // Insufficient healing cannot erase latent negative Health.
    }
    std::cout<<"PASS temporary Health World dense/sparse/equipment/DeepWound/expiry,latent debt/insufficient healing and trusted reset;normal316stillunbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
