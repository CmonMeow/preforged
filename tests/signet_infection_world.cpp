#include "gw_npc_catalog.h"
#include "gw_condition_spells.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
using namespace fw::gw;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main()try{
    using combat::conditions::Kind;
    const auto plan=*skills::effects::signet_infection_condition(7);
    auto players=std::make_unique<world::Instance>();
    players->join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    players->join(2,"Foe",150,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    players->set_vitals(1,{100,100,40,40},100);players->set_vitals(2,{100,100,40,40},100);
    const auto caster=*players->effect_scope(1),foe=*players->effect_scope(2);
    check(players->condition_spell(caster,foe,plan,100),"unbled foe cast completes");
    check(!players->has_condition(foe,Kind::disease,100),"unbled foe gets no Disease");
    check(players->apply_condition(caster,foe,Kind::bleeding,200,3000),"set Bleeding");
    check(players->condition_spell(caster,foe,plan,300),"bled foe cast completes");
    check(players->has_condition(foe,Kind::disease,300),"Disease applied");
    check(!players->has_condition(foe,Kind::disease,20300),"Disease expires");
    auto stale=foe;++stale.generation;
    check(!players->condition_spell(caster,stale,plan,300),"stale target denied");
    auto immune_player=std::make_unique<world::Instance>();
    immune_player->join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    immune_player->join(2,"Foe",150,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    immune_player->set_vitals(1,{100,100,40,40},100);immune_player->set_vitals(2,{100,100,40,40},100);
    const auto immune_source=*immune_player->effect_scope(1),immune_target=*immune_player->effect_scope(2);
    check(immune_player->apply_condition(immune_source,immune_target,Kind::bleeding,100,3000),"immune foe Bleeding");
    check(immune_player->tainted_flesh(immune_target,immune_target,*skills::effects::tainted_flesh(12),100),"owned Disease immunity");
    check(immune_player->condition_spell(immune_source,immune_target,plan,200),"immune player cast completes");
    check(!immune_player->has_condition(immune_target,Kind::disease,200),"immune player gets no Disease");
    auto paid=std::make_unique<world::Instance>();
    paid->join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    paid->join(2,"Foe",150,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    paid->set_vitals(1,{100,100,40,40},100);paid->set_vitals(2,{100,100,40,40},100);
    const auto paid_caster=*paid->effect_scope(1),paid_target=*paid->effect_scope(2);
    check(paid->apply_condition(paid_caster,paid_target,Kind::bleeding,100,3000),"paid target Bleeding");
    skills::casting::Hooks hooks;std::vector<skills::Packet> packets;unsigned title_rank=7,applications=0;
    hooks.events=[&](const auto& output){packets.insert(packets.end(),output.begin(),output.end());};
    hooks.resources=[&](auto actor){return paid->resources(actor);};
    hooks.consume_energy_at=[&](auto energy,auto now){return !energy&&paid->consume_energy(1,energy,paid_caster,now);};
    hooks.target_scope=[&](auto actor){return paid->effect_scope(actor);};
    hooks.projectile_target_valid=[&](auto actor,float range){return paid->projectile_target_valid(paid_caster,actor,range,100);};
    hooks.ebon_vanguard_rank=[&]()->std::optional<unsigned>{return title_rank;};
    hooks.condition_spell=[&](auto recipient,const auto& effect,auto now){
        ++applications;check(effect.skill==2229&&effect.rank==7,"cast uses admission title rank");
        return paid->condition_spell(paid_caster,recipient,effect,now);
    };
    skills::Policy policy;policy.primary=6;policy.campaigns=0x1e;policy.learned[2229]=1;
    skills::Slots bar{};bar[0]={2229,0};attributes::Ranks ranks{};
    auto missing_hook=hooks;missing_hook.ebon_vanguard_rank={};
    skills::casting::Runtime rejected(1);
    check(rejected.start(100,{2229,0,2},bar,policy,ranks,true,missing_hook)==skills::casting::Reason::unsupported,"missing title authority denied");
    title_rank=11;
    check(rejected.start(100,{2229,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::unsupported,"invalid title rank denied");
    title_rank=7;
    skills::casting::Runtime runtime(1);
    check(runtime.start(100,{2229,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted,"paid cast admitted");
    title_rank=10;runtime.update(1099,hooks);check(!applications,"effect waits for completion");
    runtime.update(1100,hooks);check(applications==1&&paid->has_condition(paid_target,Kind::disease,1100),"paid cast applies Disease");
    check(runtime.start(1850,{2229,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::recharge,"recharge blocks second cast");
    check(std::count(packets.begin(),packets.end(),skills::casting::generic(21,1,1945))==1,"native caster body once");
    check(std::count(packets.begin(),packets.end(),skills::casting::slot_event(228,1,{2229,0},10))==1,"native recharge packet");
    const std::string hash(64,'a');
    std::istringstream placements("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 150 0 0 - -\n");
    std::istringstream definitions("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/signet_infection_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 80 1 1 test test test - test 60 60 test test\n");
    auto population=world::load_npc_catalog(placements);
    world::bind_combat_catalog(population,combat::npcs::load_catalog(definitions,hash),hash);
    auto& flesh=*population.maps.at(146).front().combat_definition;
    flesh.fleshy=true;flesh.fleshy_evidence="test fixture";
    flesh.disease_immune=false;flesh.disease_immunity_evidence="test fixture";
    auto npcs=std::make_unique<world::Instance>();npcs->configure_npcs(population.for_map(146));
    npcs->join(1,"Caster",0,0,std::make_shared<world::Outbox>());npcs->set_vitals(1,{100,100,40,40},100);
    const auto source=*npcs->effect_scope(1),target=*npcs->effect_scope(100);
    check(npcs->condition_spell(source,target,plan,100),"unbled NPC cast completes");
    check(!npcs->has_condition(target,Kind::disease,100),"unbled NPC gets no Disease");
    check(npcs->condition_spell(source,target,*skills::effects::condition_spell(131,0),200),"bleed NPC");
    check(npcs->condition_spell(source,target,plan,300),"bled NPC cast completes");
    check(npcs->has_condition(target,Kind::disease,300),"NPC Disease applied");
    check(!npcs->has_condition(target,Kind::disease,20300),"NPC Disease expires");
    auto immune_population=population.for_map(146);
    immune_population.front().combat_definition->disease_immune=true;
    auto immune_npc=std::make_unique<world::Instance>();immune_npc->configure_npcs(immune_population);
    immune_npc->join(1,"Caster",0,0,std::make_shared<world::Outbox>());immune_npc->set_vitals(1,{100,100,40,40},100);
    const auto npc_source=*immune_npc->effect_scope(1),npc_target=*immune_npc->effect_scope(100);
    check(immune_npc->condition_spell(npc_source,npc_target,*skills::effects::condition_spell(131,0),100),"immune NPC Bleeding");
    check(immune_npc->condition_spell(npc_source,npc_target,plan,200),"immune NPC cast completes");
    check(!immune_npc->has_condition(npc_target,Kind::disease,200),"immune NPC gets no Disease");
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
