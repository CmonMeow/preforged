#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Stoneflesh player line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned kind=0;kind<8;++kind){
        world::Instance w;
        for(unsigned id:{1u,2u}){
            w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{10000,10000,100,100},100);
        }
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.physical_armor(b,60)&&w.combat_armor(b,60)&&w.elemental_armor(b,60,60)&&w.lightning_armor(b,60));
        check(!w.status_mutation(b,1375,69,12,100,999999,true));
        check(w.status_mutation(b,1375,69,12,100,13100,true));
        double expected=0;
        if(kind==0){check(w.shockwave(a,*caster_earth_aoe(174,0),20,200));expected=1;}
        if(kind==1){check(w.lightning(a,b,*lightning(224,0),20,200));expected=25*std::exp2(15./40.)-25;}
        if(kind==2){check(w.elemental(a,b,*elemental(191,0),20,200));expected=0;}
        if(kind==3){auto plan=*earth_projectile(1374,0,0);check(w.launch_projectile(a,2,plan,20,200));w.tick(500);expected=std::max(0.,double(plan.earth_damage)-25);}
        if(kind==4||kind==5){auto plan=*condition_damage(kind==4?828:864,0);check(w.condition_damage(a,b,plan,20,200));expected=std::max(0.,double(plan.damage)-25);}
        if(kind==6){check(w.ear_bite(a,b,*ear_bite(10),20,200));expected=45;}
        if(kind==7){combat::weapon::Owned sword{1,1,{40,40,2},12,false,false,27};check(w.ordinary_weapon(a,sword,20));
            auto plan=*combat::weapon::attack_skill(322,0);check(w.melee_attack(a,b,sword,plan,200));
            combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=40;
            strike.location_armor=60;strike.base_roll=.5;strike.miss_roll=.5;
            expected=std::max(0.,combat::weapon::resolve_attack_skill(strike,plan,false)->damage-25);
        }
        check(w.resources(2)->health==unsigned(std::ceil(10000.-expected)));
    }
    world::Instance w;w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.set_vitals(1,{1000,1000,100,100},100);const auto owner=*w.effect_scope(1);
    check(!w.status_mutation(owner,1375,69,21,100,13100,true));
    check(w.status_mutation(owner,1375,69,0,100,5100,true));
    check(w.status_mutation(owner,1375,69,0,200,{},false));
    {
        world::Instance battle;for(unsigned id:{1u,2u}){
            battle.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
            battle.set_vitals(id,{1000,1000,100,100},100);
        }
        const auto a=*battle.effect_scope(1),b=*battle.effect_scope(2);
        check(battle.status_mutation(b,1375,69,12,100,13100,true));
        const auto strip=*enchantment_removal(69,12);EnchantmentRemovalOutcome outcome;
        check(battle.enchantment_removal(a,b,strip,20,200,&outcome));
        check(battle.resources(2)->health==1000-strip.damage+25);
        check(battle.status_snapshot(b,200)->empty());
    }
    std::cout<<"PASS Stoneflesh Player World damage paths and duration admission\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
