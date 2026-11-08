#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Succor NPC World line "+std::to_string(at.line()));
}
world::Instance::Npc recipient(unsigned id=100){
    world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=4;
    n.definition.visual_adjustment={0,0,0,100};n.x=100;
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=4;
    d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};
    d.spawn_energy=combat::npcs::Energy{10,20};d.energy_pips=0;d.damageable=d.conditions_allowed=true;
    d.healing_allowed=true;d.healing_evidence="synthetic maintained recipient fixture";n.combat_definition=d;return n;
}
struct Fixture{
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope source,target;
    Fixture(bool healing=true){auto n=recipient();n.combat_definition->healing_allowed=healing;w.configure_npcs({n});
        w.join(1,"Maintainer",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
        source=*w.effect_scope(1);target=*w.effect_scope(100);}
};
int main()try{
    // Recall-only anchors may lack Energy; an actual Succor recipient may not.
    {world::Instance w;auto n=recipient();n.combat_definition->spawn_energy.reset();n.combat_definition->energy_pips.reset();w.configure_npcs({n});
     w.join(1,"Maintainer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
     const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);
     check(!w.succor_target_valid(owner,target,0,100)&&!w.complete_succor(owner,target,0,100)&&!w.npc_energy_snapshot(target));
    }
    for(unsigned mode=0;mode<5;++mode){Fixture f;std::vector<world::Packet> visuals;
        check(f.w.succor_target_valid(f.source,f.target,0,100));
        check(f.w.complete_succor(f.source,f.target,0,100,&visuals)&&visuals.size()==1);
        const auto first=f.w.status_snapshot(f.target,100)->front();check(first.skill==308&&!first.expires);
        const auto end=*mending_end_wire(f.source,f.target,first);f.box->drain();
        check(f.w.maintained_enchantment_count(f.source,100)==1);
        f.w.tick(3100);check(f.w.npc_combat_snapshot(f.target)->health->current==506);
        check(f.w.npc_energy_snapshot(f.target)->current==11);
        if(mode==0)check(f.w.cancel_maintained_enchantment(f.source,first.instance,3100));
        if(mode==1){check(f.w.complete_succor(f.source,f.target,0,3100));
            const auto next=f.w.status_snapshot(f.target,3100)->front();check(next.instance>first.instance);
            check(!f.w.cancel_maintained_enchantment(f.source,first.instance,3100));
            check(f.w.cancel_maintained_enchantment(f.source,next.instance,3100));}
        if(mode==2){f.w.move(1,9000,0,3100);f.w.tick(43100);}
        if(mode==3){f.w.leave(1);}
        if(mode==4){f.w.set_vitals(1,{0,1000,100,100},100);}
        check(f.w.status_snapshot(f.target,43100)->empty());
        if(mode!=3)check(f.w.maintained_enchantment_count(f.source,43100)==0);
        const auto packets=f.box->drain();
        // A departed maintainer's closed outbox need not receive end packets.
        if(mode!=3){check(std::count(packets.begin(),packets.end(),end.upkeep)==1);
            check(std::count(packets.begin(),packets.end(),end.effect)==1);}
        if(mode<2){f.w.tick(6100);check(f.w.npc_combat_snapshot(f.target)->health->current==506);
            check(f.w.npc_energy_snapshot(f.target)->current==11);}
    }
    // Three maintained links exceed this player's two baseline Energy pips.
    // The precise zero frontier releases upkeep until a sustainable rate remains.
    {world::Instance w;w.configure_npcs({recipient(100),recipient(101),recipient(102)});
        w.join(1,"Maintainer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{500,1000,1,100},100);const auto source=*w.effect_scope(1);
        for(unsigned id:{100u,101u,102u})check(w.complete_succor(source,*w.effect_scope(id),0,100));
        check(w.maintained_enchantment_count(source,100)==3);w.tick(20100);
        check(w.maintained_enchantment_count(source,20100)==2);
        unsigned active=0;for(unsigned id:{100u,101u,102u})active+=unsigned(w.status_snapshot(*w.effect_scope(id),20100)->size());
        check(active==2);
    }
    // Exact range crossing uses the moving player's float-rounded position,
    // including when its maintained recipient is a stationary NPC.
    for(float y:{0.f,171.251f,433.031f}){
        Fixture fine,coarse;
        for(auto* f:{&fine,&coarse}){check(f->w.complete_succor(f->source,f->target,0,100));f->w.move(1,9000,y,100);}
        std::uint64_t lo=100,hi=40100;
        const auto outside=[&](auto at){const auto p=fine.w.position(1,at);return std::hypot(double(p.first)-100.,double(p.second))>5000.;};
        check(!outside(lo)&&outside(hi));while(lo+1<hi){const auto mid=lo+(hi-lo)/2;if(outside(mid))hi=mid;else lo=mid;}
        check(fine.w.settle_resources(fine.source,hi-1));check(fine.w.maintained_enchantment_count(fine.source,hi-1)==1);
        check(fine.w.settle_resources(fine.source,hi));check(fine.w.maintained_enchantment_count(fine.source,hi)==0);
        coarse.w.tick(40100);check(coarse.w.maintained_enchantment_count(coarse.source,40100)==0);
        check(std::abs(fine.w.npc_combat_snapshot(fine.target)->health->current-
            coarse.w.npc_combat_snapshot(coarse.target)->health->current)<1e-8);
        check(fine.w.npc_energy_snapshot(fine.target)->current==coarse.w.npc_energy_snapshot(coarse.target)->current);
    }
    // Cross-maintainer replacement retires the exact prior upkeep owner.
    {Fixture f;f.w.join(2,"Second maintainer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        f.w.set_vitals(2,{500,1000,100,100},100);const auto other=*f.w.effect_scope(2);
        check(f.w.complete_succor(f.source,f.target,0,100));
        check(f.w.complete_succor(other,f.target,0,100));
        check(f.w.maintained_enchantment_count(f.source,100)==0&&f.w.maintained_enchantment_count(other,100)==1);
        f.w.tick(3100);check(f.w.npc_combat_snapshot(f.target)->health->current==506&&f.w.npc_energy_snapshot(f.target)->current==11);
    }
    // NPC effects and maintained spells must share collision-free allocation.
    {Fixture f;check(f.w.stage_watchful_healing(f.source,f.target,*watchful_healing(0),0,100));
        const auto stance=f.w.status_snapshot(f.target,100)->front().instance;
        check(f.w.complete_succor(f.source,f.target,0,100));const auto entries=*f.w.status_snapshot(f.target,100);
        check(entries.size()==2);const auto found=std::find_if(entries.begin(),entries.end(),[](const auto& e){return e.skill==308;});
        check(found!=entries.end()&&found->instance>stance);
    }
    // Divine Favor uses reviewed NPC healing permission and Health publication.
    for(bool healing:{false,true}){Fixture f(healing);
        check(f.w.complete_succor(f.source,f.target,5,100));
        check(f.w.npc_combat_snapshot(f.target)->health->current==(healing?516:500));}
    // Host population replacement is forbidden while players are present;
    // a rejected replacement must preserve the existing owned lease.
    {Fixture f;check(f.w.complete_succor(f.source,f.target,0,100));
        const auto old=f.w.status_snapshot(f.target,100)->front();
        const auto end=*mending_end_wire(f.source,f.target,old);f.box->drain();
        bool refused=false;try{f.w.configure_npcs({recipient()});}catch(const std::runtime_error&){refused=true;}
        check(refused&&*f.w.effect_scope(100)==f.target);
        check(f.w.maintained_enchantment_count(f.source,100)==1);
        check(f.w.cancel_maintained_enchantment(f.source,old.instance,100));
        const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),end.upkeep)==1);
        check(std::count(packets.begin(),packets.end(),end.effect)==1);}
    // A real opposing NPC attack can kill this allied recipient. Its death
    // must stop the maintainer's upkeep and publish exactly one end pair.
    {world::Instance w;auto ally=recipient();ally.combat_definition->spawn_health=combat::Health{1,1000};
        ally.combat_definition->physical_armor=0;
        auto enemy=recipient(101);enemy.x=150;
        enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
        w.configure_npcs({ally,enemy});auto box=std::make_shared<world::Outbox>();
        w.join(1,"Maintainer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,50,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100),attacker=*w.effect_scope(101);
        check(w.complete_succor(source,target,0,100));const auto effect=w.status_snapshot(target,100)->front();
        const auto end=*mending_end_wire(source,target,effect);box->drain();
        const combat::ai::ordinary::Reviewed weapon{20,20,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},
            "synthetic lethal NPC opposition regression"};
        check(w.configure_npc_opposed_ordinary(attacker,weapon,100));
        const auto token=w.begin_npc_opposed_ordinary(attacker,target,100);check(token.has_value());
        w.tick(token->completes);check(w.npc_combat_snapshot(target)->health->current==0);
        check(w.maintained_enchantment_count(source,token->completes)==0);
        w.tick(token->completes+1000);const auto packets=box->drain();
        check(std::count(packets.begin(),packets.end(),end.upkeep)==1);
        check(std::count(packets.begin(),packets.end(),end.effect)==1);}
    for(unsigned fault=0;fault<4;++fault){world::Instance w;auto n=recipient();
        if(fault==0){n.combat_definition->spawn_energy.reset();n.combat_definition->energy_pips.reset();}
        if(fault==1)n.combat_definition->energy_pips.reset();
        if(fault==2)n.combat_definition->relation=combat::npcs::Relation::neutral;
        w.configure_npcs({n});w.join(1,"Maintainer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{500,1000,100,100},100);const auto source=*w.effect_scope(1);auto target=*w.effect_scope(100);
        if(fault==3)++target.generation;
        check(!w.succor_target_valid(source,target,0,100)&&!w.complete_succor(source,target,0,100));
        check(w.maintained_enchantment_count(source,100)==0&&w.status_snapshot(*w.effect_scope(100),100)->empty());
    }
    std::cout<<"Succor NPC World application, resources, replacement, cancellation, range and retirement passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
