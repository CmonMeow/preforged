#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace combat::npcs;using namespace skills;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto plan=*effects::holy_damage(252,12);
    check(plan.damage==49&&!effects::holy_damage(252,21),"Banish scale");
    check(!effects::holy_strike(plan,{},20)&&!effects::holy_strike(plan,{false,{}},20),"unknown hidden trait treated as false");
    const std::string hash(64,'a');
    for(unsigned scenario=0;scenario<5;++scenario){
        const std::string light=scenario==4?"-":std::to_string(scenario&1),summoned=std::to_string((scenario>>1)&1);
        const std::string text="gw-npc-combat 9 "+hash+"\nsource test authored "+hash+
            " tests/holy_damage.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 - 1 0 test test - - -"
            " - - - - - - 1 - - - - - - - - - - "+light+" "+(scenario==4?"-":"test")+" "+summoned+" test\n";
        std::istringstream metadata(text);const auto catalog=load_catalog(metadata,hash);
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
        auto population=load_npc_catalog(appearance);bind_combat_catalog(population,catalog,hash);
        Instance w;w.configure_npcs(population.for_map(146));w.join(1,"Monk",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(1,{100,100,20,20},100);
        const auto source=*w.effect_scope(1),npc=*w.effect_scope(100);auto stale=npc;++stale.generation;
        check(!w.holy_damage(source,stale,plan,20,100),"stale target lifetime");
        auto forged=plan;++forged.damage;check(!w.holy_damage(source,npc,forged,20,100),"forged holy plan");
        check(w.holy_damage(source,npc,plan,20,100)==(scenario<4),"holy target policy");
        const auto expected=scenario==4?0:49*((scenario&1)?2:1)*((scenario&2)?2:1);
        check(w.npc_combat_snapshot(npc)->health->current==500-expected,"independent light and summoned multipliers");
        auto missing=text;const auto at=missing.rfind(" test\n");missing.replace(at,6," -\n");
        bool rejected=false;try{std::istringstream bad(missing);load_catalog(bad,hash);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"explicit false summoned trait accepted without evidence");
    }
    using namespace casting;Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[252]=1;Slots bar{};bar[0]={252,0};attributes::Ranks ranks{};ranks[14]=12;
    Hooks hooks;unsigned hits=0,cost=0;effects::Scope npc{1,100,1};
    hooks.events=[](const auto&){};hooks.resources=[](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional<vitals::Resources>{{100,100,20,20}}:std::nullopt;};
    hooks.consume_energy=[&](unsigned amount){cost+=amount;return true;};hooks.target_valid=[](auto,float,bool){return false;};
    hooks.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==100?std::optional(npc):std::nullopt;};hooks.projectile_target_valid=[](auto id,float range){return id==100&&range==1248;};
    hooks.holy_damage=[&](auto target,const auto& hit,auto){check(target==npc&&hit==plan,"Banish cast rank or lifetime");++hits;return true;};
    Runtime cast(42);check(cast.start(100,{252,0,100},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&cost==5,"Banish activation/energy");cast.update(1100,hooks);check(hits==1,"Banish NPC dispatch");
    Runtime replaced(42);check(replaced.start(2000,{252,0,100},bar,policy,ranks,true,hooks)==casting::Reason::accepted,"replacement cast");++npc.generation;replaced.update(3000,hooks);check(hits==1,"cast crossed NPC lifetime");
    std::cout<<"Banish hidden traits, evidence, lifetime, damage and casting verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
