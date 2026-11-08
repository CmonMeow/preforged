#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Reversal reactive hex line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope source,target;
    bool npc;
    Fixture(bool is_npc,unsigned hp,unsigned energy=1,bool healing_known=true):npc(is_npc){
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
        n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),1000};
        d.spawn_energy=combat::npcs::Energy{energy,100};d.energy_pips=0;
        d.damageable=d.conditions_allowed=true;d.general_armor=60;d.general_armor_evidence="synthetic";
        if(healing_known){d.healing_allowed=true;d.healing_evidence="synthetic";}
        n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Hex owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        w.join(2,"Player foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(2,{hp,1000,energy,100},100);
        source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
        if(!npc)check(w.energy_capacity(2,5,0,100,{80,-2}));
        w.configure_attack_miss_draws([]{return .5;});
    }
    double hp()const{return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
    unsigned energy()const{return npc?w.npc_energy_snapshot(target)->current:w.resources(2)->energy;}
    void reversal(unsigned rank){
        check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));
    }
    bool has_reversal(unsigned now=200){const auto s=*w.status_snapshot(target,now);return std::any_of(s.begin(),s.end(),[](const auto& e){return e.skill==307;});}
};
double converted_hit(double hp,unsigned amount,unsigned rank,bool& reversal,unsigned reduction=0){
    if(hp<=0||!amount)return hp;
    const double reduced=std::max(0.,double(amount)-reduction);
    if(reversal&&reduced>0){
        reversal=false;const double heal=std::min(reduced,double(effects::reversal_fortune(rank)->maximum_healing));
        hp=std::min(1000.,hp+heal);return std::max(0.,hp-(reduced-heal));
    }
    return std::max(0.,hp-reduced);
}
int main()try {
    unsigned cases=0;
    // Targeted-use Energy loss and its separate zero burst must share one-use
    // Reversal authority. A raw-lethal first component can become survivable.
    for(bool npc:{false,true})for(bool pvp:{false,true})for(unsigned rank:{0u,12u,20u})
    for(unsigned health:{10u,500u,1000u})for(unsigned energy:{0u,1u,2u}){
        ++cases;Fixture f(npc,health,energy);f.reversal(rank);
        check(f.w.apply_mind_wrack(f.source,f.target,12,pvp,100));f.box->drain();
        check(f.w.mind_wrack_targeted({f.source,f.target,42,1,200,true}));
        bool rr=true;double hp=health;
        if(energy){hp=converted_hit(hp,pvp?0:21,rank,rr);if(energy==1)hp=converted_hit(hp,83,rank,rr);}
        check(std::abs(f.hp()-hp)<1.01&&f.energy()==(energy?energy-1:0)&&f.has_reversal()==rr);
        if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(hp==0));
        f.box->drain();
        check(!f.w.mind_wrack_targeted({f.source,f.target,42,1,201,true})&&f.box->drain().empty());
        check(std::abs(f.hp()-hp)<1.01);
    }
    check(cases==108);
    // A real player Energy cost can trigger the zero burst independently of a
    // targeted-use event. It must use the same damage conversion path.
    for(bool pvp:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned health:{10u,500u,1000u}){
        ++cases;Fixture f(false,health);f.reversal(rank);check(f.w.apply_mind_wrack(f.source,f.target,12,pvp,100));
        check(f.w.consume_energy(2,1,f.target,200));bool rr=true;
        const auto hp=converted_hit(health,83,rank,rr);check(std::abs(f.hp()-hp)<1.01&&!f.has_reversal());
    }
    // Flat damage reduction precedes conversion on each distinct component.
    for(bool pvp:{false,true})for(unsigned rank:{0u,12u,20u}){
        ++cases;Fixture f(false,500);f.reversal(rank);
        check(f.w.protective_enchantment(f.target,f.target,*skills::regeneration::reviewed(299,0),0,100));
        check(f.w.apply_mind_wrack(f.source,f.target,12,pvp,100));
        check(f.w.mind_wrack_targeted({f.source,f.target,42,1,200,true}));
        bool rr=true;auto hp=converted_hit(500,pvp?0:21,rank,rr,3);hp=converted_hit(hp,83,rank,rr,3);
        check(std::abs(f.hp()-hp)<1.01&&!f.has_reversal());
    }
    // Mark of Pain's adjacent recipient has not taken the physical trigger:
    // its Reversal must convert the reactive splash, not the primary's hit.
    for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned health:{10u,500u,1000u}){
        ++cases;Fixture f(npc,health);f.reversal(rank);
        f.w.join(3,"Physical primary",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        f.w.set_vitals(3,{1000,1000,100,100},100);const auto primary=*f.w.effect_scope(3);
        check(f.w.physical_armor(primary,60));
        check(f.w.degeneration_hex(f.source,primary,*effects::degeneration_hex(150,12),100));
        combat::weapon::Owned sword{3,1,{20,20,2},12,false,false,27};
        check(f.w.ordinary_weapon(f.source,sword,20)&&f.w.ordinary_attack(f.source,3,200));
        f.w.tick(700);f.w.cancel_ordinary_attack(f.source);
        bool rr=true;const auto hp=converted_hit(health,34,rank,rr);
        check(f.w.resources(3)->health==980&&std::abs(f.hp()-hp)<1.01&&!f.has_reversal(700));
        if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(hp==0));
    }
    // Healing can settle pending Deep Wound debt lethally. The later zero
    // burst must not strike a corpse or produce another death notification.
    for(bool npc:{false,true})for(bool pvp:{false,true}){
        ++cases;Fixture f(npc,10);
        if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));
            check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));}
        else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
        f.reversal(12);check(f.w.apply_mind_wrack(f.source,f.target,12,pvp,100));
        check(f.w.mind_wrack_targeted({f.source,f.target,42,1,200,true}));
        check(f.hp()==0&&!f.has_reversal());
        if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
    }
    // Unknown NPC healing authority cannot acquire a fabricated Reversal.
    {++cases;Fixture f(true,500,1,false);f.box->drain();
        check(!f.w.stage_npc_reversal(f.target,12,100));
        check(f.energy()==1&&f.hp()==500&&!f.has_reversal()&&f.box->drain().empty());
    }
    std::cout<<"PASS "<<cases<<" reactive Reversal cases; targeted/zero Energy bursts, flat protection, mixed Mark of Pain, exact events, unknown healing admission;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
