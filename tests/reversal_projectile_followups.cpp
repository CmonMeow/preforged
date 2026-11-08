#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Projectile conversion followup line "+std::to_string(at.line()));
}
struct Fixture{
    world::Instance w;effects::Scope source,target;bool npc;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    std::optional<combat::npcs::control::Token> action;
    std::uint64_t first=0;
    Fixture(bool n,unsigned hp,std::optional<bool> knocks=true):npc(n){
        if(npc){
            world::Instance::Npc a;a.id=a.definition.id=100;a.x=600;a.definition.level=20;a.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,20};d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{double(hp),1000};d.damageable=d.conditions_allowed=true;
            d.earth_armor=d.cold_armor=d.physical_armor=60;d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
            d.healing_allowed=d.fleshy=true;d.healing_evidence=d.fleshy_evidence="synthetic known living recipient";
            d.knockdown_allowed=knocks;d.knockdown_evidence="synthetic reviewed control permission";
            a.combat_definition=d;w.configure_npcs({a});
        }
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);source=*w.effect_scope(1);
        check(w.physical_armor(source,60));check(w.configure_projectiles({1000,32}));
        if(!npc){w.join(2,"Target",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);}
        target=*w.effect_scope(npc?100:2);
        if(!npc)check(w.combat_armor(target,60)&&w.elemental_armor(target,{},60));
        box->drain();
    }
    double hp(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
    bool has(unsigned skill){const auto entries=*w.status_snapshot(target,first);return std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.skill==skill;});}
    void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,81,rank,100,8100,true));}
    void launch(unsigned skill,unsigned rank,unsigned mode){
        if(skill==172&&mode)check(w.activate_overcast_energy(source,219,0,1,200));
        if(skill==571&&mode){
            const unsigned protection=mode==2?16:mode==3?17:0;
            if(protection)check(npc?w.npc_interrupt_stance(target,protection,12,10000,100):w.status_mutation(target,protection,82,12,100,10100,true));
            if(npc){action=w.begin_npc_action(target,combat::npcs::control::Action::spell,1000,5000,skills::Slot{194,0});check(action.has_value());}
            else check(w.cast_activity(target,194,1000,5000));
        }
        if(skill==237&&mode){
            if(npc){
                if(mode==2){
                    w.join(3,"Nearby ally",610,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
                    w.set_vitals(3,{1000,1000,100,100},100);check(w.physical_armor(*w.effect_scope(3),60));
                }
                combat::ai::FlarePolicy p;p.source=target;p.level=20;p.speed=20;p.acquire=mode==2?128:1000;p.follow=1500;p.leash=2000;
                p.flight={1000,32};p.evidence="synthetic slow owned pursuit";
                check(w.configure_npc_hostile_ordinary(p,{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence},1000));
                w.tick(1000);
            }else w.move(2,900,0,900);
        }
        box->drain();
        switch(skill){
            case 571:check(w.launch_disrupting_dagger(source,target,*effects::disrupting_dagger(rank),20,1000));break;
            case 172:check(w.stone_daggers(source,target,*effects::stone_daggers(rank),20,1000));break;
            case 858:check(w.dancing_daggers(source,target,*effects::dancing_daggers(rank),20,1000));break;
            case 237:check(w.water_projectile(source,target,*effects::water_projectile(237,rank),20,1000));break;
            default:check(false);
        }
        for(const auto& p:box->drain())if(p.size()==29&&p[0]==163&&(p[1]&0x7f)==0){
            std::uint32_t bits=0;for(unsigned i=0;i<4;++i)bits|=std::uint32_t(p[16+i])<<(8*i);
            first=1000+std::uint64_t(std::llround(std::bit_cast<float>(bits)*1000));break;
        }
        check(first>1000);w.tick(first);
    }
};
int main()try{
    unsigned cases=0;
    for(unsigned skill:{571u,172u,858u,237u})for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u}){
        const unsigned modes=skill==571?4:skill==858?1:skill==237&&npc?3:2;
        for(unsigned mode=0;mode<modes;++mode){
            Fixture baseline(npc,1000);baseline.launch(skill,rank,mode);const double raw=skill==571?effects::disrupting_dagger(rank)->earth_damage:skill==172?effects::stone_daggers(rank)->damage:skill==858?effects::dancing_daggers(rank)->damage:effects::water_projectile(237,rank)->damage;check(raw>0);
            for(int rr:{-1,0,12,20})for(unsigned initial:{1u,10u,500u,1000u}){
                Fixture f(npc,initial);if(rr>=0)f.reversal(rr);f.launch(skill,rank,mode);
                const double converted=rr<0?0:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));
                const double expected=std::max(0.,std::min(1000.,initial+converted)-(raw-converted));
                if(std::abs(f.hp()-(npc?expected:std::ceil(expected)))>.001)std::cerr<<"skill="<<skill<<" npc="<<npc<<" rank="<<rank<<" mode="<<mode<<" rr="<<rr<<" hp="<<initial<<" actual="<<f.hp()<<" expected="<<expected<<'\n';
                check(std::abs(f.hp()-(npc?expected:std::ceil(expected)))<.001&&!f.has(307));
                if(skill==172)check(f.w.has_condition(f.target,combat::conditions::Kind::bleeding,f.first)==(mode&&expected>0));
                if(skill==858)check(f.w.dagger_stage(f.source,f.target,f.first)==unsigned(expected>0));
                if(skill==237)check(f.w.knocked_down(f.target,f.first)==(mode==1&&expected>0));
                if(skill==571){
                    const bool protected_action=mode>=2;
                    if(expected>0){
                        const unsigned energy=npc?f.w.npc_energy_snapshot(f.target)->current:f.w.resources(2)->energy;
                        check(energy==(npc?40u:100u)-(mode==3?effects::interpolate(10,4,12):0));
                        if(!npc)check(bool(f.w.casting_skill(f.target,f.first))==(mode&&protected_action));
                    }
                    if(f.action)check(f.w.complete_npc_action(*f.action,5000)==(expected>0&&protected_action));
                }
                if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(expected<=0));
                ++cases;
            }
        }
    }
    // Healing-caused death cannot grant a condition, chain or knockdown.
    for(unsigned skill:{571u,172u,858u,237u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
        Fixture f(npc,10);f.reversal(rr);
        check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));
        check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));
        f.launch(skill,12,skill==858?0:1);check(f.hp()==0&&!f.has(307));
        check(!f.w.has_condition(f.target,combat::conditions::Kind::bleeding,f.first));
        check(!f.w.knocked_down(f.target,f.first));check(f.w.dagger_stage(f.source,f.target,f.first)==0);
        if(npc)check(f.w.take_npc_deaths(f.source).size()==1);++cases;
    }
    // Disrupting Dagger interrupts a skill, not a plain ordinary attack.
    for(unsigned rr:{0u,12u,20u})for(unsigned protection:{0u,16u,17u}){
        Fixture f(true,500);f.reversal(rr);
        if(protection)check(f.w.npc_interrupt_stance(f.target,protection,12,10000,100));
        const auto action=f.w.begin_npc_action(f.target,combat::npcs::control::Action::attack,1000,5000);check(action.has_value());
        f.launch(571,12,0);check(f.w.npc_energy_snapshot(f.target)->current==40);
        if(protection)check(f.has(protection));check(f.w.complete_npc_action(*action,5000));++cases;
    }
    // A known immune moving NPC takes damage but cannot be knocked down.
    for(int rr:{-1,0,12,20}){
        Fixture f(true,500,false);if(rr>=0)f.reversal(rr);f.launch(237,12,1);
        check(!f.w.knocked_down(f.target,f.first)&&!f.has(307));++cases;
    }
    // Later daggers cannot reuse the first projectile's consumed conversion.
    for(unsigned skill:{172u,858u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
        Fixture f(npc,500);f.reversal(rr);f.launch(skill,12,0);
        const double damage=skill==172?effects::stone_daggers(12)->damage:effects::dancing_daggers(12)->damage;
        const double converted=std::min(damage,double(effects::reversal_fortune(rr)->maximum_healing));
        f.w.tick(3000);const auto expected=500+converted-(damage-converted)-(skill==172?1:2)*damage;
        check(f.hp()==expected&&!f.has(307));f.w.tick(3100);check(f.hp()==expected);++cases;
    }
    std::cout<<cases<<" projectile conversions: player/NPC survival, protected interrupts, Bleeding, lead chain, moving/immunity knockdown and per-projectile consumption passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
