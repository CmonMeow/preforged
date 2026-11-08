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
    if(!ok)throw std::runtime_error("Signet of Strength World line "+
        std::to_string(at.line()));
}
int main()try{
    world::Instance w;auto box=std::make_shared<world::Outbox>();
    w.join(1,"Warrior",0,0,box,0,{}, {},100,1);
    w.set_vitals(1,{1000,1000,30,30},100);
    const auto owner=*w.effect_scope(1);
    const auto plan=*skills::effects::signet_strength(12);
    auto forged=plan;++forged.bonus_damage;
    box->drain();
    check(!w.apply_signet_strength({owner.instance,owner.actor,owner.generation+1},plan,200));
    check(!w.apply_signet_strength(owner,forged,200));
    check(!w.signet_strength_snapshot(owner,200));
    check(w.apply_signet_strength(owner,plan,200));
    const auto first=w.signet_strength_snapshot(owner,200);
    check(first&&first->remaining==13&&first->owner==owner);
    const auto status=w.status_snapshot(owner,200);
    check(status&&status->size()==1&&status->front().skill==944&&
        status->front().instance==first->effect&&!status->front().expires);
    auto wire=box->drain();
    world::Packet maintained;world::u16(maintained,64);world::u32(maintained,1);
    world::u32(maintained,0);world::u16(maintained,944);
    world::u32(maintained,12);world::u32(maintained,first->effect);
    check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,1028))==1);
    check(std::count(wire.begin(),wire.end(),maintained)==1);
    check(w.apply_signet_strength(owner,plan,300));
    const auto renewed=w.signet_strength_snapshot(owner,300);
    check(renewed&&renewed->remaining==13&&renewed->effect!=first->effect);
    wire=box->drain();
    world::Packet removed;world::u16(removed,67);world::u32(removed,1);
    world::u32(removed,first->effect);
    check(std::count(wire.begin(),wire.end(),removed)==1);
    check(w.status_snapshot(owner,300)->size()==1);
    box->drain();
    check(w.kill(owner,400));
    check(!w.signet_strength_snapshot(owner,400));
    check(w.status_snapshot(owner,400)->empty());
    world::Packet death_removed;world::u16(death_removed,67);
    world::u32(death_removed,1);world::u32(death_removed,renewed->effect);
    wire=box->drain();
    check(std::count(wire.begin(),wire.end(),death_removed)==1);
    w.leave(1);
    check(!w.signet_strength_snapshot(owner,400));
    w.join(1,"Replacement",0,0,box,0,{}, {},500,1);
    w.set_vitals(1,{1000,1000,30,30},100);
    check(!w.signet_strength_snapshot(*w.effect_scope(1),500));
    const auto ordinary=[&](bool buff,bool blind,bool block=false){
        world::Instance x;auto observer=std::make_shared<world::Outbox>();
        x.join(1,"Attacker",0,0,observer,0,{}, {},100,1);
        x.join(2,"Defender",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        x.set_vitals(1,{480,480,30,30},100);
        x.set_vitals(2,{480,480,30,30},100);
        const auto attacker=*x.effect_scope(1),defender=*x.effect_scope(2);
        combat::weapon::Owned sword{12,1,{20,20,1},12,false,false,2};
        check(x.ordinary_weapon(attacker,sword,20));
        check(x.physical_armor(defender,60));
        x.configure_attack_miss_draws([]{return .5;});
        if(buff)check(x.apply_signet_strength(attacker,*skills::effects::signet_strength(0),200));
        const auto strength_effect=buff?x.signet_strength_snapshot(attacker,200)->effect:0u;
        if(blind)check(x.apply_condition(attacker,attacker,combat::conditions::Kind::blind,200,5000));
        if(block){x.configure_attack_block_draws([]{return 0.;});
            check(x.status_mutation(defender,448,1,12,200,
                200+*skills::effects::escape(448,12),true));}
        observer->drain();
        check(x.ordinary_attack(attacker,2,200));x.tick(700);
        const auto charges=x.signet_strength_snapshot(attacker,700);
        if(buff){
            const auto status=x.status_snapshot(attacker,700);
            check(status&&status->size()==(blind||block?1u:0u));
            if(!blind&&!block){
                world::Packet ended;world::u16(ended,67);world::u32(ended,1);
                world::u32(ended,strength_effect);
                const auto packets=observer->drain();
                check(std::count(packets.begin(),packets.end(),ended)==1);
            }
        }
        return std::pair{x.resources(2)->health,
            charges?charges->remaining:0u};
    };
    const auto baseline=ordinary(false,false),boosted=ordinary(true,false);
    check(baseline.first>boosted.first&&baseline.first-boosted.first==5);
    check(boosted.second==0);
    const auto missed=ordinary(true,true);
    check(missed.first==480&&missed.second==1);
    const auto blocked=ordinary(true,false,true);
    check(blocked.first==480&&blocked.second==1);
    const auto sword_skill=[&](bool buff){
        world::Instance x;auto observer=std::make_shared<world::Outbox>();
        x.join(1,"Warrior",0,0,observer,0,{}, {},100,1);
        x.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u})x.set_vitals(id,{480,480,30,30},100);
        const auto attacker=*x.effect_scope(1),target=*x.effect_scope(2);
        combat::weapon::Owned sword{10,1,{10,10,2},12,false,false,27};
        check(x.ordinary_weapon(attacker,sword,20)&&x.physical_armor(target,60));
        if(buff)check(x.apply_signet_strength(attacker,*skills::effects::signet_strength(0),200));
        check(x.melee_attack(attacker,target,sword,*combat::weapon::attack_skill(382,12),200));
        if(buff)check(!x.signet_strength_snapshot(attacker,200));
        return x.resources(2)->health;
    };
    check(sword_skill(false)-sword_skill(true)==5);
    const auto npc_contact=[&](bool buff,bool skill=false){
        const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 50 0 0 - -\n");
        auto population=world::load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+
            " tests/signet_strength_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 0 test test test - - 60 - test - 60 test\n");
        world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
        world::Instance x;x.configure_npcs(population.for_map(146));
        auto observer=std::make_shared<world::Outbox>();
        x.join(1,"Attacker",0,0,observer,0,{}, {},100,1);
        x.set_vitals(1,{480,480,30,30},100);
        const auto attacker=*x.effect_scope(1),target=*x.effect_scope(100);
        combat::weapon::Owned sword{12,1,{20,20,1},12,false,false,skill?27u:2u};
        check(x.ordinary_weapon(attacker,sword,20));
        x.configure_attack_miss_draws([]{return .5;});
        if(buff)check(x.apply_signet_strength(attacker,*skills::effects::signet_strength(0),200));
        if(skill)check(x.melee_attack(attacker,target,sword,*combat::weapon::attack_skill(382,12),200));
        else {check(x.ordinary_attack(attacker,100,200));x.tick(700);}
        if(buff)check(!x.signet_strength_snapshot(attacker,skill?200:700));
        return x.npc_combat_snapshot(target)->health->current;
    };
    check(npc_contact(false)-npc_contact(true)==5);
    check(npc_contact(false,true)-npc_contact(true,true)==5);
    struct CycloneResult{unsigned player=0;double npc=0;unsigned charges=0;};
    const auto cyclone=[&](std::optional<unsigned> rank,bool block_first=false){
        const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 -166 0 0 - -\n");
        auto population=world::load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+
            " tests/signet_strength_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 1 test test test - test 60 60 test test 60 test\n");
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
        if(rank)check(x.apply_signet_strength(attacker,*skills::effects::signet_strength(*rank),200));
        if(block_first){x.configure_attack_block_draws([]{return 0.;});
            check(x.status_mutation(target,448,1,12,200,
                200+*skills::effects::escape(448,12),true));}
        check(x.melee_attack(attacker,attacker,axe,*combat::weapon::attack_skill(330,12),200));
        const auto state=x.signet_strength_snapshot(attacker,200);
        return CycloneResult{x.resources(2)->health,x.npc_combat_snapshot(npc)->health->current,
            state?state->remaining:0u};
    };
    const auto plain=cyclone({}),single=cyclone(0),double_hit=cyclone(1);
    check(plain.player-single.player==5&&plain.npc==single.npc&&single.charges==0);
    check(plain.player-double_hit.player==5&&plain.npc-double_hit.npc==5&&double_hit.charges==0);
    const auto blocked_plain=cyclone({},true),blocked_strength=cyclone(0,true);
    check(blocked_plain.player==blocked_strength.player&&
        blocked_plain.npc-blocked_strength.npc==5&&blocked_strength.charges==0);
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
