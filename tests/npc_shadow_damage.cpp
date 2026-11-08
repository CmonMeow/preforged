#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace combat::npcs;using namespace skills;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const std::string hash(64,'a');
    for(unsigned skill:{133u,155u,158u,1078u,102u,1067u})for(unsigned initial:{100u,50u,5u}){
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 1 "+hash+"\nsource test authored "+hash+" tests/npc_shadow_damage.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 "+std::to_string(initial)+" - 1 0 test test - - -\n");
        const auto catalog=load_catalog(metadata,hash);bind_combat_catalog(population,catalog,hash);
        Instance w;w.configure_npcs(population.for_map(146));w.join(1,"Necromancer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(1,{100,200,20,20},100);
        const auto source=*w.effect_scope(1),npc=*w.effect_scope(100);const auto plan=*effects::shadow_damage(skill,0);auto stale=npc;++stale.generation;
        check(!w.shadow_damage(source,stale,plan,20,100),"stale NPC lifetime");auto forged=plan;++forged.damage;check(!w.shadow_damage(source,npc,forged,20,100),"forged shadow plan");
        check(w.shadow_damage(source,npc,plan,20,100),"NPC shadow rejected");
        const bool steals=(skill==102||skill==1067)&&initial>50;
        const double expected=std::max(0.,double(initial)-plan.damage-(steals?plan.steal_above_half:0));
        const auto snapshot=w.npc_combat_snapshot(npc);check(snapshot&&snapshot->health->current==expected&&w.resources(1)->health==100+(steals?12:0),"shadow damage/conditional transfer");
        if(!expected){check(snapshot->effects&0x10,"dead NPC effects missing");check(!w.shadow_damage(source,npc,plan,20,100),"corpse damaged again");}
    }
    using namespace casting;Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned[102]=1;Slots bar{};bar[0]={102,0};attributes::Ranks ranks{};
    Hooks hooks;unsigned hits=0;effects::Scope npc{1,100,1};hooks.events=[](const auto&){};
    hooks.resources=[](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional<vitals::Resources>{{100,100,20,20}}:std::nullopt;};hooks.consume_energy=[](unsigned amount){return amount==10;};hooks.target_valid=[](auto,float,bool){return false;};
    hooks.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==100?std::optional(npc):std::nullopt;};hooks.projectile_target_valid=[](auto id,float range){return id==100&&range==1248;};
    hooks.shadow_damage=[&](auto target,const auto& hit,auto){check(target==npc&&hit.skill==102,"shadow NPC lifetime");++hits;return true;};
    Runtime cast(42);check(cast.start(100,{102,0,100},bar,policy,ranks,true,hooks)==casting::Reason::accepted,"shadow cast start");cast.update(2100,hooks);check(hits==1,"NPC shadow dispatcher gate");
    std::cout<<"Five shadow skills resolve NPC damage, conditional life steal and death without invented armor\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
