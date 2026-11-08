#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("enchantment removal regression line "+std::to_string(at.line()));
}
int main()try{
    using namespace skills;using namespace effects;
    check(enchantment_removal(69,0)->damage==14&&enchantment_removal(69,12)->damage==83&&enchantment_removal(69,20)->damage==129);
    check(enchantment_removal(68,0)->energy==8&&enchantment_removal(68,12)->energy==15&&enchantment_removal(68,20)->energy==20);
    check(enchantment_removal(68,0)->health==40&&enchantment_removal(68,12)->health==104&&enchantment_removal(68,20)->health==147);
    check(!enchantment_removal(67)&&!enchantment_removal(68,21));
    for(unsigned skill:{68u,69u})for(unsigned rank:{0u,12u,20u})for(bool enchanted:{false,true}){
        world::Instance w;for(unsigned id:{1u,2u,3u}){
            w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{500,1000,40,100},100);
        }
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);const auto plan=*enchantment_removal(skill,rank);
        if(enchanted){check(w.status_mutation(b,180,99,12,100,60100,true));check(w.status_mutation(b,38,1,12,100,46100,true));}
        auto forged=plan;++forged.energy;check(!w.enchantment_removal(a,b,forged,20,100));
        auto stale=b;++stale.generation;check(!w.enchantment_removal(a,stale,plan,20,100));
        check(!w.enchantment_removal(a,a,plan,20,100));
        check(w.enchantment_removal(a,b,plan,20,100));
        check(w.resources(2)->health==500-(enchanted?plan.damage:0)&&w.resources(3)->health==500);
        check(w.resources(1)->health==500+(enchanted?plan.health:0)&&w.resources(1)->energy==40+(enchanted?plan.energy:0));
        const auto left=*w.status_snapshot(b,100);check(left.size()==(enchanted?1u:0u));if(enchanted)check(left[0].skill==180);
    }
    // Newest removal selects an ordinary enchantment, absorption, or Reversal.
    // Only protections still present after that removal may affect the hit.
    for(unsigned newest:{0u,1399u,307u}){
        world::Instance w;
        for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{1000,1000,100,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        if(newest!=1399)check(w.status_mutation(b,1399,80,12,100,6100,true));
        if(newest!=307)check(w.status_mutation(b,307,81,0,100,8100,true));
        check(w.status_mutation(b,newest?newest:180,82,newest==1399?12:0,100,6100,true));
        check(w.enchantment_removal(a,b,*enchantment_removal(69,12),20,100));
        const unsigned damage=83-(newest==1399?0:5)-(newest==307?0:15);
        check(w.resources(2)->health==1000-damage);
        const auto active=*w.status_snapshot(b,100);
        check(std::none_of(active.begin(),active.end(),[](auto e){return e.skill==307;}));
        check(std::any_of(active.begin(),active.end(),[](auto e){return e.skill==1399;})==(newest!=1399));
    }
    // Shielding Hands' end heal occurs before the hit; its reduction is gone.
    {
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,40,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.protective_enchantment(b,b,*regeneration::reviewed(299,12),0,100));
        check(w.enchantment_removal(a,b,*enchantment_removal(69,12),20,100));
        check(w.resources(2)->health==458);w.tick(8100);check(w.resources(2)->health==458);
    }
    // Health gain bypasses Defile Flesh and Deep Wound's healing penalty,
    // while still settling pending Deep Wound Health debt.
    for(bool wounded:{false,true})for(unsigned health:{500u,50u}){
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{health,1000,40,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.degeneration_hex(b,a,*degeneration_hex(129,12),100));
        if(wounded)check(w.apply_condition(b,a,combat::conditions::Kind::deep_wound,100,60000));
        check(w.status_mutation(b,180,1,12,100,60100,true));
        check(w.enchantment_removal(a,b,*enchantment_removal(68,12),20,100));
        check(w.resources(1)->health==health+(wounded?4u:104u)); // Deep Wound loss is capped at100.
    }
    {
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,id==1?3u:40u,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.energy_capacity(1,5,0,100,{80,-2})&&w.energy_capacity(1,5,0,200,{0,-2}));
        check(w.status_mutation(b,180,1,12,200,60200,true));
        check(w.enchantment_removal(a,b,*enchantment_removal(68,12),20,300));
        check(w.resources(1)->energy==0&&w.resources(1)->health==1000);
        check(w.energy_capacity(1,5,0,400,{80,-2})&&w.resources(1)->energy==18);
    }
    for(unsigned newest:{0u,1399u,307u}){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;
        n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
        d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="authored Shatter conversion";
        n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(100);
        if(newest!=1399)check(w.stage_npc_shield_absorption(b,12,100));
        if(newest!=307)check(w.stage_npc_reversal(b,0,100));
        if(newest==1399)check(w.stage_npc_shield_absorption(b,12,100));
        else if(newest==307)check(w.stage_npc_reversal(b,0,100));
        else check(w.stage_npc_vigorous_spirit(b,*vigorous_spirit(0),100));
        check(w.enchantment_removal(a,b,*enchantment_removal(69,12),20,100));
        const unsigned loss=83-(newest==1399?0:5)-(newest==307?0:15);
        check(w.npc_combat_snapshot(b)->health->current==1000-loss);
    }
    const std::string hash(64,'a');std::istringstream p("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
    std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/enchantment_removal.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 1000 1000 80 1 1 test test test - test 60 60 test test\n");
    auto population=world::load_npc_catalog(p);world::bind_combat_catalog(population,combat::npcs::load_catalog(m,hash),hash);
    {
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?999u:500u,1000,99,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.status_mutation(b,180,1,12,100,60100,true));
        check(w.enchantment_removal(a,b,*enchantment_removal(68,12),20,100));
        check(w.resources(1)->health==1000&&w.resources(1)->energy==100);
    }
    // The end heal can settle lethal wound debt. Removal must complete without
    // a second damage attempt or a duplicate death/control invalidation.
    {
        world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{50,1000,40,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.protective_enchantment(b,b,*regeneration::reviewed(299,0),0,100));
        check(w.apply_condition(a,b,combat::conditions::Kind::deep_wound,100,60000));
        const auto epoch=*w.control_epoch(b);
        check(w.enchantment_removal(a,b,*enchantment_removal(69,12),20,100));
        check(w.resources(2)->health==0&&*w.control_epoch(b)==epoch+1);
    }
    // Runtime's local Aura observer must disappear when the world removes the
    // enchantment; the next Orison causes one base heal, no stale Aura trigger.
    {
        using namespace casting;world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,100,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);unsigned heals=0;std::uint64_t now=100;Hooks h;
        h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[&](auto n){return w.consume_energy(2,n);};
        h.target_valid=[](auto id,float,bool){return id==2;};h.restore_energy=[](auto){return true;};
        h.heal=[&](auto id,auto amount,float range,bool self){++heals;return w.heal_target(2,id,amount,range,self,now);};
        h.status_mutation=[&](auto skill,auto effect,auto rank,auto at,auto ends,bool active){return w.status_mutation(b,skill,effect,rank,at,ends,active);};
        h.effect_snapshot=[&](auto at){return w.status_snapshot(b,at);};
        Policy policy;policy.primary=6;policy.secondary=3;policy.campaigns=255;policy.learned[180]=policy.learned[281]=1;Slots bar{};bar[0]={180,0};bar[1]={281,0};attributes::Ranks ranks{};Runtime runtime(2);
        check(runtime.start(now,{180,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);now=1100;runtime.update(now,h);
        check(w.has_enchantment(b,2,now)&&w.enchantment_removal(a,b,*enchantment_removal(69,0),20,1200));
        now=2000;runtime.update(now,h);check(runtime.start(now,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        now=3000;runtime.update(now,h);check(heals==1);
    }
    // Real casts include normal recharge, failure/cancel/control and scoped
    // replacement checks. A known unenchanted NPC never supplies free gains.
    for(unsigned skill:{68u,69u})for(unsigned mode:{0u,1u,2u,3u,4u,5u}){
        using namespace casting;world::Instance w;auto box=std::make_shared<world::Outbox>();w.configure_npcs(population.for_map(146));
        for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,40,100},100);}
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(1,5,0,100,{80,-2}));
        if(mode!=4)check(w.status_mutation(b,180,1,12,100,60100,true));
        std::uint64_t now=100;Hooks h;std::vector<Packet> sent;
        h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy=[&](auto n){return w.consume_energy(1,n);};h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(a,id,range,now);};
        h.enchantment_removal=[&](auto target,const auto& plan,auto at){return w.enchantment_removal(a,target,plan,20,at);};
        Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[skill]=1;Slots slots{};slots[0]={skill,0};attributes::Ranks ranks{};ranks[2]=ranks[3]=12;Runtime runtime(1);
        check(runtime.start(now,{skill,0,1},slots,policy,ranks,true,h)==casting::Reason::target);
        const auto victim=mode==5?100u:2u;
        check(runtime.start(now,{skill,0,victim},slots,policy,ranks,true,h)==casting::Reason::accepted);
        if(mode==1)runtime.cancel(h);
        if(mode==2)runtime.interrupt(500,h);
        if(mode==3){w.leave(2);w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,1000,40,100},100);}
        now=100+static_cast<std::uint64_t>(find_skill(skill)->activation*1000);runtime.update(now,h);
        const bool applied=mode==0;const auto plan=*enchantment_removal(skill,12);
        check(w.resources(2)->health==500-(applied?plan.damage:0));
        check(w.resources(1)->health==500+(applied?plan.health:0)&&w.resources(1)->energy==40-find_skill(skill)->energy_cost()+(applied?plan.energy:0));
        if(!mode||mode>=4)check(std::find(sent.begin(),sent.end(),slot_event(228,1,{skill,0},20))!=sent.end());
        if(!mode){auto out=box->drain();check(std::count(out.begin(),out.end(),*particles::targeted_body_2022(2,1,skill))==1);}
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

