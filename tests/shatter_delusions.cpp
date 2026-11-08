#include "gw_npc_catalog.h"
#include "gw_shatter_delusions.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace world;using namespace combat::npcs;
void check(bool value,std::source_location where=std::source_location::current()){
    if(!value)throw std::runtime_error("Shatter Delusions regression line "+std::to_string(where.line()));
}
int main()try{
    using namespace skills::effects;
    check(shatter_delusions(0)->damage==15&&shatter_delusions(12)->damage==63&&shatter_delusions(15)->damage==75&&!shatter_delusions(21));
    for(unsigned skill:{27u,67u,302u})for(bool reversal:{false,true}){
        Instance w;std::vector<Instance::Npc> npcs;
        for(unsigned id:{100u,101u}){Instance::Npc n;n.id=n.definition.id=id;n.x=100;n.definition.level=20;
            n.definition.visual_adjustment={0,0,0,100};Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
            d.relation=Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
            d.healing_allowed=true;d.healing_evidence="authored removal conversion";
            d.holy_traits.light_sensitive=id==101;d.light_sensitive_evidence="authored light sensitivity";
            n.combat_definition=d;npcs.push_back(n);}
        w.configure_npcs(npcs);
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?100:1000,0,std::make_shared<Outbox>(),0,{}, {},100,id==3?2:1);
            w.set_vitals(id,{1000,1000,100,100},100);}
        const auto source=*w.effect_scope(1),npc=*w.effect_scope(100),ally=*w.effect_scope(2),enemy=*w.effect_scope(3);
        const auto plan=skill==27?*shatter_delusions(12):skill==67?*shatter_hex(12):*smite_hex(12);
        const auto primary=skill==27?npc:ally;
        check(w.degeneration_hex(skill==27?source:enemy,primary,*degeneration_hex(31,0),100));
        for(unsigned id:{100u,101u}){const auto target=*w.effect_scope(id);
            check(w.stage_npc_shield_absorption(target,12,100));if(reversal)check(w.stage_npc_reversal(target,12,100));}
        check(w.shatter_delusions(source,primary,plan,20,100));
        for(unsigned id:{100u,101u}){const auto target=*w.effect_scope(id);
            const auto damage=double(plan.damage)*(skill==302&&id==101?2:1)-5;
            const auto after=1000-std::max(0.,damage-(reversal?reversal_fortune(12)->maximum_healing:0));
            check(w.npc_combat_snapshot(target)->health->current==after);
            check(w.damage_npc(source,target,{combat::Kind::armor_ignoring,50,20,27},1248,101).reason==combat::Reason::accepted);
            check(w.npc_combat_snapshot(target)->health->current==after-40);
        }
    }
    // Independent primary/adjacent protection counters and Reversal consumption.
    for(unsigned skill:{27u,67u,302u})for(bool reversal:{false,true}){
        Instance w;auto box=std::make_shared<Outbox>();
        for(unsigned id:{1u,2u,3u,4u}){
            w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<Outbox>(),0,{}, {},100,id==1||id==4?1:2);
            w.set_vitals(id,{1000,1000,100,100},100);
            check(w.player_holy_equipment(*w.effect_scope(id),0));
        }
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2),near=*w.effect_scope(3),ally=*w.effect_scope(4);
        const auto primary=skill==27?target:ally;
        const auto plan=skill==27?*shatter_delusions(12):skill==67?*shatter_hex(12):*smite_hex(12);
        check(w.degeneration_hex(skill==27?source:target,primary,*degeneration_hex(31,0),100));
        for(auto recipient:{target,near}){
            check(w.status_mutation(recipient,1399,80,12,100,6100,true));
            if(reversal)check(w.status_mutation(recipient,307,81,0,100,8100,true));
        }
        check(w.shatter_delusions(source,primary,plan,20,100));
        const unsigned loss=unsigned(std::max(0.,double(plan.damage)-5-(reversal?15:0)));
        check(w.resources(2)->health==1000-loss&&w.resources(3)->health==1000-loss);
        // Next damage must see a separate second absorption hit on each actor.
        for(auto recipient:{target,near}){
            check(w.energy_drain(source,recipient,*energy_drain(42,12),20,101));
            check(w.resources(recipient.actor)->health==1000-loss-(energy_drain(42,12)->amount*energy_drain(42,12)->damage_per_energy-10));
            const auto active=*w.status_snapshot(recipient,101);
            check(std::none_of(active.begin(),active.end(),[](auto entry){return entry.skill==307;}));
        }
    }
    {
        // First recipient's Seed heals the later recipient before its hit.
        // Restoring the admission Health would erase that real healing.
        Instance w;
        for(unsigned id:{1u,2u,3u}){
            w.join(id,"Actor",id==1?0:100,0,std::make_shared<Outbox>(),0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{id==1?1000u:500u,1000,100,100},100);
        }
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2),monk=*w.effect_scope(3);
        check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
        const auto seed=*healing_seed(12);check(w.complete_healing_seed(monk,target,seed,0,100));
        for(auto recipient:{target,monk})check(w.status_mutation(recipient,1399,80,12,100,6100,true));
        check(w.degeneration_hex(source,target,*degeneration_hex(31,0),100));
        check(w.shatter_delusions(source,target,*shatter_delusions(12),20,100));
        check(w.resources(2)->health==500-63+5+seed.healing);
        check(w.resources(3)->health==500-63+5+seed.healing);
    }
    Scope scope{1,10,1};Registry registry(scope);
    check(registry.apply(scope,{1,2,31,0,Category::hex,100,2100},Replace::same_skill).accepted);
    check(registry.apply(scope,{2,3,26,0,Category::hex,100,5100},Replace::same_skill).accepted);
    check(registry.apply(scope,{3,4,135,0,Category::hex,100,3100},Replace::same_skill).accepted);
    check(registry.newest(scope,Category::hex,100,5)->instance==2); // application order, not timestamp or caster
    const std::string hash(64,'a');std::ostringstream appearance,metadata;
    appearance<<"gw-npcs 1\n";metadata<<"gw-npc-combat 2 "<<hash<<"\nsource test authored "<<hash<<" tests/shatter_delusions.cpp\n";
    for(unsigned id=100;id<=102;++id){const double x=id==100?100:id==101?266:266.1;
        appearance<<"npc 146 "<<id<<" 1 1234 0 1677721600 0 8 1 1 0 "<<x<<" 0 0 - -\n";
        metadata<<"npc 146 "<<id<<" 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 1 test test test - test 60 60 test test\n";
    }
    std::istringstream input(appearance.str()),meta(metadata.str());auto population=load_npc_catalog(input);
    const auto catalog=load_catalog(meta,hash);bind_combat_catalog(population,catalog,hash);
    for(const bool hexed:{false,true}){
        auto world=std::make_unique<Instance>();auto box=std::make_shared<Outbox>();world->configure_npcs(population.for_map(146));
        world->join(1,"Mesmer",0,0,box);world->set_vitals(1,{100,100,20,20},100);
        if(hexed){world->join(2,"Bonded",500,0,box,0,{}, {},100,2);world->set_vitals(2,{100,100,20,20},100);}
        const auto source=*world->effect_scope(1),target=*world->effect_scope(100);
        if(hexed){const auto bonded=*world->effect_scope(2);check(world->elemental_armor(bonded,60,60));check(world->stage_npc_bonds_of_torment(target,bonded,*bonds_of_torment_npc(),100));}
        if(hexed)check(world->degeneration_hex(source,target,*degeneration_hex(31,0),100));
        auto stale=target;++stale.generation;check(!world->shatter_delusions(source,stale,*shatter_delusions(12),20,100));
        auto forged=*shatter_delusions(12);++forged.damage;check(!world->shatter_delusions(source,target,forged,20,100));
        check(world->shatter_delusions(source,target,*shatter_delusions(12),20,100));
        check(world->npc_combat_snapshot(target)->health->current==(hexed?37:100));
        check(world->npc_combat_snapshot(*world->effect_scope(101))->health->current==(hexed?37:100));
        check(world->npc_combat_snapshot(*world->effect_scope(102))->health->current==100);
        if(hexed)check(world->resources(2)->health==77); // 63 cold from level eight loses 23 after armor.
        world->tick(1100);check(world->npc_combat_snapshot(target)->health->current==(hexed?37:100));
        check(!(world->npc_combat_snapshot(target)->effects&0x800));
    }
    {
        auto world=std::make_unique<Instance>();auto box=std::make_shared<Outbox>();world->configure_npcs(population.for_map(146));
        world->join(1,"Mesmer",0,0,box);world->set_vitals(1,{100,100,20,20},100);
        const auto source=*world->effect_scope(1),target=*world->effect_scope(100);
        check(world->degeneration_hex(source,target,*degeneration_hex(31,0),100));
        check(world->empathy_npc(source,target,26,0,100));
        check(world->faintheartedness(source,target,*faintheartedness(12),100));
        check(world->shatter_delusions(source,target,*shatter_delusions(0),20,100));
        world->tick(1100);check(world->npc_combat_snapshot(target)->health->current==71);
        check(world->shatter_delusions(source,target,*shatter_delusions(0),20,1100));
        world->tick(2100);check(world->npc_combat_snapshot(target)->health->current==52);
        check(world->shatter_delusions(source,target,*shatter_delusions(0),20,2100));
        check(world->npc_combat_snapshot(target)->health->current==52); // only Necromancer hex remains
    }
    {
        auto world=std::make_unique<Instance>();auto box=std::make_shared<Outbox>();
        world->join(1,"Mesmer",0,0,box,0,{}, {},100,1);world->join(2,"Foe",100,0,box,0,{}, {},100,2);
        world->join(3,"Adjacent",266,0,box,0,{}, {},100,2);world->join(4,"Ally",100,0,box,0,{}, {},100,1);
        for(unsigned id=1;id<=4;++id)world->set_vitals(id,{100,100,20,20},100);
        const auto source=*world->effect_scope(1),target=*world->effect_scope(2);
        check(world->degeneration_hex(source,target,*degeneration_hex(31,0),100));
        check(world->shatter_delusions(source,target,*shatter_delusions(12),20,100));
        check(world->resources(2)->health==37&&world->resources(3)->health==37&&world->resources(4)->health==100);
        world->tick(1100);check(world->resources(2)->health==37);
    }
    std::cout<<"Shatter Delusions scoped removal, Mesmer selection, conditional adjacent damage, NPC/player health and expiry verified\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
