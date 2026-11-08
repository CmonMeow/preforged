#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace skills;using namespace casting;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    check(effects::backfire(28,12)->damage==119&&effects::backfire(28,15)->damage==140&&!effects::backfire(28,21),"Backfire scale");
    for(unsigned scenario=0;scenario<5;++scenario){
        Instance w;w.join(1,"Mesmer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.join(2,"Monk",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        const unsigned initial=scenario==1?20:scenario==2?110:100;
        w.set_vitals(1,{200,200,40,40},100);w.set_vitals(2,{initial,200,40,40},100);
        const auto mesmer=*w.effect_scope(1),monk=*w.effect_scope(2);const auto plan=*effects::backfire(28);
        check(w.backfire(mesmer,monk,plan,20,100),"apply Backfire to player");
        const unsigned skill=scenario==2?282:scenario==3?301:scenario==4?293:281;
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};
        Hooks hooks;unsigned reactions=0,commits=0;std::uint64_t event=0;const auto end=100+std::uint64_t(std::llround(find_skill(skill)->activation*1000));
        hooks.events=[](const auto&){};hooks.resources=[&](unsigned id){return w.resources(id);};hooks.target_valid=[](auto id,float,bool){return id==2;};hooks.consume_energy=[](auto){return true;};
        hooks.before_spell_effect=[&](unsigned id,auto token,auto now){++reactions;event=token;return w.before_spell_effect(monk,id,token,now);};
        hooks.heal=[&](unsigned target,unsigned amount,float range,bool self){++commits;check(w.resources(2)->health==(scenario==4?initial:initial-35),"healing ran before Backfire");if(scenario==2)check(amount==35,"self-heal used pre-reaction half-health snapshot");return w.heal_target(2,target,amount,range,self,end);};
        hooks.remove_effects=[&](unsigned target,const auto& removal,auto now){++commits;check(w.resources(2)->health==65,"hex removal ran before Backfire");return w.remove_effects(monk,target,removal,now);};
        Runtime cast(2);check(cast.start(100,{skill,0,2},bar,policy,ranks,true,hooks)==casting::Reason::accepted,"reaction cast start");cast.update(end,hooks);cast.update(end,hooks);
        check(reactions==(scenario==4?0u:1u)&&commits==(scenario==1?0u:1u),"duplicate reaction/effect or non-spell reaction");
        if(scenario==1)check(w.resources(2)->health==0,"Backfire death was healed");
        if(scenario==2)check(w.resources(2)->health==110,"post-reaction conditional healing");
        if(scenario==3){check(w.status_snapshot(monk,end)->empty(),"Remove Hex left Backfire");check(w.before_spell_effect(monk,281,spell_activation_event(),end+1000)&&w.resources(2)->health==65,"removed Backfire triggered");}
        if(event&&scenario!=1)check(!w.before_spell_effect(monk,skill,event,end),"duplicate activation accepted");
    }
    {
        Instance w;w.join(1,"Mesmer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.join(2,"Target",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{200,200,40,40},100);w.set_vitals(2,{200,200,40,40},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.learned[28]=1;Slots bar{};bar[0]={28,0};attributes::Ranks ranks{};ranks[2]=12;Hooks h;unsigned energy=0;
        h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[&](auto cost){energy+=cost;return true;};h.target_valid=[](auto,float,bool){return false;};h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[](auto id,float range){return id==2&&range==1248;};
        h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(source,skill,event,now);};h.backfire=[&](auto victim,const auto& plan,auto now){return w.backfire(source,victim,plan,20,now);};
        Runtime cast(1);check(cast.start(100,{28,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted&&energy==15,"Backfire native cost");cast.update(3099,h);check(w.status_snapshot(target,3099)->empty(),"Backfire early application");cast.update(3100,h);
        check(w.before_spell_effect(target,281,spell_activation_event(),3100)&&w.resources(2)->health==81,"rank12 reaction");
        check(w.before_spell_effect(target,281,spell_activation_event(),13100)&&w.resources(2)->health==81,"Backfire exact expiry");
        auto stale=target;++stale.generation;check(!w.backfire(source,stale,*effects::backfire(28),20,13100),"stale Backfire target");
    }
    {
        Instance w;w.join(1,"Mesmer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.join(2,"Target",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{200,200,40,40},100);w.set_vitals(2,{200,200,40,40},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.backfire(source,target,*effects::backfire(28),20,100)&&w.backfire(source,target,*effects::backfire(28),20,100),"Backfire renewal");
        check(w.protective_enchantment(target,target,*regeneration::reviewed(299,0),0,100)&&w.status_mutation(target,346,9,0,100,8100,true),"damage modifiers fixture");
        check(w.before_spell_effect(target,281,spell_activation_event(),100)&&w.resources(2)->health==133,"renewal stacked reaction or wrong Frenzy/Shielding ordering");
        check(w.shatter_delusions(source,target,*effects::shatter_delusions(0),20,100),"Shatter Backfire");const auto after=w.resources(2)->health;
        check(w.before_spell_effect(target,281,spell_activation_event(),100)&&w.resources(2)->health==after,"Shattered Backfire triggered");
    }
    {
        // A reaction death must not reset a used signet or an earlier cooldown.
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned={{2,1},{281,1},{313,1}};Slots bar{};bar[0]={2,0};bar[1]={281,0};bar[2]={313,0};attributes::Ranks ranks{};
        vitals::Resources body{100,200,40,40},corpse{0,200,40,40};Hooks h;bool lethal=false;effects::Scope other{1,43,1};
        h.events=[](const auto&){};h.resources=[&](auto id)->std::optional<vitals::Resources>{return id==42?std::optional(body):id==43?std::optional(corpse):std::nullopt;};
        h.consume_energy=[](auto){return true;};h.target_valid=[](auto id,float,bool){return id==42;};h.target_scope=[&](auto id)->std::optional<effects::Scope>{return id==43?std::optional(other):std::nullopt;};
        h.resurrection_target_valid=[](auto id,float){return id==43;};h.resurrect=[&](auto target,const auto&,auto){corpse.health=100;return target==other;};
        h.heal=[&](auto,unsigned amount,float,bool){body.health=std::min(body.max_health,body.health+amount);return true;};
        h.before_spell_effect=[&](auto,auto,auto){if(lethal){body.health=0;return false;}return true;};
        Runtime cast(42);check(cast.start(100,{2,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted,"signet fixture");cast.update(3100,h);
        check(cast.start(3200,{281,0,42},bar,policy,ranks,true,h)==casting::Reason::accepted,"recharge fixture");cast.update(4200,h);lethal=true;
        check(cast.start(5000,{313,0,42},bar,policy,ranks,true,h)==casting::Reason::accepted,"lethal reaction fixture");cast.update(5750,h);cast.update(5751,h);body.health=100;lethal=false;
        check(cast.start(5800,{281,0,42},bar,policy,ranks,true,h)==casting::Reason::recharge,"Backfire death reset existing cooldown");
        check(cast.start(5800,{2,0,43},bar,policy,ranks,true,h)==casting::Reason::recharge,"Backfire death reset used Resurrection Signet");
    }
    {
        Instance w;w.join(1,"Mesmer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.join(2,"Target",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{200,200,40,40},100);w.set_vitals(2,{200,200,40,40},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        w.tick(1100);check(w.before_spell_effect(target,281,spell_activation_event(),100),"unhexed spell rejected after newer world health tick");
        check(w.backfire(source,target,*effects::backfire(28),20,1100),"clock fixture Backfire");w.tick(1200);
        check(w.before_spell_effect(target,281,spell_activation_event(),1100)&&w.resources(2)->health==165,"late spell reaction rewound or rejected current health");w.tick(11100);
        check(w.before_spell_effect(target,281,spell_activation_event(),1200)&&w.resources(2)->health==165,"late timestamp resurrected expired Backfire");
    }
    {
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[281]=1;Slots bar{};bar[0]={281,0};attributes::Ranks ranks{};Hooks h;bool gone=false;unsigned heals=0;
        h.events=[](const auto&){};h.resources=[&](auto)->std::optional<vitals::Resources>{return gone?std::nullopt:std::optional<vitals::Resources>{{100,200,40,40}};};h.consume_energy=[](auto){return true;};h.target_valid=[](auto,float,bool){return true;};h.heal=[&](auto,auto,float,bool){++heals;return true;};h.before_spell_effect=[&](auto,auto,auto){gone=true;return true;};
        Runtime cast(42);check(cast.start(100,{281,0,42},bar,policy,ranks,true,h)==casting::Reason::accepted,"disappearing actor fixture");cast.update(1100,h);check(!heals,"missing post-reaction actor was dereferenced/healed");
    }
    {
        using namespace combat::npcs;const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 1 "+hash+"\nsource test authored "+hash+" tests/backfire.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 - 1 0 test test - - -\n");const auto catalog=load_catalog(metadata,hash);bind_combat_catalog(population,catalog,hash);
        Instance w;w.configure_npcs(population.for_map(146));w.join(1,"Mesmer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(1,{200,200,40,40},100);
        w.join(2,"Bonded",500,0,std::make_shared<Outbox>(),0,{}, {},100,2);w.set_vitals(2,{200,200,40,40},100);
        const auto source=*w.effect_scope(1),npc=*w.effect_scope(100),bonded=*w.effect_scope(2);
        check(w.elemental_armor(bonded,60,60),"Bonded cold armor");
        check(w.backfire(source,npc,*effects::backfire(28),20,100),"NPC Backfire");
        check(w.stage_npc_bonds_of_torment(npc,bonded,*effects::bonds_of_torment_npc(),100),"NPC Bonds");
        check(w.before_spell_effect(npc,281,spell_activation_event(),100)&&w.npc_combat_snapshot(npc)->health->current==65,"trusted NPC spell reaction");
        check(w.resources(2)->health==187,"Backfire NPC damage did not produce the level-eight cold hit");
        check(!w.before_spell_effect(npc,293,spell_activation_event(),100)&&w.npc_combat_snapshot(npc)->health->current==65,"non-spell trusted event accepted");
        w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(1,{200,200,40,40},100);
        check(w.before_spell_effect(npc,281,spell_activation_event(),100)&&w.npc_combat_snapshot(npc)->health->current==30,"original caster departure incorrectly erased damage");
        check(!w.before_spell_effect(npc,281,spell_activation_event(),100)&&w.npc_combat_snapshot(npc)->health->current==0,"lethal NPC activation proceeded");
    }
    {
        Instance w;const auto out=std::make_shared<Outbox>();w.join(1,"Mesmer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.join(2,"Target",100,0,out,0,{}, {},100,2);
        w.set_vitals(1,{200,200,40,40},100);w.set_vitals(2,{100,200,40,40},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.backfire(source,target,*effects::backfire(28),20,100),"departed source player fixture");w.leave(1);out->drain();
        check(w.before_spell_effect(target,281,spell_activation_event(),100)&&w.resources(2)->health==65,"departed source player damage");
        const auto packets=out->drain();const auto expected=combat::health_packets_2022({2,target.generation,combat::Health{65,200},{},0,true});
        for(const auto& packet:expected)check(std::find(packets.begin(),packets.end(),packet)!=packets.end(),"departed source published pre-hit player health");
    }
    std::cout<<"Backfire activation ordering, deaths, removal, expiry, deduplication and NPC lifetime handling verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
