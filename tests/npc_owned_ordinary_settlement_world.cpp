// Requires narrow friend NpcOwnedOrdinarySettlementTestAccess in Instance.
// Invoke the real driver before public tick's field prepass, with a real Lava
// Font pulse. No fake death callback or direct mutation of objective membership.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Owned ordinary settlement line "+std::to_string(at.line()));
}
namespace fw::gw::world {
struct NpcOwnedOrdinarySettlementTestAccess {
    static void drive(Instance& w,skills::effects::Scope source,std::uint64_t now){
        std::lock_guard lock(w.mutex_);
        const auto motion=w.npc_flare_ai_.find(source.actor),unused=w.npc_flare_ai_.end();
        const auto body=w.npcs_.find(source.actor);const auto live=w.npc_combat_.find(source.actor);
        check(motion!=unused&&body!=w.npcs_.end()&&live!=w.npc_combat_.end()&&live->second.hex_scope()==source);
        w.drive_npc_quest_melee_locked(motion->second,body->second,live->second,now);
    }
};
}
world::Instance::Npc sample(unsigned id,unsigned health){
    world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=5;n.x=50;
    n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=5;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),health};
    d.damageable=true;d.conditions_allowed=true;d.fire_armor=60;d.physical_armor=0;n.combat_definition=d;return n;
}
int main()try{
    for(bool any_death:{false,true}){
        world::Instance w;auto out=std::make_shared<world::Outbox>();
        w.join(1,"Field owner",0,0,out,0,{}, {},100,1);w.set_vitals(1,{1000,1000,40,40},100);
        const auto owner=*w.effect_scope(1);check(w.physical_armor(owner,0));
        check(w.spawn_kill_encounter(owner,71,1,{sample(100,1000),sample(101,1)},[]{return true;},false,{},{},false,0,any_death));
        const auto source=*w.effect_scope(60000),sibling=*w.effect_scope(60001);
        combat::ai::FlarePolicy p;p.source=source;p.target=owner;p.level=5;p.speed=220;p.acquire=1000;
        p.follow=1500;p.leash=2000;p.flight={100,32};p.evidence="synthetic settlement regression";
        combat::ai::ordinary::Reviewed attack{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{100,32},p.evidence};
        check(w.configure_npc_owned_ordinary(owner,71,1,p,attack,combat::ai::FlareController::OwnedAcquisition::owner_proximity,100));
        check(w.damage_field(owner,*skills::effects::damage_field(195,12),20,100));
        check(w.npc_combat_snapshot(sibling)->health->current==1);out->drain();
        world::NpcOwnedOrdinarySettlementTestAccess::drive(w,source,1100);
        check(w.npc_combat_snapshot(sibling)->health->current==0);
        check(w.npc_combat_snapshot(source)->health->current>0&&w.npc_combat_snapshot(source)->health->current<1000);
        const auto motion=w.npc_flare_ai_snapshot(source);check(motion&&motion->melee_quest);
        if(any_death){check(motion->melee_quest->retired&&!motion->engaged);check(!w.npc_ordinary_pending(source));}
        else{check(!motion->melee_quest->retired&&motion->engaged);check(w.npc_ordinary_pending(source).has_value());}
        check(w.resources(1)->health==1000);
    }
    std::cout<<"Owned ordinary revalidates real sibling death completion before attack\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
