#include "gw_npc_catalog.h"
#include "gw_faintheartedness.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace combat::npcs;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const std::string hash(64,'a');
    std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
    auto population=load_npc_catalog(appearance);
    std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+
        " tests/faintheartedness_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 1 test test test - test 60 60 test test\n");
    const auto catalog=load_catalog(metadata,hash);bind_combat_catalog(population,catalog,hash);
    const auto plan=*skills::effects::faintheartedness(12);
    LiveActor live(*catalog.find(146,100),{1,1,1,146,100});const auto life=live.lifetime();
    check(live.apply_faintheartedness(life,plan,2,1,100).has_value(),"NPC hex apply");
    check(live.attack_duration(life,2000,100)==3000,"NPC cadence not slowed");
    auto update=live.advance_conditions(life,1100);check(update&&update->after.current==96&&update->cause==2,"NPC degeneration missing");
    update=live.advance_conditions(life,14100);check(update&&update->after.current==48,"late tick lost expiry interval");
    check(live.attack_duration(life,2000,13100)==2000,"expired slow retained");
    check(live.apply_faintheartedness(life,*skills::effects::faintheartedness(0),2,2,14100).has_value(),"rank zero NPC hex");
    check(live.attack_duration(life,2000,14100)==3000,"rank zero NPC slow missing");
    check(live.clear_hexes(life,15100)&&live.attack_duration(life,2000,15100)==2000,"clear hex retained slowdown");
    auto instance=std::make_unique<Instance>();auto box=std::make_shared<Outbox>();
    instance->configure_npcs(population.for_map(146));instance->join(2,"Necromancer",0,0,box);instance->set_vitals(2,{100,100,20,20},100);
    const auto caster=*instance->effect_scope(2),victim=*instance->effect_scope(100);
    check(instance->faintheartedness(caster,victim,plan,100),"world NPC hex rejected");
    instance->tick(1100);check(instance->npc_combat_snapshot(victim)->health->current==96,"world NPC degeneration missing");
    instance->tick(14100);check(instance->npc_combat_snapshot(victim)->health->current==48,"world expiry removed history early");
    auto players=std::make_unique<Instance>();players->join(1,"Caster",0,0,box,0,{}, {},100,1);
    players->join(2,"Foe",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
    players->set_vitals(1,{100,100,20,20},100);players->set_vitals(2,{100,100,20,20},100);
    const auto p1=*players->effect_scope(1),p2=*players->effect_scope(2);
    check(players->faintheartedness(p1,p2,plan,100)&&players->attack_duration(p2,500,100)==750,"player application/cadence");
    players->tick(1100);check(players->resources(2)->health==96,"player degeneration");
    check(players->faintheartedness(p1,p2,*skills::effects::faintheartedness(0),1100),"player replacement");
    players->tick(4100);check(players->resources(2)->health==96&&players->attack_duration(p2,500,4100)==500,"replacement/expiry");
    check(!players->faintheartedness(p1,p1,plan,4100),"friendly target admitted");
    for(unsigned id:{31u,859u}){
        auto hex_world=std::make_unique<Instance>();hex_world->configure_npcs(population.for_map(146));
        hex_world->join(2,"Mesmer",0,0,box);hex_world->set_vitals(2,{100,100,20,20},100);
        const auto source=*hex_world->effect_scope(2),foe=*hex_world->effect_scope(100);
        const auto hex=*skills::effects::degeneration_hex(id,0);auto stale=foe;++stale.generation;
        check(!hex_world->degeneration_hex(source,stale,hex,100),"stale NPC hex lifetime accepted");
        auto forged=hex;++forged.pips;check(!hex_world->degeneration_hex(source,foe,forged,100),"forged NPC hex pips accepted");
        check(hex_world->degeneration_hex(source,foe,hex,100),"plain NPC degeneration rejected");
        hex_world->tick(1100);check(hex_world->npc_combat_snapshot(foe)->health->current==100-2*hex.pips,"NPC hex pips missing");
        hex_world->tick(5100);check(hex_world->npc_combat_snapshot(foe)->health->current==100-4*hex.pips,"late NPC hex expiry lost history");
        check(!(hex_world->npc_combat_snapshot(foe)->effects&0x800),"expired NPC hex icon persisted");
    }
    {
        LiveActor stacked(*catalog.find(146,100),{1,1,1,146,100});const auto life=stacked.lifetime();
        check(bool(stacked.apply_single_degeneration_hex(life,*skills::effects::degeneration_hex(31,0),2,1,100)),"Conjure apply failed");
        check(bool(stacked.apply_single_degeneration_hex(life,*skills::effects::degeneration_hex(859,0),2,2,100)),"Nightmare apply failed");
        check(stacked.advance_conditions(life,1100)->after.current==80,"combined NPC hex degeneration cap not applied");
        check(bool(stacked.apply_single_degeneration_hex(life,*skills::effects::degeneration_hex(31,0),2,3,1100)),"Conjure renewal failed");
        check(stacked.advance_conditions(life,2100)->after.current==60,"renewed Conjure stacked or erased Nightmare");
        check(stacked.clear_hexes(life,2100),"NPC hex cleansing failed");
        check(stacked.advance_conditions(life,3100)->after.current==60,"removed NPC hex continued degeneration");
    }
    std::cout<<"Faintheartedness NPC/player degeneration, expiry, rank zero, and cadence verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
