#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool v,std::source_location at=std::source_location::current()){
    if(!v)throw std::runtime_error("attack-hex skill runtime line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned hex:{43u,47u})for(unsigned attack:{322u,391u}){
        world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Warrior",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{1000,1000,100,100},100);w.set_vitals(2,{1000,1000,100,100},100);
        const auto foe=*w.effect_scope(1),actor=*w.effect_scope(2);
        combat::weapon::Owned sword{7,2,{20,20,1},12,false,false,attack==391?5u:27u};
        check(w.ordinary_weapon(actor,sword,20)&&w.physical_armor(foe,60));
        check(w.degeneration_hex(foe,actor,*skills::effects::degeneration_hex(hex,12),100,20));
        using namespace skills;using namespace casting;
        std::uint64_t now=200;unsigned commits=0;std::vector<Packet> packets;Hooks h;
        h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy=[&](auto cost){return w.consume_energy(2,cost);};
        h.weapon_snapshot=[&](){return w.attack_weapon(actor);};
        h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(actor,at);};
        h.attack_duration=[&](auto base,auto at){return w.attack_duration(actor,base,at);};
        h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.melee_target=[&](auto id,auto at){return w.melee_target(actor,id,at);};
        h.attack_skill_target=[&](auto id,auto skill,auto at){return w.melee_target(actor,id,at,skill);};
        h.attack_activity=[&](auto until){w.attack_activity(actor,until,now);};
        h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(actor);};
        h.control_epoch=[&](){return w.control_epoch(actor);};
        h.control_event_time=[&](auto epoch){return w.control_event_time(actor,epoch);};
        h.cast_activity=[&](auto skill,auto begins,auto ends){w.cast_activity(actor,skill,begins,ends);};
        h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return w.melee_attack(actor,target,weapon,plan,at);};
        Policy policy;policy.primary=1;policy.secondary=2;policy.campaigns=255;policy.learned[attack]=1;
        Slots bar{};bar[0]={attack,0};attributes::Ranks ranks{};ranks[17]=ranks[25]=12;Runtime runtime(2);
        check(runtime.start(now,{attack,0,999},bar,policy,ranks,true,h)==casting::Reason::target);
        check(w.resources(2)->health==1000&&w.resources(2)->energy==100);
        check(runtime.start(now,{attack,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(2)->health==(hex==43?924u:886u)&&w.resources(2)->energy==100-find_skill(attack)->energy_cost());
        now=attack==391?1400:800;runtime.update(now,h);
        check(!runtime.active()&&commits==(hex==43?0u:1u));
        check(w.resources(1)->health==1000);
        const auto recharge=find_skill(attack)->recharge;
        check(std::find(packets.begin(),packets.end(),slot_event(228,2,{attack,0},recharge))!=packets.end());
        // Hunter's Shot has a native one-second aftercast. Test recharge only
        // after that separate action restriction has elapsed.
        now+=1001;
        check(runtime.start(now,{attack,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
        if(hex==43){
            // The interrupt happened synchronously at admission200, although
            // Runtime observed it at800. Recharge must use the event timestamp.
            now=200+std::uint64_t(recharge)*1000-1;
            check(runtime.start(now,{attack,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
            ++now;
            check(runtime.start(now,{attack,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);
            runtime.cancel(h);
        }
        w.tick(now+2000);check(w.resources(1)->health==1000);
        check(!w.has_condition(foe,combat::conditions::Kind::bleeding,now+2000));
    }
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
