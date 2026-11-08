#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("elemental Mantra regression line "+std::to_string(at.line()));
}
std::unique_ptr<world::Instance> fixture(){
    auto w=std::make_unique<world::Instance>();
    w->join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w->join(2,"Defender",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    w->set_vitals(1,{1000,1000,20,40},100);w->set_vitals(2,{1000,1000,0,40},100);
    const auto target=*w->effect_scope(2);
    check(w->combat_armor(target,60)&&w->elemental_armor(target,60,60)&&w->lightning_armor(target,60));return w;
}
int main()try{
    for(unsigned skill=6;skill<=9;++skill)for(unsigned rank=0;rank<=20;++rank){
        const auto plan=*effects::elemental_mantra(skill,rank);
        check(plan.energy==2&&plan.reduction==effects::interpolate(26,50,rank)&&plan.duration==std::uint64_t(30+4*rank)*1000);
        combat::Actor actor{42,1,combat::Health{1000,1000},60,0,true,60,60,60};
        actor.elemental_protection=plan.kind;actor.elemental_multiplier=1.-double(plan.reduction)/100.;
        auto hit=combat::apply(actor,{plan.kind,100,20,174},true);
        check(hit.reason==combat::Reason::accepted&&std::abs(hit.damage-(100-plan.reduction))<1e-9);
        actor.health=combat::Health{1000,1000};actor.incoming_damage_multiplier=2;actor.incoming_reduction=3;
        check(std::abs(combat::apply(actor,{plan.kind,100,20,174},true).damage-(2*(100-plan.reduction)-3))<1e-9);
        actor.health=combat::Health{1000,1000};actor.incoming_damage_multiplier=1;actor.incoming_reduction=0;
        check(combat::apply(actor,{combat::Kind::armor_ignoring,100,20,133},true).damage==100);
        effects::Entry entry{1,42,skill,rank,effects::Category::stance,100,100+plan.duration};
        check(!effects::active_elemental_mantra({entry},99));
        check(effects::active_elemental_mantra({entry},100)&&!effects::active_elemental_mantra({entry},100+plan.duration));
    }
    check(!effects::elemental_mantra(5)&&!effects::elemental_mantra(10)&&!effects::elemental_mantra(6,21));
    {
        auto w=fixture();const auto a=*w->effect_scope(1),b=*w->effect_scope(2);
        check(w->status_mutation(b,8,1,15,100,90100,true));
        check(w->elemental(a,b,*effects::elemental(89,0),20,100));
        check(w->resources(2)->energy==2); // One cold hit plus one shadow hit, not two cold hits.
        check(w->elemental(a,b,*effects::elemental(191,0),20,100)&&w->resources(2)->energy==2);
    }
    {
        auto w=fixture();const auto a=*w->effect_scope(1),b=*w->effect_scope(2);
        check(w->status_mutation(b,6,1,15,100,90100,true));
        check(w->shockwave(a,*effects::caster_earth_aoe(937,0),20,100));
        check(w->resources(2)->health==978&&w->resources(2)->energy==6); // Three separate elemental hits.
    }
    for(bool expired:{false,true}){
        auto w=fixture();const auto a=*w->effect_scope(1),b=*w->effect_scope(2);
        w->set_vitals(2,{1000,1000,39,40},100);
        check(w->status_mutation(b,7,1,15,100,101,true));
        check(w->elemental(a,b,*effects::elemental(191,0),20,expired?101:100));
        check(w->resources(2)->energy==(expired?39u:40u));
        check(w->resources(2)->health==(expired?980u:990u));
    }
    {
        auto w=fixture();const auto a=*w->effect_scope(1),b=*w->effect_scope(2);
        check(w->status_mutation(b,7,1,15,100,90100,true));
        check(w->protective_enchantment(b,b,*regeneration::reviewed(299,15),0,100));
        check(w->elemental(a,b,*effects::elemental(191,0),20,100));
        check(w->resources(2)->health==1000&&w->resources(2)->energy==2); // Absorbed elemental event still triggers.
    }
    {
        auto w=fixture();const auto a=*w->effect_scope(1),b=*w->effect_scope(2);
        check(w->status_mutation(b,7,1,15,100,90100,true));
        check(w->energy_capacity(2,1,0,100)&&w->resources(2)->max_energy==20);
        for(unsigned i=0;i<10;++i){check(w->elemental(a,b,*effects::elemental(191,0),20,100));check(w->resources(2)->energy==0);}
        check(w->elemental(a,b,*effects::elemental(191,0),20,100)&&w->resources(2)->energy==2); // Repay negative Energy first.
    }
    {
        auto w=fixture();const auto a=*w->effect_scope(1),b=*w->effect_scope(2);
        check(w->status_mutation(b,6,1,15,100,90100,true)&&w->physical_armor(b,60));
        combat::weapon::Owned axe{1,1,{20,20,1},12,false,false,2};check(w->ordinary_weapon(a,axe,20));
        check(w->melee_attack(a,b,axe,*combat::weapon::attack_skill(322,0),100));
        check(w->resources(2)->energy==0&&w->resources(2)->health<1000);
        check(w->ordinary_attack(a,2,100));w->tick(600);check(w->resources(2)->energy==0);
    }
    {
        auto w=fixture();const auto a=*w->effect_scope(1),b=*w->effect_scope(2);
        check(w->status_mutation(b,7,1,15,100,90100,true));
        combat::weapon::Owned wand{1,1,{20,20,5},0,false,true,22};check(w->ordinary_weapon(a,wand,20));
        check(w->ordinary_attack(a,2,100));w->tick(600);w->tick(1000);
        check(w->resources(2)->health==990&&w->resources(2)->energy==2);
    }
    for(unsigned skill=6;skill<=9;++skill){
        auto w=std::make_unique<world::Instance>();
        w->join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w->join(2,"Defender",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w->set_vitals(1,{1000,1000,20,40},100);w->set_vitals(2,{1000,1000,0,40},100);
        const auto a=*w->effect_scope(1),b=*w->effect_scope(2);
        check(w->combat_armor(b,60)&&w->elemental_armor(b,60,60)&&w->lightning_armor(b,60));
        check(w->status_mutation(b,skill,1,15,100,90100,true));
        double base=0;
        if(skill==6){auto p=*effects::caster_earth_aoe(174,0);base=p.strikes[0].earth_damage;check(w->shockwave(a,p,20,100));}
        if(skill==7){auto p=*effects::elemental(191,0);base=p.damage;check(w->elemental(a,b,p,20,100));}
        if(skill==8){auto p=*effects::elemental(846,0);base=p.damage;check(w->elemental(a,b,p,20,100));}
        if(skill==9){auto p=*effects::lightning(224,0);base=*combat::amount({combat::Kind::lightning,p.damage,20,p.skill,p.penetration},60);check(w->lightning(a,b,p,20,100));}
        check(w->resources(2)->health==unsigned(std::ceil(1000-base*.5))&&w->resources(2)->energy==2);
        // Unrelated armor-ignoring damage/life stealing cannot trigger a Mantra.
        check(w->shadow_damage(a,b,*effects::shadow_damage(133,0),20,100));
        check(w->life_steal(a,b,*effects::life_steal(153,0),100)&&w->resources(2)->energy==2);
        check(w->status_mutation(b,13,2,0,100,5100,true));
        check(w->status_snapshot(b,100)->size()==1);
        check(w->elemental(a,b,*effects::elemental(191,0),20,100)&&w->resources(2)->energy==2);
    }
    {
        using namespace casting;
        Policy policy;policy.primary=5;policy.campaigns=0x1e;
        Slots bar{};for(unsigned i=0;i<4;++i){bar[i]={6+i,0};policy.learned[6+i]=1;}
        attributes::Ranks ranks{};ranks[find_skill(6)->attribute]=15;Hooks h;unsigned cost=0,commits=0;
        h.resources=[](auto)->std::optional<vitals::Resources>{return vitals::Resources{100,100,100,100};};
        h.consume_energy=[&](unsigned n){cost+=n;return true;};h.events=[](const auto&){};
        h.target_valid=[](unsigned id,float,bool self){return id==42&&self;};
        Runtime missing(42);check(missing.start(100,{6,0,0},bar,policy,ranks,true,h)==casting::Reason::unsupported&&cost==0);
        h.status_mutation=[&](unsigned skill,unsigned,unsigned rank,auto now,auto until,bool add){check(skill>=6&&skill<=9);if(add){check(rank==15&&until==now+90000);++commits;}return true;};
        Runtime runtime(42);for(unsigned id=6;id<=9;++id)check(runtime.start(100,{id,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(commits==4&&cost==40);
    }
    std::cout<<"Elemental Mantras damage, Energy, replacement, duration and casting PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
