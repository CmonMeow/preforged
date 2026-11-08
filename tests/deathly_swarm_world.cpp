#include "gw_npc_catalog.h"
#include "gw_deathly_swarm.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace combat::npcs;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const std::string hash(64,'a');
    std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
    auto population=load_npc_catalog(appearance);
    std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+
        " tests/deathly_swarm_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 30 30 60 1 0 test test test - - 60 60 test test\n");
    bind_combat_catalog(population,load_catalog(metadata,hash),hash);
    auto enemies=population.for_map(146);
    for(unsigned i=1;i<3;++i){auto copy=enemies.front();copy.id=100+i;copy.combat_definition->binding.actor=copy.id;copy.x=100+322.f*i;enemies.push_back(copy);}
    auto instance=std::make_unique<Instance>();auto box=std::make_shared<Outbox>();
    instance->join(2,"Necromancer",0,0,box);instance->set_vitals(2,{100,100,20,20},100);
    const auto caster=*instance->effect_scope(2);const auto plan=*skills::effects::deathly_swarm(0);
    check(instance->spawn_kill_encounter(caster,85,1,enemies,[]{return true;}),"encounter setup");
    const auto primary=*instance->effect_scope(60000),second=*instance->effect_scope(60001),third=*instance->effect_scope(60002);
    check(instance->deathly_swarm(caster,primary,plan,20,100),"launch rejected");box->drain();
    instance->tick(1000);check(instance->npc_combat_snapshot(primary)->health->current==30,"early damage");
    instance->tick(1100);check(instance->npc_combat_snapshot(primary)->health->current==0&&instance->npc_combat_snapshot(second)->health->current==30,"first hit");
    instance->tick(2100);check(instance->npc_combat_snapshot(second)->health->current==0&&instance->npc_combat_snapshot(third)->health->current==30,"second hit from corpse");
    instance->tick(3100);check(instance->npc_combat_snapshot(third)->health->current==0,"third hit from secondary");
    check(instance->pending_quest_completions(caster)==std::vector<Instance::QuestCompletion>{{85,1}},"quest completion missing");
    check(instance->take_npc_deaths(caster).size()==3,"death attribution lost");
    instance->tick(4100);check(instance->take_npc_deaths(caster).empty(),"repeated death");
    const auto packets=box->drain();unsigned particles=0;
    for(const auto& p:packets)if(p.size()==14&&p[0]==158&&p[2]==21&&p[10]==201)++particles;
    check(particles==3,"impact particles missing");
    auto stale=std::make_unique<Instance>();stale->configure_npcs(enemies);stale->join(2,"Old",0,0,box);stale->set_vitals(2,{100,100,20,20},100);
    const auto old=*stale->effect_scope(2),victim=*stale->effect_scope(100);
    check(stale->deathly_swarm(old,victim,plan,20,100),"stale setup");stale->leave(2);stale->join(2,"Replacement",0,0,box);stale->set_vitals(2,{100,100,20,20},100);
    stale->tick(3100);check(stale->npc_combat_snapshot(victim)->health->current==30,"recycled caster damage");
    auto unknown=enemies;unknown[0].combat_definition->cold_armor.reset();auto missing=std::make_unique<Instance>();
    missing->configure_npcs(unknown);missing->join(2,"Necromancer",0,0,box);missing->set_vitals(2,{100,100,20,20},100);
    check(!missing->deathly_swarm(*missing->effect_scope(2),*missing->effect_scope(100),plan,20,100),"unknown cold armor accepted");
    auto players=std::make_unique<Instance>();players->join(1,"Caster",0,0,box,0,{}, {},100,1);
    players->join(2,"Foe",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
    players->join(3,"Ally",101,0,std::make_shared<Outbox>(),0,{}, {},100,1);
    for(unsigned id:{1u,2u,3u})players->set_vitals(id,{100,100,20,20},100);
    const auto p1=*players->effect_scope(1),p2=*players->effect_scope(2);
    check(players->elemental_armor(p2,60,100)&&players->deathly_swarm(p1,p2,plan,20,100),"player launch");
    players->tick(3100);check(players->resources(2)->health==85&&players->resources(3)->health==100,"cold mitigation/ally exclusion/distinct targets");
    check(players->deathly_swarm(p1,p2,plan,20,3200),"replacement target setup");
    players->leave(2);players->join(2,"Reused ID",100,0,std::make_shared<Outbox>(),0,{}, {},3200,2);
    players->set_vitals(2,{100,100,20,20},100);players->elemental_armor(*players->effect_scope(2),60,60);
    players->tick(6200);check(players->resources(2)->health==100,"recycled target inherited swarm");
    std::cout<<"Deathly Swarm world cold damage, delayed chains, particles, quest deaths and stale caster verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

