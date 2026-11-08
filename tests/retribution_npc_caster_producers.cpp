// DETACHED phase2 draft: requires actual NPC-maintainer support before
// registration. No allegiance bypass or synthetic direct damage receipt.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Retribution NPC caster producer line "+std::to_string(at.line()));
}
int main()try{unsigned cases=0;
    for(unsigned skill:{0u,320u,394u,330u,396u})for(unsigned rank:{0u,12u,20u}){
        ++cases;auto w=std::make_unique<world::Instance>();world::Instance::Npc npc;
        npc.id=npc.definition.id=100;npc.x=60;npc.definition.level=20;npc.definition.primary=3;npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
        d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=4;d.physical_armor=60;d.damageable=d.conditions_allowed=true;
        d.healing_allowed=true;d.healing_evidence="synthetic reviewed NPC maintainer";d.holy_traits={false,false};
        d.light_sensitive_evidence=d.summoned_evidence="synthetic explicit Holy traits";npc.combat_definition=d;w->configure_npcs({npc});
        auto box=std::make_shared<world::Outbox>();w->join(1,"Attacker",0,0,box,0,{}, {},100,1);w->set_vitals(1,{1000,1000,100,100},100);
        const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
        const auto plan=combat::weapon::attack_skill(skill?skill:320,12);check(plan.has_value());
        combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,plan->required_type};if(weapon.item_type==5)weapon.damage.raw_damage_type=1;
        check(w->ordinary_weapon(source,weapon,20)&&w->player_holy_equipment(source,0));
        w->configure_attack_critical_draws([]{return .999999;});w->configure_attack_miss_draws([]{return .5;});
        check(w->stage_retribution(target,target,*skills::effects::retribution(rank),100));
        check(w->maintained_enchantment_count(target,100)==1);box->drain();
        if(skill)check(w->melee_attack(source,skill==330?source:target,weapon,*plan,500));
        else check(w->ordinary_attack(source,target.actor,500));
        w->tick(1300);w->cancel_ordinary_attack(source);
        const auto packets=box->drain();double expected=0;unsigned bases=0,returns=0;
        const auto number=[](const world::Packet& p,unsigned at){return std::uint32_t(p[at])|std::uint32_t(p[at+1])<<8|std::uint32_t(p[at+2])<<16|std::uint32_t(p[at+3])<<24;};
        for(const auto& p:packets){if(p.size()!=18||p[0]!=162||p[1])continue;
            const auto victim=number(p,6),cause=number(p,10);const auto damage=-double(std::bit_cast<float>(number(p,14)))*1000;
            if(victim==100&&cause==1&&damage>0){++bases;expected+=std::min<double>(5+rank,std::max(1.,std::floor((damage+1e-4)*.33)));}
            if(victim==1&&cause==100&&damage>0)++returns;
        }
        check(bases>0&&returns==bases&&w->resources(1)->health==1000-unsigned(expected));
        check(w->retribution_pending_damage_contacts()==0&&w->retribution_damage_failures()==0);
    }
    std::cout<<"PASS NPC maintained Retribution actual ordinary/direct/ranged/Cyclone/dual producers "<<cases<<" cases\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
