#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace world;using namespace combat::npcs;
void check(bool value,std::source_location where=std::source_location::current()){
    if(!value)throw std::runtime_error("NPC Life Siphon regression line "+std::to_string(where.line()));
}
int main()try{
    const std::string hash(64,'a');
    std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
    auto population=load_npc_catalog(appearance);
    std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+
        " tests/npc_life_siphon.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 1 test test test - test 60 60 test test\n");
    const auto catalog=load_catalog(metadata,hash);bind_combat_catalog(population,catalog,hash);
    const auto fixture=[&](){auto world=std::make_unique<Instance>();world->configure_npcs(population.for_map(146));
        world->join(1,"Necromancer",0,0,std::make_shared<Outbox>());world->set_vitals(1,{50,200,20,20},100);return world;};
    const auto plan=*skills::effects::degeneration_hex(109,0);
    {
        auto world=fixture();const auto source=*world->effect_scope(1),target=*world->effect_scope(100);
        check(world->degeneration_hex(source,target,plan,100));world->tick(1100);
        check(world->resources(1)->health==52&&world->npc_combat_snapshot(target)->health->current==98);
        check(world->degeneration_hex(source,target,plan,1100));world->tick(2100);
        check(world->resources(1)->health==54&&world->npc_combat_snapshot(target)->health->current==96);
        check(world->damage_npc(source,target,{combat::Kind::armor_ignoring,100,20,27},1248,2100).killed);
        world->tick(3100);check(world->resources(1)->health==54&&world->take_npc_deaths(source).size()==1);
    }
    {
        auto world=fixture();const auto source=*world->effect_scope(1),target=*world->effect_scope(100);
        check(world->degeneration_hex(source,target,plan,100));world->tick(20100);
        check(world->resources(1)->health==74&&world->npc_combat_snapshot(target)->health->current==76);
        world->tick(21100);check(world->resources(1)->health==74&&world->npc_combat_snapshot(target)->health->current==76);
    }
    {
        auto world=fixture();world->join(2,"Replacement",0,0,std::make_shared<Outbox>());world->set_vitals(2,{50,200,20,20},100);
        const auto first=*world->effect_scope(1),second=*world->effect_scope(2),target=*world->effect_scope(100);
        check(world->degeneration_hex(first,target,plan,100));
        check(world->degeneration_hex(second,target,plan,1100));world->tick(2100);
        check(world->resources(1)->health==52&&world->resources(2)->health==52&&world->npc_combat_snapshot(target)->health->current==96);
        world->leave(2);world->tick(3100);check(world->npc_combat_snapshot(target)->health->current==96);
    }
    {
        auto world=fixture();const auto source=*world->effect_scope(1),target=*world->effect_scope(100);
        check(world->degeneration_hex(source,target,*skills::effects::degeneration_hex(109,20),100));world->tick(20100);
        check(world->npc_combat_snapshot(target)->health->current==0&&world->resources(1)->health==150);
        const auto deaths=world->take_npc_deaths(source);
        check(deaths.size()==1&&deaths.front().time<20100);
        world->tick(21100);check(world->resources(1)->health==150&&world->take_npc_deaths(source).empty());
    }
    {
        auto world=fixture();const auto source=*world->effect_scope(1),target=*world->effect_scope(100);
        check(world->degeneration_hex(source,target,*skills::effects::degeneration_hex(109,20),100));
        check(!world->degeneration_hex(source,target,plan,20100));
        check(world->resources(1)->health==150&&world->npc_combat_snapshot(target)->health->current==0);
    }
    {
        auto low=population.for_map(146);low.front().combat_definition->spawn_health=combat::Health{20,100};
        auto world=std::make_unique<Instance>();world->configure_npcs(low);
        world->join(1,"Necromancer",0,0,std::make_shared<Outbox>());world->set_vitals(1,{50,200,20,20},100);
        const auto source=*world->effect_scope(1),target=*world->effect_scope(100);
        check(world->degeneration_hex(source,target,plan,100));world->tick(20100);
        // Rank0 drains two health/second: death after10 seconds, even when
        // observed later. Timed quest/loot credit must retain that event time.
        const auto deaths=world->take_npc_deaths(source);check(deaths.size()==1&&deaths.front().time==10100);
        world->tick(21100);check(world->take_npc_deaths(source).empty());
    }
    {
        auto durable=population.for_map(146);durable.front().combat_definition->spawn_health=combat::Health{1000,1000};
        auto world=std::make_unique<Instance>();world->configure_npcs(durable);
        world->join(1,"Necromancer",0,0,std::make_shared<Outbox>());world->set_vitals(1,{50,200,20,20},100);
        const auto source=*world->effect_scope(1),target=*world->effect_scope(100);
        const auto long_plan=*skills::effects::degeneration_hex(109,20);
        check(world->degeneration_hex(source,target,long_plan,100));world->move(1,6500,0,100);world->tick(24100);
        const auto health=world->npc_combat_snapshot(target)->health->current;check(health>0);
        world->tick(25100);check(world->npc_combat_snapshot(target)->health->current==health);
    }
    std::cout<<"NPC Life Siphon linked healing, renewal, expiry, caster departure and death cleanup verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
