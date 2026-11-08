#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <sstream>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Brood quest World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope owner,source;std::vector<world::Instance::Npc> templates;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(){
        const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw/quests";
        std::ifstream appearance(root/"actors/domination-magic-brood.txt");check(bool(appearance));
        auto population=world::load_npc_catalog(appearance);
        std::ifstream header(root/"domination-magic-development-combat.txt");std::string magic,hash;unsigned version=0;
        header>>magic>>version>>hash;check(magic=="gw-npc-combat"&&version==11);
        std::ifstream metadata(root/"domination-magic-development-combat.txt");
        world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
        templates=population.for_map(161);check(templates.size()==1);join();
        check(w.spawn_kill_encounter(owner,49,1,templates,[]{return true;}));source=*w.effect_scope(60000);
        check(w.arm_brood_caster(owner,1,1000));check(w.arm_brood_caster(owner,1,1000));
    }
    void join(){w.join(1,"Mesmer quest owner",templates.front().x+100,templates.front().y,box,0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,40,40},100);owner=*w.effect_scope(1);check(w.elemental_armor(owner,60,60));}
    void hurt(std::uint64_t now){check(w.damage_npc(owner,source,{combat::Kind::armor_ignoring,1,20,194},1248,now).reason==combat::Reason::accepted);}
    double health(){return w.npc_combat_snapshot(source)->health->current;}
};
int main()try{
    {Fixture f;f.w.tick(1000);f.hurt(1100);
        check(f.w.grenths_balance(f.owner,f.source,1101));check(f.health()==100);
        f.w.tick(1101);check(f.w.npc_energy_snapshot(f.source)->current==35);
        f.w.tick(1351);check(f.w.npc_aura_snapshot(f.source,1351).has_value());}
    {Fixture f;f.w.tick(1000);check(!f.w.npc_aura_snapshot(f.source,1000));check(!f.w.npc_elemental_ai_snapshot(f.source)->pending);
        f.hurt(1100);f.w.tick(1100);check(f.w.npc_energy_snapshot(f.source)->current==35);check(f.health()==99);
        f.w.tick(1350);check(f.w.npc_aura_snapshot(f.source,1350).has_value());check(f.health()==99);
        check(f.w.backfire(f.owner,f.source,*skills::effects::backfire(28,0),20,1500));
        f.w.tick(2100);const auto pending=f.w.npc_elemental_ai_snapshot(f.source)->pending;check(pending&&pending->use.slot.skill==214);
        check(pending->token.completes==3100);f.w.tick(3100);
        check(f.health()==74); //99 minus35 Backfire plus10 Aura, no first-Aura heal
        check(f.w.npc_elemental_ai_snapshot(f.source)->flights==1);f.w.tick(3200);check(f.w.resources(1)->health<1000);
        check(!f.w.arm_brood_caster(f.owner,2,3200));
        check(f.w.clear_kill_objective(f.owner,49,1,true));f.w.tick(3400);
        check(!f.w.npc_elemental_ai_snapshot(f.source));check(!f.w.effect_scope(f.source.actor));
        check(f.w.spawn_kill_encounter(f.owner,49,2,f.templates,[]{return true;}));const auto fresh=*f.w.effect_scope(60001);
        check(fresh!=f.source);check(f.w.arm_brood_caster(f.owner,2,3500));f.w.tick(3500);
        check(!f.w.npc_aura_snapshot(fresh,3500));check(!f.w.npc_elemental_ai_snapshot(fresh)->pending);}
    {Fixture f;f.hurt(1100);f.w.tick(1100);const auto old=f.owner;f.w.leave(1);f.join();check(f.owner!=old);
        f.w.tick(1500);check(!f.w.effect_scope(f.source.actor));check(!f.w.npc_aura_snapshot(f.source,1500));
        check(!f.w.arm_brood_caster(old,1,1500));check(f.w.spawn_kill_encounter(f.owner,49,1,f.templates,[]{return true;}));
        const auto fresh=*f.w.effect_scope(60001);check(f.w.arm_brood_caster(f.owner,1,1600));f.w.tick(1600);
        check(!f.w.npc_aura_snapshot(fresh,1600));check(f.w.resources(1)->health==1000);}
    std::cout<<"Existing quest49 Brood Aura, Ice Spear, Backfire and restoration passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
