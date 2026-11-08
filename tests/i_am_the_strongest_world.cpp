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
    if(!ok)throw std::runtime_error("I Am the Strongest World line "+std::to_string(at.line()));
}
int main()try{
    const auto run=[](bool buff,bool blind,bool block){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Attacker",0,0,box,0,{}, {},100,1);
        w.join(2,"Defender",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u})w.set_vitals(id,{480,480,30,30},100);
        const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2);
        const combat::weapon::Owned sword{12,1,{20,20,1},12,false,false,2};
        check(w.ordinary_weapon(attacker,sword,20)&&w.physical_armor(target,60));
        w.configure_attack_miss_draws([]{return .5;});
        if(buff)check(w.i_am_the_strongest(attacker,*skills::effects::i_am_the_strongest(0),200));
        if(blind)check(w.apply_condition(attacker,attacker,combat::conditions::Kind::blind,200,5000));
        if(block){w.configure_attack_block_draws([]{return 0.;});
            check(w.status_mutation(target,448,1,12,200,200+*skills::effects::escape(448,12),true));}
        box->drain();check(w.ordinary_attack(attacker,2,200));w.tick(700);
        const auto state=w.i_am_the_strongest_snapshot(attacker,700);
        return std::pair{w.resources(2)->health,state?state->remaining:0u};
    };
    const auto base=run(false,false,false),hit=run(true,false,false);
    check(base.first>hit.first&&base.first-hit.first==14&&hit.second==4);
    const auto missed=run(true,true,false),blocked=run(true,false,true);
    check(missed.first==480&&missed.second==5);
    check(blocked.first==480&&blocked.second==5);
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    w.join(1,"Norn",0,0,box,0,{}, {},100,1);w.set_vitals(1,{480,480,30,30},100);
    const auto owner=*w.effect_scope(1);
    const auto plan=*skills::effects::i_am_the_strongest(5);
    check(!w.i_am_the_strongest({owner.instance,owner.actor,owner.generation+1},plan,200));
    check(w.i_am_the_strongest(owner,plan,200));
    check(w.i_am_the_strongest_snapshot(owner,200)->remaining==8);
    auto wire=box->drain();
    check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,1909))==1);
    check(w.i_am_the_strongest(owner,plan,300));
    wire=box->drain();
    check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,1909))==1);
    check(!w.i_am_the_strongest_snapshot(owner,300));
    check(w.status_snapshot(owner,300)->empty());
    check(w.i_am_the_strongest(owner,plan,400));
    check(bool(w.i_am_the_strongest_snapshot(owner,400)));
    check(w.kill(owner,500));
    check(!w.i_am_the_strongest_snapshot(owner,500));
    w.leave(1);
    check(!w.i_am_the_strongest_snapshot(owner,501));
    world::Instance expiry;auto expiry_box=std::make_shared<world::Outbox>();
    expiry.join(9,"Expiry",0,0,expiry_box,0,{}, {},100,1);
    expiry.set_vitals(9,{480,480,30,30},100);
    const auto expiring=*expiry.effect_scope(9);
    check(expiry.i_am_the_strongest(expiring,plan,200));
    check(bool(expiry.i_am_the_strongest_snapshot(expiring,27000199ull)));
    check(!expiry.i_am_the_strongest_snapshot(expiring,27000200ull));
    const auto sword_skill=[&](bool buff){
        world::Instance x;auto observer=std::make_shared<world::Outbox>();
        x.join(1,"Warrior",0,0,observer,0,{}, {},100,1);
        x.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u})x.set_vitals(id,{480,480,30,30},100);
        const auto attacker=*x.effect_scope(1),target=*x.effect_scope(2);
        combat::weapon::Owned sword{10,1,{10,10,2},12,false,false,27};
        check(x.ordinary_weapon(attacker,sword,20)&&x.physical_armor(target,60));
        if(buff)check(x.i_am_the_strongest(attacker,*skills::effects::i_am_the_strongest(0),200));
        check(x.melee_attack(attacker,target,sword,*combat::weapon::attack_skill(382,12),200));
        if(buff)check(x.i_am_the_strongest_snapshot(attacker,200)->remaining==4);
        return x.resources(2)->health;
    };
    check(sword_skill(false)-sword_skill(true)==14);
    struct CycloneResult {unsigned player=0;double npc=0;unsigned remaining=0;};
    const auto cyclone=[&](bool buff,bool block_first){
        const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 -166 0 0 - -\n");
        auto population=world::load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+
            " tests/i_am_the_strongest_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 1 test test test - test 60 60 test test 60 test\n");
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
        if(buff)check(x.i_am_the_strongest(attacker,*skills::effects::i_am_the_strongest(0),200));
        if(block_first){x.configure_attack_block_draws([]{return 0.;});
            check(x.status_mutation(target,448,1,12,200,200+*skills::effects::escape(448,12),true));}
        check(x.melee_attack(attacker,attacker,axe,*combat::weapon::attack_skill(330,12),200));
        const auto state=x.i_am_the_strongest_snapshot(attacker,200);
        return CycloneResult{x.resources(2)->health,x.npc_combat_snapshot(npc)->health->current,
            state?state->remaining:0u};
    };
    const auto plain=cyclone(false,false),double_hit=cyclone(true,false);
    check(plain.player-double_hit.player==14&&plain.npc-double_hit.npc==14&&double_hit.remaining==3);
    const auto blocked_plain=cyclone(false,true),blocked_cyclone=cyclone(true,true);
    check(blocked_plain.player==blocked_cyclone.player&&blocked_plain.npc-blocked_cyclone.npc==14&&blocked_cyclone.remaining==4);
    std::cout<<"PASS I Am the Strongest World cast, hit-only charges, body and reapplication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
