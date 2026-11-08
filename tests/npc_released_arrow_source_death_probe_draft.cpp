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
// Independent same-Scope death strictly BETWEEN actual launch and impact.
// Current ledger documents this contract already; this is a BEFORE probe,
// not an assertion that a production repair is needed.
int main()try{
 for(unsigned mode=0;mode<4;++mode){
  Fixture f(false);check(f.w.physical_armor(f.target,60));
  combat::ai::ordinary::Reviewed profile{10,10,12,0,false,combat::ai::ordinary::Delivery::ranged,500,1500,1000,{100,32},
   "Authored fixed physical arrow; lifetime behavior evidenced by2026 Archer receipts; numbers not retail calibration"};
  check(f.w.configure_npc_ordinary_ai(f.source,profile,1000));
  f.w.tick(1000);check(f.w.npc_ordinary_pending(f.source).has_value());
  if(mode==0){
   check(f.w.damage_npc(f.target,f.source,{combat::Kind::armor_ignoring,5000,20,194},1248,1200).killed);
   f.w.tick(1500);f.w.tick(2500);check(f.w.resources(1)->health==1000);
   check(!f.w.npc_ordinary_pending(f.source));
  }else{
   f.w.tick(1500);check(!f.w.npc_ordinary_pending(f.source)&&f.w.resources(1)->health==1000);
   const auto launched_source=f.source,launched_target=f.target;
   if(mode==1){
    check(f.w.damage_npc(f.target,f.source,{combat::Kind::armor_ignoring,5000,20,194},1248,1700).killed);
    check(f.w.effect_scope(100)==std::optional{launched_source});
    check(f.w.npc_combat_snapshot(f.source)->health->current==0);
    f.w.tick(2400);check(f.w.resources(1)->health==1000);f.w.tick(2500);
    check(f.w.resources(1)->health==990);
    const auto outcomes=f.w.take_attack_outcomes(launched_source);
    check(outcomes.size()==1&&outcomes[0].outcome==combat::weapon::AttackOutcome::hit&&outcomes[0].released==1500&&outcomes[0].at==2500);
    f.w.tick(2500);f.w.tick(2600);check(f.w.resources(1)->health==990&&f.w.take_attack_outcomes(launched_source).empty());
    std::cout<<"released_dead_source_hit source="<<launched_source.actor<<" generation="<<launched_source.generation<<" health=990 once=1\n";
   }else if(mode==2){
    f.w.leave(1);f.w.join(1,"New target",100,0,f.box,0,{}, {},1700,1);f.w.set_vitals(1,{1000,1000,100,100},100);
    check(*f.w.effect_scope(1)!=launched_target);check(f.w.physical_armor(*f.w.effect_scope(1),60));
    f.w.tick(2500);check(f.w.resources(1)->health==1000);
   }else{
    f.w.leave(1);f.w.configure_npcs({f.appearance});check(*f.w.effect_scope(100)!=launched_source);
    f.w.join(1,"New actors",100,0,f.box,0,{}, {},1700,1);f.w.set_vitals(1,{1000,1000,100,100},100);
    check(f.w.physical_armor(*f.w.effect_scope(1),60));f.w.tick(2500);check(f.w.resources(1)->health==1000);
    check(f.w.take_attack_outcomes(*f.w.effect_scope(100)).empty());
   }
  }
 }
 std::cout<<"PASS NPC released-arrow lifetime actual launch/death/impact, pending death and retired scopes; player Bow untouched\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
