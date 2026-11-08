// Narrow friend access invokes the real begin helper, with no fake callbacks.
// This invokes the real begin path BEFORE the ordinary tick prepass. A public
// tick settles NPC degeneration first and cannot prove this callback ordering.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Immediate prebegin retirement line "+std::to_string(at.line()));
}
namespace fw::gw::world {
struct NpcImmediatePrebeginTestAccess {
    static bool begin(Instance& w,skills::effects::Scope source,skills::effects::Scope target,std::uint64_t now){
        std::lock_guard lock(w.mutex_);
        const auto controller=w.npc_immediate_ai_.find(source.actor);
        const auto body=w.npcs_.find(source.actor);const auto player=w.players_.find(target.actor);
        const auto live=w.npc_combat_.find(source.actor);const auto motion=w.npc_flare_ai_.find(source.actor);
        check(controller!=w.npc_immediate_ai_.end()&&controller->second.source==source&&!controller->second.pending);
        check(body!=w.npcs_.end()&&player!=w.players_.end()&&player->second.effect_scope==target);
        check(live!=w.npc_combat_.end()&&live->second.hex_scope()==source&&motion!=w.npc_flare_ai_.end());
        check(live->second.actor(live->second.lifetime())->health->current>0);
        check(w.mind_wrack_credit_.contains(source.actor));
        check(std::any_of(w.life_drain_links_.begin(),w.life_drain_links_.end(),[&](const auto& entry){
            return entry.second.target==source&&entry.second.caster==target;
        }));
        return w.begin_npc_immediate_locked(motion->second,body->second,player->second,now);
    }
    static bool has_book(Instance& w,skills::effects::Scope source){
        std::lock_guard lock(w.mutex_);const auto it=w.npc_immediate_ai_.find(source.actor);
        return it!=w.npc_immediate_ai_.end()&&it->second.source==source;
    }
};
}
int main()try{
    for(bool lethal:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=2;n.definition.flags=4;
        n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding={145,100,100,0,0,0,4,2};
        d.relation=combat::npcs::Relation::hostile_to_players;
        d.spawn_health=combat::Health{lethal?1.:100.,100};d.spawn_energy=combat::npcs::Energy{40,40};
        d.energy_pips=0;d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Linked caster",100,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
        const auto source=*w.effect_scope(100),target=*w.effect_scope(1);
        combat::ai::FlarePolicy p;p.source=source;p.target=target;p.level=2;p.speed=220;
        p.acquire=200;p.follow=1500;p.leash=1000;p.flight={1000,32};p.evidence="synthetic prebegin callback test";
        check(w.configure_npc_immediate_ai(p,{{{155,0},*skills::effects::shadow_damage(155,0),p.evidence}},100));
        check(w.configure_npc_wander(source,{{{80,0}},1200,6000,p.evidence},100));
        check(w.apply_mind_wrack(target,source,12,false,100));
        check(w.degeneration_hex(target,source,*skills::effects::degeneration_hex(109,0),100));
        check(w.npc_combat_snapshot(source)->health->current==(lethal?1.:100.));
        box->drain();
        // The linked caster settles Health here, not via an earlier public
        // tick/settle call. Life Siphon then deals actual due degeneration.
        check(world::NpcImmediatePrebeginTestAccess::begin(w,source,target,1100));
        const auto state=w.npc_combat_snapshot(source);check(state&&state->health);
        const auto cast=w.npc_flare_ai_snapshot(source)->cast;
        bool started=false;for(const auto& packet:box->drain()){
            if(packet.size()>=14&&(packet[0]==158||packet[0]==159)&&packet[1]==0&&packet[2]==60&&
               packet[3]==0&&packet[4]==0&&packet[5]==0&&packet[6]==100&&packet[7]==0)started=true;
        }
        if(lethal){
            check(state->health->current==0&&!cast&&!started);
            check(!world::NpcImmediatePrebeginTestAccess::has_book(w,source));
            check(w.npc_energy_snapshot(source)->current==40&&w.npc_ai_coverage().wandering==0);
        }else{
            check(state->health->current==98&&cast&&cast->skill&&cast->skill->skill==155&&started);
            check(world::NpcImmediatePrebeginTestAccess::has_book(w,source));
            check(w.npc_energy_snapshot(source)->current==30);
        }
    }
    std::cout<<"Real prebegin linked degeneration retires controller before selection; live control casts\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
