#include "gw_npc_catalog.h"
#include "gw_ignite_arrows.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace world;using namespace combat::npcs;
void check(bool value,std::source_location where=std::source_location::current()){
    if(!value)throw std::runtime_error("Ignite Arrows regression line "+std::to_string(where.line()));
}
int main()try{
    for(bool reversal:{false,true}){
        Instance w;w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .99;});
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?100:266,0,std::make_shared<Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{480,480,100,100},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2),side=*w.effect_scope(3);
        for(auto scope:{target,side}){check(w.physical_armor(scope,60)&&w.elemental_armor(scope,60,60));check(w.status_mutation(scope,1399,80,12,100,6100,true));}
        if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
        combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};check(w.ordinary_weapon(source,bow,20)&&w.status_mutation(source,431,1,12,100,24100,true));
        check(w.ordinary_attack(source,2,100));w.tick(600);w.tick(700);w.cancel_ordinary_attack(source);
        check(w.resources(2)->health==(reversal?475u:460u)&&w.resources(3)->health==470);
        check(w.ordinary_attack(source,2,800));w.tick(1300);w.tick(1400);w.cancel_ordinary_attack(source);
        check(w.resources(2)->health==(reversal?470u:455u)&&w.resources(3)->health==465);
    }

    check(skills::ranger::ignite_arrows(0)->damage==3&&skills::ranger::ignite_arrows(12)->damage==15&&!skills::ranger::ignite_arrows(21));
    for(const bool blind:{false,true})for(const bool remove_in_flight:{false,true}){
        auto world=std::make_unique<Instance>();auto box=std::make_shared<Outbox>();
        world->configure_attack_miss_draws([]{return .5;});
        world->join(1,"Ranger",0,0,box,0,{}, {},100,1);world->join(2,"Target",100,0,box,0,{}, {},100,2);
        world->join(3,"Adjacent",266,0,box,0,{}, {},100,2);world->join(4,"Outside",267,0,box,0,{}, {},100,2);
        for(unsigned id=1;id<=4;++id)world->set_vitals(id,{480,480,20,20},100);
        const auto source=*world->effect_scope(1),target=*world->effect_scope(2);
        for(unsigned id=2;id<=4;++id)check(world->physical_armor(*world->effect_scope(id),60)&&world->elemental_armor(*world->effect_scope(id),60,60));
        combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};check(world->ordinary_weapon(source,bow,20));
        check(world->status_mutation(source,432,1,12,100,24100,true));
        check(world->status_mutation(source,431,2,12,100,24100,true));
        const auto effects=*world->status_snapshot(source,100);check(effects.size()==1&&effects.front().skill==431);
        if(blind)check(world->apply_condition(source,source,combat::conditions::Kind::blind,100,1000));
        check(world->melee_attack(source,target,bow,*combat::weapon::attack_skill(396,0),100));
        if(remove_in_flight)check(world->status_mutation(source,431,2,0,110,{},false));
        world->tick(200);check(world->resources(2)->health==(blind?450u:420u));
        check(world->resources(3)->health==450&&world->resources(4)->health==480);
    }
    {
        auto world=std::make_unique<Instance>();auto box=std::make_shared<Outbox>();
        world->join(1,"Ranger",0,0,box,0,{}, {},100,1);world->join(2,"Target",100,0,box,0,{}, {},100,2);
        for(unsigned id=1;id<=2;++id)world->set_vitals(id,{480,480,20,20},100);
        const auto source=*world->effect_scope(1),target=*world->effect_scope(2);combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};
        check(world->ordinary_weapon(source,bow,20)&&world->physical_armor(target,60)&&world->elemental_armor(target,60,60));
        check(world->status_mutation(source,431,1,12,100,24100,true));
        check(world->status_mutation(source,432,2,12,100,24100,true));
        check(world->ordinary_attack(source,2,100));world->tick(600);world->tick(700);
        check(world->resources(2)->health==451); //20 base+9 Read the Wind; no stale Ignite explosion.
    }
    const std::string hash(64,'a');std::ostringstream appearance,metadata;
    appearance<<"gw-npcs 1\n";metadata<<"gw-npc-combat 2 "<<hash<<"\nsource test authored "<<hash<<" tests/ignite_arrows.cpp\n";
    for(unsigned id=100;id<=102;++id){const unsigned x=id==100?100:id==101?266:267;
        appearance<<"npc 146 "<<id<<" 1 1234 0 1677721600 0 8 1 1 0 "<<x<<" 0 0 - -\n";
        metadata<<"npc 146 "<<id<<" 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 1 test test test - test 60 60 test test\n";
    }
    std::istringstream input(appearance.str()),meta(metadata.str());auto population=load_npc_catalog(input);
    const auto catalog=load_catalog(meta,hash);bind_combat_catalog(population,catalog,hash);
    auto npcs=population.for_map(146);for(auto& npc:npcs){npc.combat_definition->physical_armor=60;npc.combat_definition->physical_armor_evidence="test";}
    for(bool absorption:{false,true}){
        auto world=std::make_unique<Instance>();world->configure_npcs(npcs);world->join(1,"Ranger",0,0,std::make_shared<Outbox>());world->set_vitals(1,{100,100,20,20},100);
        const auto source=*world->effect_scope(1),target=*world->effect_scope(100);combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};
        check(world->ordinary_weapon(source,bow,20)&&world->status_mutation(source,431,1,12,100,24100,true));
        if(absorption)for(unsigned id:{100u,101u})check(world->stage_npc_shield_absorption(*world->effect_scope(id),12,100));
        check(world->ordinary_attack(source,100,100));world->tick(600);world->tick(700);
        check(world->npc_combat_snapshot(target)->health->current==(absorption?80:65));
        check(world->npc_combat_snapshot(*world->effect_scope(101))->health->current==(absorption?90:85));
        check(world->npc_combat_snapshot(*world->effect_scope(102))->health->current==100);
    }
    std::cout<<"Ignite Arrows player/NPC adjacent fire, Blind, Dual Shot, captured preparation, and replacement verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
