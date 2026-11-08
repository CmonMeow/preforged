#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("NPC ordinary World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,target;bool elemental;world::Instance::Npc appearance;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(bool multi,unsigned energy=0,unsigned health=500,float distance=100,bool fast=false):elemental(multi){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),health};
        d.damageable=true;d.earth_armor=60;d.conditions_allowed=true;d.knockdown_allowed=true;d.knockdown_evidence="authored fixture";
        d.spawn_energy=combat::npcs::Energy{energy,30};d.energy_pips=0;n.combat_definition=d;appearance=n;w.configure_npcs({n});
        w.join(1,"Target",distance,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        source=*w.effect_scope(100);target=*w.effect_scope(1);check(w.elemental_armor(target,60,60));
        combat::ai::FlarePolicy policy;policy.source=source;policy.level=20;policy.fire_magic=12;policy.speed=300;
        policy.acquire=2000;policy.follow=2500;policy.leash=2500;policy.flight={fast?10000.:100.,32};policy.evidence="authored caster fixture";
        if(multi)check(w.configure_npc_elemental_ai(policy,{{{194,0},*skills::effects::elemental(194,12),"native Flare, authored NPC loadout"}},4,1000));
        else check(w.configure_npc_flare_ai(policy,1000));
    }
    void fallback(){
        combat::ai::ordinary::Reviewed p{10,10,0,5,true,combat::ai::ordinary::Delivery::ranged,500,1500,1000,{100,32},
            "Authored fixed fire wand fixture; mixed ordinary/spell actions captured, numbers uncalibrated"};
        check(w.configure_npc_ordinary_ai(source,p,1000));check(!w.configure_npc_ordinary_ai(source,p,1000));
    }
    bool casting(){return elemental?w.npc_elemental_ai_snapshot(source)->pending.has_value():w.npc_flare_ai_snapshot(source)->cast.has_value();}
    std::uint64_t cast_end(){return elemental?w.npc_elemental_ai_snapshot(source)->pending->token.completes:w.npc_flare_ai_snapshot(source)->cast->completes;}
};
std::optional<float> last_npc_x(const std::vector<world::Packet>& packets){
    std::optional<float> x;
    for(const auto& p:packets)if(p.size()==16&&p[0]==44&&p[1]==0&&p[2]==100&&p[3]==0&&p[4]==0&&p[5]==0){
        const std::uint32_t bits=std::uint32_t(p[6])|(std::uint32_t(p[7])<<8)|(std::uint32_t(p[8])<<16)|(std::uint32_t(p[9])<<24);
        x=std::bit_cast<float>(bits);
    }
    return x;
}
int main()try{
    for(bool multi:{false,true}){
        {Fixture f(multi);f.w.tick(1000);f.w.tick(4000);check(!f.w.npc_ordinary_pending(f.source)&&f.w.resources(1)->health==1000);}
        {Fixture f(multi);f.fallback();f.box->drain();f.w.tick(1000);
         check(f.w.npc_ordinary_pending(f.source).has_value()&&!f.casting()&&f.w.npc_energy_snapshot(f.source)->current==0);
         const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),combat::weapon::timing_2022(100,1.5f,1.f))!=packets.end());
         check(std::find(packets.begin(),packets.end(),combat::weapon::start_2022(100,1))!=packets.end());
         f.w.tick(1400);check(f.w.resources(1)->health==1000);f.w.tick(1500);check(!f.w.npc_ordinary_pending(f.source));
         f.w.tick(2400);check(f.w.resources(1)->health==1000);f.w.tick(2500);check(f.w.resources(1)->health==990);
         const auto outcomes=f.w.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes[0].outcome==combat::weapon::AttackOutcome::hit&&outcomes[0].released==1500);
         f.w.tick(2500);check(f.w.resources(1)->health==990&&f.w.take_attack_outcomes(f.source).empty());}
        {Fixture f(multi,5);f.fallback();f.w.tick(1000);check(f.casting()&&!f.w.npc_ordinary_pending(f.source));
         const auto end=f.cast_end();f.w.tick(end);f.w.tick(end+650);check(!f.w.npc_ordinary_pending(f.source));
         f.w.tick(end+750);check(f.w.npc_ordinary_pending(f.source).has_value()&&f.w.npc_energy_snapshot(f.source)->current==0);
         check(multi?f.w.npc_elemental_ai_snapshot(f.source)->flights==1:f.w.npc_flare_ai_snapshot(f.source)->shots.size()==1);
         // The released spell must land even while the ordinary windup runs.
         f.w.tick(end+1000);check(f.w.resources(1)->health<1000&&f.w.npc_ordinary_pending(f.source).has_value());}
        {Fixture f(multi,0,20);f.fallback();check(f.w.spiteful_spirit(f.target,f.source,*skills::effects::spiteful_spirit(12),20,1000));
         f.w.tick(1000);f.w.tick(1500);check(f.w.npc_combat_snapshot(f.source)->health->current==0);
         f.w.tick(2500);check(f.w.resources(1)->health==990);const auto outcomes=f.w.take_attack_outcomes(f.source);
         check(outcomes.size()==1&&outcomes[0].outcome==combat::weapon::AttackOutcome::hit);}
        {Fixture f(multi);f.fallback();f.w.tick(1000);check(f.w.kill(f.target,1100));f.w.tick(1500);
         check(!f.w.npc_ordinary_pending(f.source)&&f.w.take_attack_outcomes(f.source).empty());
         const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),combat::weapon::stop_2022(100))!=packets.end());}
        {Fixture f(multi);f.fallback();f.w.tick(1000);
         check(f.w.earthquake(f.target,f.source,*skills::effects::earthquake(170,12),20,1100));
         f.w.tick(1500);check(!f.w.npc_ordinary_pending(f.source));f.w.tick(2500);
         check(f.w.resources(1)->health==1000&&f.w.take_attack_outcomes(f.source).empty());}
        {Fixture f(multi);f.fallback();f.w.tick(1000);f.w.tick(1500);check(f.w.settle_resources(f.target,3000));
         const auto outcomes=f.w.take_attack_outcomes(f.source);check(f.w.resources(1)->health==990&&outcomes.size()==1&&outcomes[0].at==2500);
         f.w.tick(2500);check(f.w.resources(1)->health==990&&f.w.take_attack_outcomes(f.source).empty());
         f.w.tick(3000);check(f.w.resources(1)->health==990&&f.w.take_attack_outcomes(f.source).empty());}
        {Fixture f(multi);f.fallback();f.w.tick(1000);f.w.tick(1500);const auto retired=f.source;
         f.w.leave(1);f.w.configure_npcs({f.appearance});check(*f.w.effect_scope(100)!=retired);
         f.w.join(1,"Replacement",100,0,f.box,0,{}, {},1600,1);f.w.set_vitals(1,{1000,1000,100,100},100);
         check(f.w.elemental_armor(*f.w.effect_scope(1),60,60));f.w.tick(2500);
         check(f.w.resources(1)->health==1000&&!f.w.npc_ordinary_pending(retired));
         check(f.w.take_attack_outcomes(*f.w.effect_scope(100)).empty());}
        {Fixture f(multi,5,500,1600,true);f.box->drain();
         for(std::uint64_t now=1000;now<=6000&&!f.casting();now+=100)f.w.tick(now);
         check(f.casting());const auto x=last_npc_x(f.box->drain());check(x&&*x>30);
         const auto end=f.cast_end();f.w.tick(end);f.box->drain();check(f.w.kill(f.target,end+1));
         f.w.tick(end+200);f.w.tick(end+600);check(!last_npc_x(f.box->drain()));
         f.w.tick(end+850);const auto returned=last_npc_x(f.box->drain());
         // Only100ms after the750ms aftercast boundary is spendable, even
         // though250ms elapsed since the previous World tick.
         check(returned&&std::abs(*returned-(*x-30.f))<.01f);}
    }
    std::cout<<"NPC ordinary World fallback passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
