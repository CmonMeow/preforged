// Trusted staged NPC recipient mechanics; no normal248 binding or paid cast.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static unsigned checks=0;
static void check(bool ok,std::source_location at=std::source_location::current()){
    ++checks;if(!ok)throw std::runtime_error("Retribution NPC line "+std::to_string(at.line()));
}
static world::Instance::Npc body(unsigned id,bool hostile=false,bool sensitive=false){
    world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};n.x=id==100?100:200;
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
    d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
    d.spawn_health=combat::Health{500,1000};d.spawn_energy=combat::npcs::Energy{20,20};d.energy_pips=0;
    d.physical_armor=60;d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic scoped Retribution recipient";
    d.holy_traits={sensitive,false};d.light_sensitive_evidence=d.summoned_evidence="synthetic explicit Holy traits";
    n.combat_definition=d;return n;
}
struct Fixture {
    std::unique_ptr<world::Instance> world=std::make_unique<world::Instance>();
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope source,target,enemy;
    Fixture(bool sensitive=false){world->configure_npcs({body(100),body(101,true,sensitive)});
        world->join(1,"Maintainer",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
        world->set_vitals(1,{500,1000,100,100},100);source=*world->effect_scope(1);target=*world->effect_scope(100);enemy=*world->effect_scope(101);
        check(world->ordinary_weapon(source,std::nullopt,20));}
};
int main()try{
    for(unsigned rank:{0u,12u,20u})for(bool sensitive:{false,true}){
        Fixture f(sensitive);auto& w=*f.world;
        check(w.stage_retribution(f.source,f.target,*retribution(rank),100,0,true));
        const auto status=w.status_snapshot(f.target,100);check(status&&status->size()==1&&status->front().skill==248&&!status->front().expires);
        check(w.maintained_enchantment_count(f.source,100)==1&&w.npc_energy_snapshot(f.target)->current==20);
        check(w.npc_combat_snapshot(f.target)->health->current==500); // No phantom Mending regeneration.
        const combat::ai::ordinary::Reviewed weapon{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"synthetic reviewed20 attack"};
        check(w.configure_npc_opposed_ordinary(f.enemy,weapon,1000));check(w.begin_npc_opposed_ordinary(f.enemy,f.target,1000).has_value());
        f.box->drain();w.tick(1750);
        const auto returned=std::min(5+rank,6u)*(sensitive?2:1);
        check(w.npc_combat_snapshot(f.target)->health->current==480);
        check(w.npc_combat_snapshot(f.enemy)->health->current==500-returned);
        check(w.retribution_damage_failures()==0&&w.retribution_pending_damage_contacts()==0);
        const auto packets=f.box->drain();std::size_t base=packets.size(),reaction=packets.size();
        const auto number=[](const world::Packet& p,unsigned at){return std::uint32_t(p[at])|std::uint32_t(p[at+1])<<8|std::uint32_t(p[at+2])<<16|std::uint32_t(p[at+3])<<24;};
        for(std::size_t i=0;i<packets.size();++i){const auto& p=packets[i];if(p.size()!=18||p[0]!=162||p[1])continue;
            if(number(p,6)==100&&number(p,10)==101)base=std::min(base,i);
            if(number(p,6)==101&&number(p,10)==1)reaction=std::min(reaction,i);}
        check(reaction<base&&base<packets.size());
        check(w.cancel_maintained_enchantment(f.source,status->front().instance,1750));
        check(w.status_snapshot(f.target,1750)->empty()&&w.maintained_enchantment_count(f.source,1750)==0);
    }
    for(unsigned mode=0;mode<4;++mode){Fixture f;auto& w=*f.world;
        check(w.stage_retribution(f.source,f.target,*retribution(20),100));const auto first=w.status_snapshot(f.target,100)->front();
        if(mode==0){w.join(2,"Second maintainer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,1000,100,100},100);
            const auto other=*w.effect_scope(2);check(w.stage_retribution(other,f.target,*retribution(0),100));
            const auto second=w.status_snapshot(f.target,100)->front();check(second.rank==20&&second.cause==2&&second.instance!=first.instance);
            check(w.maintained_enchantment_count(f.source,100)==0&&w.maintained_enchantment_count(other,100)==1);
        }else if(mode==1){w.leave(1);check(w.status_snapshot(f.target,100)->empty());
        }else if(mode==2){
            // Population replacement is supported only with no joined players.
            // First depart through the real cleanup path, then install a new
            // population and prove neither reused actor inherits the lease.
            w.leave(1);w.configure_npcs({body(100),body(101,true)});
            w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
            const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
            check(source!=f.source&&target!=f.target&&w.status_snapshot(target,100)->empty());
            check(w.maintained_enchantment_count(source,100)==0);
        }else{w.move(1,9000,0,100);w.tick(40100);check(w.maintained_enchantment_count(f.source,40100)==0&&w.status_snapshot(f.target,40100)->empty());}
    }
    {Fixture f;auto stale=f.target;++stale.generation;
        check(!f.world->stage_retribution(f.source,stale,*retribution(0),100));
        check(!f.world->stage_retribution(f.source,f.enemy,*retribution(0),100));
        check(f.world->stage_retribution(f.source,f.target,*retribution(0),100,12));check(f.world->npc_combat_snapshot(f.target)->health->current==538);
    }
    {auto w=std::make_unique<world::Instance>();auto unknown=body(100);unknown.combat_definition->spawn_energy.reset();unknown.combat_definition->energy_pips.reset();
        w->configure_npcs({unknown});w->join(1,"Maintainer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w->set_vitals(1,{500,1000,100,100},100);
        const auto source=*w->effect_scope(1),target=*w->effect_scope(100);
        check(!w->stage_retribution(source,target,*retribution(0),100)&&w->maintained_enchantment_count(source,100)==0);
    }
    std::cout<<"PASS staged Retribution NPC recipient "<<checks<<" checks; no paid/normal binding\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
