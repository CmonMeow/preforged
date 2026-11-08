#include "gw_world_simulation.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Reversal spell families line "+std::to_string(at.line()));
}
struct Fixture{
    world::Instance w;effects::Scope source,target;
    Fixture(unsigned hp,unsigned rank,unsigned maximum=1000){
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{1000,1000,100,100},100);w.set_vitals(2,{hp,maximum,100,100},100);
        source=*w.effect_scope(1);target=*w.effect_scope(2);w.public_professions(1,6,3);
        check(w.combat_armor(target,60)&&w.elemental_armor(target,60,60)&&w.lightning_armor(target,60));
        check(w.status_mutation(target,307,5,rank,100,8100,true));
    }
    void health(unsigned skill,double expected){const auto actual=w.resources(2)->health;
        if(actual!=static_cast<unsigned>(std::ceil(expected)))throw std::runtime_error("skill="+std::to_string(skill)+" actual="+std::to_string(actual)+" expected="+std::to_string(expected));}
    bool reversal(unsigned at){const auto entries=*w.status_snapshot(target,at);return std::any_of(entries.begin(),entries.end(),[](auto e){return e.skill==307;});}
};
int main()try{
    unsigned elemental_cases=0,lightning_cases=0;
    for(unsigned skill:{89u,1227u,170u,1086u,1068u,866u,145u,3059u,163u,188u,1081u,845u,194u,183u,186u,189u,191u,1379u,884u,2192u,1095u,846u})
    for(unsigned rank:{0u,12u})for(unsigned hp:{10u,300u,1000u})for(bool burning:{false,true}){
        const auto plan=*effects::elemental(skill,12,0);if(plan.projectile&&burning)continue;
        ++elemental_cases;Fixture f(hp,rank);if(burning)check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::burning,100,10000));
        const bool no_damage=plan.burn_or_damage&&!burning;double expected=hp;
        const double incoming=(plan.damage+(burning?plan.bonus_if_burning:0))*(plan.half_if_enchanted?.5:1.);
        if(!no_damage){const double converted=std::min(incoming,rank==0?15.:67.);
            expected=std::max(0.,std::min(1000.,expected+converted)-(incoming-converted));
            if(expected>0&&hp>500&&plan.shadow_above_half)expected=std::max(0.,expected-plan.shadow_above_half);}
        effects::ElementalOutcome outcome;std::optional<skills::sacrifice::Loss> paid;std::vector<world::Packet> visuals;
        if(!f.w.elemental(f.source,plan.caster_center?f.source:f.target,plan,20,100,&visuals,&paid,&outcome))throw std::runtime_error("elemental admission="+std::to_string(skill));
        if(plan.projectile)f.w.tick(1000);
        f.health(skill,expected);const unsigned at=plan.projectile?1000:100;
        check(f.reversal(at)==no_damage);
        check(f.w.has_condition(f.target,combat::conditions::Kind::burning,at)==(expected>0&&(burning||plan.burning_ms)));
        check(f.w.has_condition(f.target,combat::conditions::Kind::blind,at)==(expected>0&&burning&&plan.blind_if_burning_ms));
        if(skill==170||skill==1086)check(f.w.knocked_down(f.target,at)==(expected>0));
    }
    check(elemental_cases==252);
    for(unsigned skill:{224u,228u,1369u,1082u,229u,865u,3396u,2059u,842u,1664u,232u})
    for(unsigned rank:{0u,12u})for(unsigned hp:{10u,300u,1000u}){
        ++lightning_cases;Fixture f(hp,rank);const auto plan=*effects::lightning(skill,12,0);
        const double incoming=plan.damage*std::exp2((60-60*(1.-double(plan.penetration)/100))/40.);
        const double converted=std::min(incoming,rank==0?15.:67.);
        const double expected=std::max(0.,std::min(1000.,hp+converted)-(incoming-converted));
        std::vector<world::Packet> visuals;if(!f.w.lightning(f.source,f.target,plan,20,100,&visuals))throw std::runtime_error("lightning admission="+std::to_string(skill));
        if(plan.projectile)f.w.tick(1000);const unsigned at=plan.projectile?1000:100;
        f.health(skill,expected);check(!f.reversal(at));
        check(f.w.has_condition(f.target,combat::conditions::Kind::weakness,at)==(expected>0&&plan.weakness_ms));
        check(f.w.has_condition(f.target,combat::conditions::Kind::blind,at)==(expected>0&&plan.blind_ms));
        check(f.w.has_condition(f.target,combat::conditions::Kind::cracked_armor,at)==(expected>0&&plan.cracked_ms));
    }
    check(lightning_cases==66);
    // This first raw hit is lethal,but conversion saves the actor. The second
    // component must still execute,and must not get a second conversion.
    for(unsigned hp:{10u,60u}){Fixture f(hp,12,100);effects::ElementalOutcome outcome;
        check(f.w.elemental(f.source,f.target,*effects::elemental(89,20),20,100,nullptr,nullptr,&outcome));
        f.health(89,hp==60?35:75);check(!f.reversal(100));}
    // A moving target receives both Lightning Bolt strikes even when the
    // first unconverted proposal would have killed it.
    {Fixture f(10,12);f.w.move(2,600,0,100);std::vector<world::Packet> visuals;
        check(f.w.lightning(f.source,f.target,*effects::lightning(1369,12),20,100,&visuals));f.w.tick(1000);
        const double raw=41*std::exp2(15./40.);f.health(1369,(10+raw)-raw);check(!f.reversal(1000));}
    unsigned seed_cases=0;
    for(unsigned mode=0;mode<3;++mode){++seed_cases;world::Instance w;
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*40),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{id==2?10u:500u,1000,100,100},100);}
        const auto monk=*w.effect_scope(1),target=*w.effect_scope(2),attacker=*w.effect_scope(3);
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
        check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));check(w.lightning_armor(target,60));
        if(mode)check(w.status_mutation(target,307,5,mode==1?0:12,100,8100,true));
        check(w.lightning(attacker,target,*effects::lightning(224,12),20,100));
        check(w.resources(1)->health==(mode==2?500:525));check((w.resources(2)->health>0)==(mode==2));
    }
    check(seed_cases==3);
    std::cout<<"252 elemental/66 lightning Reversal World cases,2 shadow-order moving double-bolt and3 Seed-order cases PASS;307 paid casts remain unbound\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
