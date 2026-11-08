#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Dodge This World line "+std::to_string(at.line()));
}
int main()try{
    const auto ordinary=[](bool buff,bool blind,bool block){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Attacker",0,0,box,0,{}, {},100,1);
        w.join(2,"Defender",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u})w.set_vitals(id,{480,480,30,30},100);
        const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2);
        combat::weapon::Owned sword{12,1,{20,20,1},12,false,false,2};
        check(w.ordinary_weapon(attacker,sword,20)&&w.physical_armor(target,60));
        w.configure_attack_miss_draws([]{return .5;});
        if(buff)check(w.dodge_this(attacker,*skills::effects::dodge_this(0),200));
        if(blind)check(w.apply_condition(attacker,attacker,combat::conditions::Kind::blind,200,5000));
        if(block){w.configure_attack_block_draws([]{return 0.;});
            check(w.status_mutation(target,448,1,12,200,200+*skills::effects::escape(448,12),true));}
        check(w.ordinary_attack(attacker,2,200));w.tick(700);
        return std::pair{w.resources(2)->health,w.dodge_this_armed(attacker,700)};
    };
    const auto base=ordinary(false,false,false),boosted=ordinary(true,false,false);
    check(base.first-boosted.first==14&&!boosted.second);
    const auto missed=ordinary(true,true,false),blocked=ordinary(false,false,true),unblocked=ordinary(true,false,true);
    check(missed.first==480&&!missed.second);
    check(blocked.first==480&&unblocked.first<480&&!unblocked.second);
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    w.join(1,"Norn",0,0,box,0,{}, {},100,1);w.set_vitals(1,{480,480,30,30},100);
    const auto owner=*w.effect_scope(1);const auto plan=*skills::effects::dodge_this(5);
    check(!w.dodge_this({owner.instance,owner.actor,owner.generation+1},plan,200));
    check(w.dodge_this(owner,plan,200));
    check(w.dodge_this_armed(owner,200));
    check(w.status_snapshot(owner,200)->size()==1);
    check(w.dodge_this(owner,plan,300));
    check(w.status_snapshot(owner,300)->size()==1);
    check(!w.dodge_this_armed(owner,20300));
    check(w.kill(owner,20400));w.leave(1);
    struct CycloneResult{unsigned player=0;double npc=0;bool armed=false;};
    const auto cyclone=[](bool buff,bool block_first){
        const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 -166 0 0 - -\n");
        auto population=world::load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+
            " tests/dodge_this_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 1 test test test - test 60 60 test test 60 test\n");
        world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
        world::Instance x;x.configure_npcs(population.for_map(146));
        auto observer=std::make_shared<world::Outbox>();
        x.join(1,"Warrior",0,0,observer,0,{}, {},100,1);
        x.join(2,"Foe",166,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        x.set_vitals(1,{100,100,20,20},100);x.set_vitals(2,{100,100,20,20},100);
        const auto attacker=*x.effect_scope(1),target=*x.effect_scope(2),npc=*x.effect_scope(100);
        combat::weapon::Owned axe{1,1,{20,20,1},12,false,false,2};
        check(x.ordinary_weapon(attacker,axe,20)&&x.physical_armor(target,60));
        x.configure_attack_miss_draws([]{return .5;});
        if(buff)check(x.dodge_this(attacker,*skills::effects::dodge_this(0),200));
        if(block_first){x.configure_attack_block_draws([]{return 0.;});
            check(x.status_mutation(target,448,1,12,200,200+*skills::effects::escape(448,12),true));}
        check(x.melee_attack(attacker,attacker,axe,*combat::weapon::attack_skill(330,12),200));
        return CycloneResult{x.resources(2)->health,x.npc_combat_snapshot(npc)->health->current,
            x.dodge_this_armed(attacker,200)};
    };
    const auto plain=cyclone(false,false),buffed=cyclone(true,false);
    check(plain.player-buffed.player==14&&plain.npc==buffed.npc&&!buffed.armed);
    const auto blocked_plain=cyclone(false,true),buffed_block=cyclone(true,true);
    check(blocked_plain.player==100&&buffed_block.player<100&&blocked_plain.npc==buffed_block.npc);
    std::cout<<"PASS Dodge This World one-attack unblockable/bonus, miss and Cyclone first contact\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
