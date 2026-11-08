#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bile World timeline line "+std::to_string(at.line()));
}
world::Instance::Npc make_npc(unsigned id,double hp){
    world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;
    n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{hp,1000};
    d.damageable=d.conditions_allowed=true;d.fleshy=true;
    d.fleshy_evidence="synthetic Bile/poison timeline recipient";n.combat_definition=d;return n;
}
void join(world::Instance& w,unsigned id,float x,unsigned hp,unsigned team){
    w.join(id,"Timeline "+std::to_string(id),x,0,std::make_shared<world::Outbox>(),0,{}, {},100,team);
    w.set_vitals(id,{hp,1000,100,100},100);
}
int main()try{
    unsigned cases=0;
    for(bool is_npc:{false,true})for(bool dense:{false,true})for(bool reverse:{false,true})for(bool recipient_first:{false,true}){
        world::Instance w;const unsigned caster=reverse?10:1,recipient=reverse?1:10,dead=is_npc?100:20;
        if(is_npc)w.configure_npcs({make_npc(dead,2)});
        join(w,caster,0,1000,1);join(w,recipient,100,200,2);if(!is_npc)join(w,dead,0,2,2);
        check(w.degeneration_hex(*w.effect_scope(caster),*w.effect_scope(dead),*skills::effects::degeneration_hex(2058,0),100,20));
        if(dense)for(unsigned at=200;at<5100;at+=100)w.tick(at);
        const auto first=*w.effect_scope(recipient_first?recipient:caster);
        check(w.settle_resources(first,5100));w.tick(5100);
        if(w.resources(recipient)->health!=175||w.pending_putrid_deaths()!=0){
            std::cerr<<"case npc="<<is_npc<<" dense="<<dense<<" reverse="<<reverse<<" first="<<recipient_first
                <<" hp="<<w.resources(recipient)->health<<" pending="<<w.pending_putrid_deaths()<<"\n";
            for(const auto& reason:w.putrid_death_diagnostics())std::cerr<<reason<<"\n";
        }
        check(w.resources(recipient)->health==175&&w.pending_putrid_deaths()==0);
        if(is_npc)check(w.npc_combat_snapshot(*w.effect_scope(dead))->health->current==0);
        else check(w.resources(dead)->health==0);
        ++cases;
    }
    for(bool dense:{false,true}){
        world::Instance w;join(w,1,0,1000,1);join(w,2,0,2,2);join(w,3,400,200,2);join(w,4,0,200,2);
        w.move(2,400,0,100);
        check(w.degeneration_hex(*w.effect_scope(1),*w.effect_scope(2),*skills::effects::degeneration_hex(2058,0),100,20));
        if(dense)for(unsigned at=200;at<5100;at+=100)w.tick(at);
        check(w.settle_resources(*w.effect_scope(3),5100));w.tick(5100);
        check(w.resources(2)->health==0&&w.resources(3)->health==175&&w.resources(4)->health==200);
        check(w.pending_putrid_deaths()==0);++cases;
    }
    for(bool is_npc:{false,true})for(bool dense:{false,true})for(bool shield:{false,true})for(bool reversal:{false,true}){
        world::Instance w;if(is_npc)w.configure_npcs({make_npc(100,2)});
        join(w,1,0,1000,1);if(!is_npc)join(w,2,0,2,2);
        join(w,3,100,200,2);join(w,4,260,200,2);
        const auto caster=*w.effect_scope(1),recipient=*w.effect_scope(3),monk=*w.effect_scope(4);
        check(w.energy_capacity(4,6,0,100,{80,-2}));
        check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
        const auto seed=*skills::effects::healing_seed(12);
        check(w.complete_healing_seed(monk,recipient,seed,0,100));
        if(shield)check(w.status_mutation(recipient,1399,800,12,100,6100,true));
        if(reversal)check(w.status_mutation(recipient,307,900,0,100,8100,true));
        check(w.degeneration_hex(caster,*w.effect_scope(is_npc?100:2),*skills::effects::degeneration_hex(2058,0),100,20));
        if(dense)for(unsigned at=200;at<5100;at+=100)w.tick(at);
        check(w.settle_resources(recipient,5100));w.tick(5100);
        const unsigned damage=shield?20:25,convert=reversal?std::min(damage,skills::effects::reversal_fortune(0)->maximum_healing):0;
        check(w.resources(3)->health==200+convert-(damage-convert)+seed.healing);
        check(w.resources(4)->health==200+seed.healing&&w.pending_putrid_deaths()==0);++cases;
    }
    for(bool reverse:{false,true}){
        world::Instance w;w.configure_npcs({make_npc(100,reverse?6:2),make_npc(200,reverse?2:6)});
        join(w,1,0,1000,1);join(w,2,100,200,2);
        for(unsigned id:{100u,200u})check(w.degeneration_hex(*w.effect_scope(1),*w.effect_scope(id),*skills::effects::degeneration_hex(2058,0),100,20));
        check(w.settle_resources(*w.effect_scope(2),5100));w.tick(5100);
        check(w.resources(2)->health==150&&w.pending_putrid_deaths()==0);
        for(unsigned id:{100u,200u})check(w.npc_combat_snapshot(*w.effect_scope(id))->health->current==0);
        ++cases;
    }
    for(bool is_npc:{false,true}){
        world::Instance w;if(is_npc)w.configure_npcs({make_npc(100,1000)});
        join(w,1,0,1000,1);join(w,2,100,200,2);if(!is_npc)join(w,3,0,1000,2);
        const auto target=*w.effect_scope(is_npc?100:3);
        check(w.degeneration_hex(*w.effect_scope(1),target,*skills::effects::degeneration_hex(2058,0),100,20));
        check(w.settle_resources(*w.effect_scope(2),10100));w.tick(10100);
        check(w.resources(2)->health==200&&w.pending_putrid_deaths()==0);
        check(w.settle_resources(*w.effect_scope(2),20100));w.tick(20100);
        check(w.pending_putrid_deaths()==0);++cases;
    }
    // Bile death ownership is independent of whether a different condition's
    // departed caster can still receive kill credit.
    {
        world::Instance w;w.configure_npcs({make_npc(100,10)});
        join(w,1,0,1000,1);join(w,2,100,200,2);join(w,5,0,1000,1);
        const auto target=*w.effect_scope(100);
        check(w.apply_condition(*w.effect_scope(5),target,combat::conditions::Kind::poison,100,4000));
        check(w.degeneration_hex(*w.effect_scope(1),target,*skills::effects::degeneration_hex(2058,0),100,20));
        w.leave(5);check(w.has_condition(target,combat::conditions::Kind::poison,100));
        check(w.settle_resources(*w.effect_scope(2),5100));w.tick(5100);
        check(w.resources(2)->health==175&&w.pending_putrid_deaths()==0);++cases;
    }
    for(bool is_npc:{false,true}){
        world::Instance w;if(is_npc)w.configure_npcs({make_npc(100,10)});
        join(w,1,0,1000,1);join(w,2,100,200,2);if(!is_npc)join(w,3,0,10,2);
        check(w.degeneration_hex(*w.effect_scope(1),*w.effect_scope(is_npc?100:3),*skills::effects::degeneration_hex(2058,0),100,20));
        check(w.settle_resources(*w.effect_scope(2),10100));w.tick(10100);
        check(w.resources(2)->health==200&&w.pending_putrid_deaths()==0);++cases;
    }
    for(bool dense:{false,true}){
        world::Instance w;auto target=make_npc(100,4);target.x=300;
        auto corpse=make_npc(101,1);corpse.combat_definition->fleshy=true;
        corpse.combat_definition->fleshy_evidence="synthetic Well corpse";
        w.configure_npcs({target,corpse});join(w,1,0,1000,1);join(w,2,500,200,2);
        const auto caster=*w.effect_scope(1);
        check(w.damage_npc(caster,*w.effect_scope(101),{combat::Kind::armor_ignoring,1000,20,194},1248,100).killed);
        check(w.claim_well_corpse(caster,*skills::effects::well_cast_plan(93,0),1,100,252,[](const auto&){return true;}));
        check(w.degeneration_hex(caster,*w.effect_scope(100),*skills::effects::degeneration_hex(2058,0),100,20));
        if(dense)for(unsigned at=200;at<1500;at+=100)w.tick(at);
        check(w.settle_resources(*w.effect_scope(2),1500));w.tick(1500);
        check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==0);
        check(w.resources(2)->health==175&&w.pending_putrid_deaths()==0);++cases;
    }
    std::cout<<cases<<" World Bile coarse/fine, resource ordering, moving player, NPC chain and expiry cases\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
