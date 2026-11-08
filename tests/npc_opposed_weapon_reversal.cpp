#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Opposed weapon Reversal line "+std::to_string(at.line()));
}
struct Fixture{
    world::Instance w;skills::effects::Scope source,target;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(bool hostile_source,unsigned health,unsigned damage,unsigned type,unsigned factor,bool ranged){
        std::vector<world::Instance::Npc> actors;
        for(unsigned id:{100u,101u}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;n.x=id==100?0:100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding={146,id,id,0,0,0,0,20};
            const bool hostile=(id==100)==hostile_source;
            d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
            d.spawn_health=combat::Health{double(id==101?health:1000),1000};
            d.damageable=d.conditions_allowed=true;d.physical_armor=d.fire_armor=60;
            d.healing_allowed=true;d.healing_evidence="synthetic known NPC healing";
            d.fire_multiplier=id==101?factor:1;d.fire_multiplier_evidence="synthetic fire multiplier";
            n.combat_definition=d;actors.push_back(n);
        }
        w.configure_npcs(actors);w.join(1,"Observer",200,0,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);source=*w.effect_scope(100);target=*w.effect_scope(101);
        w.configure_attack_miss_draws([]{return .5;});
        check(w.configure_npc_opposed_ordinary(source,{damage,damage,0,type,false,
            ranged?combat::ai::ordinary::Delivery::ranged:combat::ai::ordinary::Delivery::melee,
            750,2000,128,{1200,32},"synthetic opposed weapon profile"},1000));
    }
    void swing(){
        check(w.begin_npc_opposed_ordinary(source,target,1000).has_value());w.tick(1750);w.tick(2000);
        const auto outcome=w.take_attack_outcomes(source);
        check(outcome.size()==1&&outcome.front().outcome==combat::weapon::AttackOutcome::hit);
    }
    double health(){return w.npc_combat_snapshot(target)->health->current;}
};
int main()try{
    unsigned cases=0;
    for(bool hostile:{false,true})for(bool ranged:{false,true})for(unsigned type:{0u,5u})
    for(unsigned factor:{1u,2u})for(unsigned damage:{5u,20u,200u}){
      // Measure this exact reviewed NPC weapon without conversion. This test
      // targets conversion ordering, not the separately tested mastery curve.
      Fixture baseline(hostile,1000,damage,type,factor,ranged);baseline.swing();
      const double raw=1000-baseline.health();check(raw>0);
      for(unsigned health:{1u,10u,500u,1000u})for(int rank:{-1,0,12,20}){
        Fixture f(hostile,health,damage,type,factor,ranged);unsigned effect=0;
        if(rank>=0){check(f.w.stage_npc_reversal(f.target,rank,100));
            const auto before=*f.w.status_snapshot(f.target,100);
            for(const auto& e:before)if(e.skill==307)effect=e.instance;
            check(effect!=0);
        }
        f.box->drain();f.swing();
        const double converted=rank<0?0:std::min(raw,double(skills::effects::reversal_fortune(rank)->maximum_healing));
        const double expected=std::max(0.,std::min(1000.,health+converted)-(raw-converted));
        if(std::abs(f.health()-expected)>.001)std::cerr<<hostile<<' '<<ranged<<' '<<type<<' '<<factor<<' '<<damage<<' '<<health<<' '<<rank<<" actual="<<f.health()<<" expected="<<expected<<'\n';
        check(std::abs(f.health()-expected)<.001);
        const auto status=*f.w.status_snapshot(f.target,2000);
        check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;}));
        const auto packets=f.box->drain();
        if(rank>=0){world::Packet removed;world::u16(removed,67);world::u32(removed,f.target.actor);world::u32(removed,effect);
            check(std::count(packets.begin(),packets.end(),removed)==1);}
        // NPC attack credit must not be fabricated as a player kill.
        check(f.w.take_npc_deaths(*f.w.effect_scope(1)).empty());
        ++cases;
    }
    }
    for(bool ranged:{false,true})for(unsigned damage:{5u,200u})for(unsigned rank:{0u,12u,20u}){
        Fixture baseline(true,1000,damage,0,1,ranged);baseline.swing();
        const double raw=1000-baseline.health();
        Fixture f(true,500,damage,0,1,ranged);
        const auto caster=*f.w.effect_scope(1);f.w.set_vitals(1,{500,1000,100,100},100);
        const auto seed=*skills::effects::healing_seed(12);
        check(f.w.complete_healing_seed(caster,f.target,seed,0,100));
        check(f.w.stage_npc_reversal(f.target,rank,100));f.box->drain();f.swing();
        const auto converted=std::min(raw,double(skills::effects::reversal_fortune(rank)->maximum_healing));
        check(f.w.resources(1)->health==500+(raw>converted?seed.healing:0));
        ++cases;
    }
    for(bool ranged:{false,true})for(unsigned rank:{0u,12u,20u}){
        Fixture f(false,10,20,0,1,ranged);const auto player=*f.w.effect_scope(1);
        check(f.w.stage_npc_reversal(f.target,rank,100));
        check(f.w.apply_condition(player,player,combat::conditions::Kind::deep_wound,100,10000));
        check(f.w.condition_transfer(player,f.target,*skills::effects::condition_transfer(132,12),100));
        f.box->drain();f.swing();check(f.health()==0);
        // Healing-caused death consumes the conversion and must not create a
        // remaining weapon-loss event, or a fabricated player kill source.
        const auto status=*f.w.status_snapshot(f.target,2000);
        check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;}));
        check(f.w.take_npc_deaths(player).empty());++cases;
    }
    std::cout<<cases<<" opposed NPC weapon contacts preserve conversion-before-loss, one-use retirement, both sides, ranged delivery, actual-loss Seed and healing-caused death\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
