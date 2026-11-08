#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool value,std::source_location where=std::source_location::current()){
    if(!value)throw std::runtime_error("resource gain regression line "+std::to_string(where.line()));
}
std::unique_ptr<world::Instance> fixture(unsigned health=49,float x=100){
    const std::string hash(64,'a');
    std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 "+std::to_string(x)+" 0 0 - -\n");
    auto population=world::load_npc_catalog(appearance);
    // Health is known; armor and Energy are deliberately absent.
    std::istringstream metadata("gw-npc-combat 1 "+hash+"\nsource test authored "+hash+
        " tests/resource_gain.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 "+std::to_string(health)+" - 1 0 test test - - -\n");
    world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
    auto w=std::make_unique<world::Instance>();w->configure_npcs(population.for_map(146));
    w->join(1,"Caster",0,0,std::make_shared<world::Outbox>());w->set_vitals(1,{100,1000,0,40},100);return w;
}
int main()try{
    for(unsigned rank=0;rank<=20;++rank){
        const auto lost=*effects::resource_gain(1365,rank),ether=*effects::resource_gain(881,rank);
        check(lost.health==10+6*rank&&lost.energy==effects::interpolate(1,10,rank)&&effects::valid_resource_gain(lost));
        check(ether.health==0&&ether.energy==effects::interpolate(10,20,rank)&&ether.energy_threshold==effects::interpolate(5,10,rank)&&effects::valid_resource_gain(ether));
    }
    check(!effects::resource_gain(1365,21)&&!effects::resource_gain(881,21)&&!effects::resource_gain(146));
    for(unsigned target_health:{49u,50u,51u}){
        auto w=fixture(target_health);const auto source=*w->effect_scope(1),target=*w->effect_scope(100);const auto p=*effects::resource_gain(1365,12);
        const auto result=w->resource_gain(source,target,p,100);
        check(result&&result->condition_met==(target_health<50));
        check(w->resources(1)->health==(target_health<50?182u:100u)&&w->resources(1)->energy==(target_health<50?8u:0u));
        check(w->npc_combat_snapshot(target)->health->current==target_health); // no enemy Energy drain or damage.
        auto stale=target;++stale.generation;check(!w->resource_gain(source,stale,p,100));
        auto stale_source=source;++stale_source.generation;check(!w->resource_gain(stale_source,target,p,100));
        auto forged=p;++forged.energy;check(!w->resource_gain(source,target,forged,100));
        w->leave(1);w->join(1,"Replacement",0,0,std::make_shared<world::Outbox>());w->set_vitals(1,{100,1000,0,40},100);
        check(!w->resource_gain(source,target,p,100)&&w->resources(1)->health==100&&w->resources(1)->energy==0);
    }
    {
        auto w=fixture(49,1248.1f);check(!w->resource_gain(*w->effect_scope(1),*w->effect_scope(100),*effects::resource_gain(1365,12),100));
    }
    {
        auto w=fixture();const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
        w->set_vitals(1,{995,1000,39,40},100);
        const auto capped=w->resource_gain(source,target,*effects::resource_gain(1365,12),100);
        check(capped&&capped->condition_met&&capped->health_delta==5&&capped->energy_gained==1&&w->resources(1)->health==1000&&w->resources(1)->energy==40);
    }
    {
        auto w=fixture();const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
        w->set_vitals(1,{400,500,0,40},100);
        check(w->apply_condition(source,source,combat::conditions::Kind::deep_wound,100,1000));
        const auto gained=w->resource_gain(source,target,*effects::resource_gain(1365,12),100);
        check(gained&&gained->health_delta==82&&w->resources(1)->health==382); // Health gain is not reduced as healing.
    }
    {
        auto w=fixture();const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
        w->set_vitals(1,{50,500,0,40},100);
        check(w->apply_condition(source,source,combat::conditions::Kind::deep_wound,100,1000));
        const auto epoch=*w->control_epoch(source);
        const auto lethal=w->resource_gain(source,target,*effects::resource_gain(1365,0),100);
        check(lethal&&lethal->condition_met&&lethal->health_delta==-1&&w->resources(1)->health==0&&*w->control_epoch(source)>epoch);
    }
    {
        world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{100,1000,0,40},100);w.set_vitals(2,{50,100,10,40},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);const auto p=*effects::resource_gain(1365,12);
        check(w.apply_condition(source,target,combat::conditions::Kind::bleeding,100,1000));
        const auto below=w.resource_gain(source,target,p,110);
        check(below&&below->condition_met&&w.resources(2)->health==50); // precise49.94, displayed ceil50.
        check(w.resources(1)->health==182);
    }
    {
        auto w=fixture();const auto source=*w->effect_scope(1);const auto p=*effects::resource_gain(881,12);
        w->set_vitals(1,{100,1000,8,40},100);
        const auto result=w->resource_gain(source,source,p,100);check(result&&result->condition_met&&result->energy_gained==18&&w->resources(1)->energy==26);
        const auto no_gain=w->resource_gain(source,source,p,100);check(no_gain&&!no_gain->condition_met&&w->resources(1)->energy==26);
        check(!w->resource_gain(source,*w->effect_scope(100),p,100));
    }
    {
        auto w=fixture();const auto source=*w->effect_scope(1);w->set_vitals(1,{100,1000,8,40},100);
        check(w->energy_capacity(1,5,0,100,{20,1})); //3pips gives1Energy during the1second cast.
        const auto result=w->resource_gain(source,source,*effects::resource_gain(881,12),1100);
        check(result&&!result->condition_met&&w->resources(1)->energy==9); // strict less-than threshold9.
    }
    {
        auto w=fixture();const auto source=*w->effect_scope(1);check(w->energy_capacity(1,5,0,100,{0,0})); //40->20 capacity, preserves deficit as-20Energy.
        const auto p=*effects::resource_gain(881,12);
        const auto first=w->resource_gain(source,source,p,100),second=w->resource_gain(source,source,p,100);
        check(first&&second&&first->condition_met&&second->condition_met&&first->energy_gained==18&&second->energy_gained==18&&w->resources(1)->energy==16);
    }
    {
        world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Actor",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,1,30},100);const auto source=*w.effect_scope(1);
        check(w.energy_capacity(1,6,0,100,{0,-2})&&w.activate_overcast_energy(source,231,0,1,1000));
        check(w.energy_capacity(1,6,0,1000,{0,1})&&w.settle_resources(source,1500));
        const auto before=*w.overcast_energy_snapshot(source);check(overcast::precise_current(before)==-2550000);
        box->drain();const auto reward=w.resource_gain(source,source,*effects::resource_gain(881,12),1500);
        const auto after=*w.overcast_energy_snapshot(source);
        check(reward&&reward->condition_met&&reward->energy_gained==18);
        check(overcast::precise_current(after)-overcast::precise_current(before)==18*overcast::energy_unit&&after.resources.energy==9&&after.regeneration.remainder()==1500);
        check(!box->drain().empty());
    }
    for(unsigned skill:{881u,1365u}){
        using namespace casting;auto w=fixture(skill==1365?51u:49u);const auto source=*w->effect_scope(1);
        Policy policy;policy.primary=skill==881?5:4;policy.campaigns=0x1e;policy.learned[skill]=1;
        Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[skill==881?3:6]=12;
        Hooks h;std::vector<Packet> packets;std::string phase;unsigned calls=0,payments=0;std::uint64_t clock=100;
        h.events=[&](const auto& p){packets.insert(packets.end(),p.begin(),p.end());};
        h.resources=[&](auto id){return w->resources(id);};h.target_scope=[&](auto id){return w->effect_scope(id);};
        h.target_valid=[&](auto id,float range,bool self){return w->valid_heal_target(1,id,range,self);};
        h.projectile_target_valid=[&](auto id,float range){return w->projectile_target_valid(source,id,range,clock);};
        h.consume_energy=[&](auto amount){++payments;return w->consume_energy(1,amount,source);};
        h.diagnostic=[&](auto,const char* p){phase=p;};
        Runtime missing(1);check(missing.start(100,{skill,0,100},bar,policy,ranks,true,h)==casting::Reason::unsupported&&!payments);
        h.resource_gain=[&](auto target,const auto& plan,auto at){++calls;return w->resource_gain(source,target,plan,at);};
        if(skill==881){w->set_vitals(1,{100,1000,8,40},100);check(w->energy_capacity(1,5,0,100,{20,1}));}
        Runtime runtime(1);check(runtime.start(100,{skill,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted);
        if(skill==1365){const auto hit=w->damage_npc(source,*w->effect_scope(100),{combat::Kind::armor_ignoring,2,20,133},1248,200);check(hit.reason==combat::Reason::accepted&&w->npc_combat_snapshot(*w->effect_scope(100))->health->current==49);}
        const auto complete=skill==881?1100u:350u;clock=complete-1;runtime.update(clock,h);check(!calls);
        clock=complete;runtime.update(clock,h);check(calls==1&&phase==(skill==881?"applied_no_resource_gain":"applied"));
        runtime.update(clock+1,h);check(calls==1);
        const auto* row=find_skill(skill);check(std::count(packets.begin(),packets.end(),slot_event(228,1,{skill,0},row->recharge))==1);
        check(std::count(packets.begin(),packets.end(),generic(21,1,row->caster_body_resource))==1);
    }
    std::cout<<"Conditional resource gains: strict health/Energy predicates, regeneration, negative Energy, NPCs without Energy, cast and wire lifecycle PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
