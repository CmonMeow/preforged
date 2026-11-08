// Owned quest enrollment uses the production metadata and resource policy.
#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Blood Fanatic owned immediate line "+std::to_string(at.line()));
}
world::Instance::Npc sample(bool resources){
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw/quests";
    std::ifstream appearance(root/"actors/power-of-blood-fanatics.txt");check(bool(appearance));
    auto population=world::load_npc_catalog(appearance);
    const auto path=root/"power-of-blood-development-combat.txt";
    std::ifstream header(path);std::string magic,hash;unsigned version;header>>magic>>version>>hash;
    check(magic=="gw-npc-combat");std::ifstream metadata(path);
    world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
    auto actor=population.for_map(145).front();actor.x=actor.y=0;
    check(actor.combat_definition->spawn_health->maximum==120);
    check(actor.combat_definition->spawn_energy==combat::npcs::Energy{40,40});
    check(actor.combat_definition->energy_pips==4);
    if(!resources){actor.combat_definition->spawn_energy.reset();actor.combat_definition->energy_pips.reset();}
    return actor;
}
int main()try{
    // Unknown resources fail closed without creating a controller or actor.
    {
        world::Instance w;w.join(1,"Unknown owner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        const auto owner=*w.effect_scope(1);
        check(w.spawn_kill_encounter(owner,43,1,{sample(false)},[]{return true;}));
        const auto report=w.arm_blood_fanatic_immediate(owner,1,1000);
        check(report.owner_valid&&report.remaining==1&&report.armed==0&&report.unsupported==1);
        check(w.npc_ai_coverage().placed==1&&w.npc_ai_coverage().active_hostiles==0);
    }
    // A hit before the first host arming poll must not disappear.
    {
        world::Instance early;early.join(1,"Early pull",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        early.set_vitals(1,{1000,1000,40,40},100);const auto owner=*early.effect_scope(1);
        check(early.spawn_kill_encounter(owner,43,1,{sample(true)},[]{return true;}));
        const auto source=*early.effect_scope(60000);
        check(early.damage_npc(owner,source,{combat::Kind::armor_ignoring,1,20,194},1248,1000).reason==combat::Reason::accepted);
        check(early.arm_blood_fanatic_immediate(owner,1,1100).armed==1);
        early.tick(1100);check(early.npc_flare_ai_snapshot(source)->engaged);
    }
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    w.join(1,"Fanatic owner",500,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,40,40},100);
    const auto owner=*w.effect_scope(1);
    check(w.spawn_kill_encounter(owner,43,7,{sample(true)},[]{return true;}));
    const auto source=*w.effect_scope(60000);auto wrong=owner;++wrong.generation;
    check(!w.arm_blood_fanatic_immediate(wrong,7,1000).owner_valid);
    check(!w.arm_blood_fanatic_immediate(owner,8,1000).owner_valid);
    auto report=w.arm_blood_fanatic_immediate(owner,7,1000);
    check(report.owner_valid&&report.remaining==1&&report.armed==1&&!report.unsupported);
    check(w.npc_ai_coverage().placed==1&&w.npc_ai_coverage().wandering==1);
    // Generic quest exclusion remains strict, even after owned registration.
    auto policy=w.npc_flare_ai_snapshot(source)->policy;
    check(!w.configure_npc_immediate_ai(policy,{{{155,0},*skills::effects::shadow_damage(155,0),policy.evidence}},1000));
    check(!w.configure_npc_wander(source,{{{20,0}},100,2000,policy.evidence},1000));
    auto wrong_source=policy;++wrong_source.source.generation;
    check(!w.configure_npc_owned_immediate(owner,43,7,wrong_source,
        {{{155,0},*skills::effects::shadow_damage(155,0),policy.evidence}},{{{20,0}},100,2000,policy.evidence},1000));
    w.tick(1000);check(w.arm_blood_fanatic_immediate(owner,7,2000).armed==1);
    w.tick(2200);check(w.npc_pursuit_movement_snapshot(source)->outcome.deferral==combat::ai::motion::Deferral::zero_budget);
    w.tick(2300);check(w.npc_pursuit_movement_snapshot(source)->outcome.travelled>0);
    for(unsigned at=2400;at<=5000;at+=100)w.tick(at);
    check(!w.npc_flare_ai_snapshot(source)->engaged&&!w.npc_flare_ai_snapshot(source)->cast);
    check(w.resources(1)->health==1000&&w.npc_energy_snapshot(source)->current==40);
    check(w.damage_npc(owner,source,{combat::Kind::armor_ignoring,1,20,194},1248,5100).reason==combat::Reason::accepted);
    bool cast=false,fought=false;unsigned terminal=0;
    for(unsigned at=5100;at<=15000;at+=100){
        w.tick(at);const auto state=*w.npc_flare_ai_snapshot(source);
        if(state.cast){cast=true;check(state.cast->skill&&state.cast->skill->skill==155);
            const auto token=*state.cast;const auto energy=w.npc_energy_snapshot(source)->current;
            check(w.arm_blood_fanatic_immediate(owner,7,at).armed==1);
            check(w.npc_flare_ai_snapshot(source)->cast==token&&w.npc_energy_snapshot(source)->current==energy);
        }
        if(w.resources(1)->health<1000){fought=true;terminal=at;break;}
    }
    check(cast&&fought&&w.resources(1)->health==980);
    // Owner death drops target and returns; no reset of paid action authority.
    check(w.kill(owner,terminal+100));bool returning=false,returned=false;unsigned home=0;
    for(unsigned at=terminal+100;at<=terminal+15000;at+=100){
        w.tick(at);const auto state=*w.npc_flare_ai_snapshot(source);returning=returning||state.returning;
        if(returning&&!state.returning&&!state.engaged){returned=true;home=at;break;}
    }
    check(returned);bool resumed=false;
    for(unsigned at=home+100;at<=home+4000;at+=100){w.tick(at);
        const auto movement=w.npc_pursuit_movement_snapshot(source);
        resumed=resumed||(movement&&movement->at==at&&movement->outcome.travelled>0);
        check(!w.npc_flare_ai_snapshot(source)->engaged);
    }
    check(resumed);w.leave(1);w.tick(home+4100);
    check(w.arm_blood_fanatic_immediate(owner,7,home+4100).armed==0);
    check(w.npc_ai_coverage().wandering==0);
    w.join(1,"Replacement",500,0,std::make_shared<world::Outbox>(),0,{}, {},home+4200,1);
    w.set_vitals(1,{1000,1000,40,40},100);
    const auto replacement=*w.effect_scope(1);check(replacement!=owner);
    check(!w.arm_blood_fanatic_immediate(replacement,7,home+4200).owner_valid);
    check(w.spawn_kill_encounter(replacement,43,8,{sample(true)},[]{return true;}));
    const auto next=*w.effect_scope(60001);check(next!=source);
    check(w.arm_blood_fanatic_immediate(replacement,8,home+4300).armed==1);
    check(w.npc_energy_snapshot(next)->current==40&&!w.npc_flare_ai_snapshot(next)->engaged);
    const auto final_hit=w.damage_npc(replacement,next,{combat::Kind::armor_ignoring,120,20,194},1248,home+4400);
    check(final_hit.reason==combat::Reason::accepted);
    w.tick(home+4400);const auto exhausted=w.arm_blood_fanatic_immediate(replacement,8,home+4500);
    check(exhausted.owner_valid&&!exhausted.remaining&&!exhausted.armed);
    std::cout<<"Owned quest43 staged enrollment, idle, paid155, return, lifetime and refusal\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
