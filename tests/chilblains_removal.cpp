#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Chilblains removal line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned rank:{0u,12u}){
        world::Instance w;
        for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
        const auto source=*w.effect_scope(1),victim=*w.effect_scope(2);
        unsigned aura_heals=0,energy_returns=0;std::uint64_t clock=100;casting::Hooks h;
        h.resources=[&](auto id){return w.resources(id);};h.events=[](const auto&){};
        h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(2,amount,victim,at);};
        h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto id,auto,auto){return id==2;};
        h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(victim,skill,event,at);};
        h.status_mutation=[&](auto skill,auto effect,auto r,auto at,auto ends,auto active){return w.status_mutation(victim,skill,effect,r,at,ends,active);};
        h.effect_snapshot=[&](auto at){return w.status_snapshot(victim,at);};
        h.heal=[&](auto id,auto amount,auto range,auto self){++aura_heals;return w.heal_target(2,id,amount,range,self,clock);};
        h.heal_scoped=[&](auto scope,auto amount,auto range,auto self,auto at){return w.heal_target(2,scope.actor,amount,range,self,at);};
        h.restore_energy=[&](auto amount){energy_returns+=amount;return w.restore_energy(2,amount);};
        Policy policy;policy.primary=6;policy.secondary=3;policy.campaigns=255;policy.learned[180]=policy.learned[281]=1;
        Slots bar{};bar[0]={180,0};bar[1]={281,0};attributes::Ranks ranks{};ranks[12]=12;
        casting::Runtime receiver(2);
        check(receiver.start(100,{180,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);clock=350;receiver.update(clock,h);
        check(aura_heals==0&&energy_returns==0);
        check(w.status_mutation(victim,38,99,12,400,46400,true));
        check(receiver.start(1200,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);clock=2200;receiver.update(clock,h);
        check(aura_heals==1&&energy_returns==1);
        check(w.consume_energy(2,1,victim,2250));const auto before=w.resources(2)->energy;
        check(w.prepare_channeling(victim,281,1,2250)&&w.complete_channeling(victim,1,true,2250));
        check(w.resources(2)->energy==before+1);
        std::vector<world::Packet> visuals;
        check(w.stage_chilblains(source,victim,*effects::chilblains(rank),20,2300,&visuals));
        const auto remaining=w.status_snapshot(victim,2300);check(remaining&&remaining->size()==(rank==0?1:0));
        if(rank==0)check(remaining->front().skill==180);
        check(w.consume_energy(2,1,victim,2400));const auto after=w.resources(2)->energy;
        check(w.prepare_channeling(victim,281,2,2400)&&w.complete_channeling(victim,2,true,2400));
        check(w.resources(2)->energy==after&&!w.complete_channeling(victim,2,true,2400));
        clock=2400;receiver.update(clock,h);
        check(receiver.start(5000,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);clock=6000;receiver.update(clock,h);
        check(aura_heals==(rank==0?2:1)&&energy_returns==(rank==0?2:1));
        check(w.has_condition(source,combat::conditions::Kind::poison,6000));
    }
    std::cout<<"PASS Chilblains reviewed Aura/Channeling removal:rank count,newest ordering,real Aura cast/cache retirement and no stale Energy reward\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
