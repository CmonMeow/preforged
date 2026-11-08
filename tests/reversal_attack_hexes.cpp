#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Attack hex Reversal line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;bool npc;
    effects::Scope source,target,bonded;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(bool is_npc,unsigned hp=500,unsigned energy=20):npc(is_npc){
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;
        d.spawn_energy=combat::npcs::Energy{energy,100};d.energy_pips=0;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:700,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==2?hp:1000u,1000,id==2?energy:100u,100},100);}
        source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);bonded=*w.effect_scope(3);
        if(npc){check(w.elemental_armor(bonded,60,60));check(w.stage_npc_bonds_of_torment(target,bonded,*effects::bonds_of_torment_npc(),100));}
        else check(w.energy_capacity(2,5,0,100,{80,-2}));
    }
    double hp()const{return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
    unsigned energy()const{return npc?w.npc_energy_snapshot(target)->current:w.resources(2)->energy;}
    void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
    bool has_reversal(unsigned at=200){const auto s=*w.status_snapshot(target,at);return std::any_of(s.begin(),s.end(),[](const auto& e){return e.skill==307;});}
    void hex(unsigned skill,unsigned rank=12){check(w.degeneration_hex(source,target,*effects::degeneration_hex(skill,rank),100));}
    void source_state(unsigned state){if(state==1)check(w.kill(source,100));if(state>=2)w.leave(1);if(state==3){w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);}}
    void attack(unsigned skill,bool alive,unsigned now=200){
        if(npc){const auto token=w.begin_npc_action(target,combat::npcs::control::Action::attack,now,now+400);check(token.has_value());check(w.complete_npc_action(*token,now+400)==(skill!=43&&alive));}
        else check(w.attack_activity(target,now+1000,now)==(skill!=43&&alive));
    }
};
double converted(unsigned hp,unsigned damage,int rr){const double heal=rr<0?0:std::min(damage,effects::reversal_fortune(rr)->maximum_healing);return std::max(0.,std::min(1000.,hp+heal)-(damage-heal));}
int main()try {
    unsigned cases=0;
    for(bool npc:{false,true})for(unsigned skill:{43u,47u})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(unsigned state:{0u,1u,2u,3u}){
        ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);f.hex(skill,rank);f.source_state(state);
        const auto amount=effects::attack_hex(skill,rank)->damage;const auto heal=rr<0?0:std::min(amount,effects::reversal_fortune(rr)->maximum_healing);
        const double healed=std::min(1000.,hp+double(heal)),loss=std::min(healed,double(amount-heal)),wanted=healed-loss;
        f.attack(skill,wanted>0);check(std::abs(f.hp()-wanted)<1.01&&!f.has_reversal(600));
        if(wanted>0)check(f.w.has_condition(f.target,combat::conditions::Kind::blind,600)==(skill==47));
        if(npc){check(std::abs(double(f.w.resources(3)->health)-(1000-loss))<1.01);if(state<2)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));}
        if(wanted>0){const auto before=f.hp();f.attack(47,true,700);check(f.hp()==before);} // One-shot hex was consumed.
        if(state==3)check(f.w.resources(1)->health==1000);
    }
    check(cases==576);
    // Documented player Clumsiness anomaly: protection pays its cost, yet
    // interruption still occurs independently of converted damage survival.
    for(unsigned stance:{16u,17u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
        ++cases;Fixture f(false,hp);if(rr>=0)f.reversal(rr);f.hex(43);
        check(f.w.status_mutation(f.target,stance,950,12,100,10100,true));const auto wanted=converted(hp,effects::attack_hex(43,12)->damage,rr);
        f.attack(43,wanted>0);check(std::abs(f.hp()-wanted)<1.01);if(wanted>0&&stance==17)check(f.energy()==15);
    }
    // Two attack hexes share one conversion: the second does not reuse it.
    for(bool npc:{false,true})for(int rr:{-1,0,12,20}){
        ++cases;Fixture f(npc);if(rr>=0)f.reversal(rr);f.hex(43);f.hex(47);
        const auto first=converted(500,effects::attack_hex(43,12)->damage,rr),wanted=first-effects::attack_hex(47,12)->damage;
        f.attack(43,true);check(f.hp()==wanted&&!f.has_reversal(600));
        if(npc){const auto heal=rr<0?0:std::min(76u,effects::reversal_fortune(rr)->maximum_healing);check(f.w.resources(3)->health==1000-(76-heal)-114);}
    }
    // Spirit Shackles is Energy loss, never a damage-conversion input. A real
    // zero may independently trigger Mind Wrack and then consume Reversal.
    for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(bool wrack:{false,true}){
        ++cases;Fixture f(npc,500,wrack?5:20);f.reversal(rr);f.hex(66);
        if(wrack)check(f.w.apply_mind_wrack(f.source,f.target,12,false,100));
        f.attack(66,true);check(f.energy()==(wrack?0u:15u));check(f.hp()==(wrack?converted(500,83,rr):500));check(f.has_reversal(600)==!wrack);
    }
    // A conversion-healing death carries no fabricated damaging Hit; the World
    // still receives its skill/ownership identity and publishes one death.
    for(bool npc:{false,true})for(unsigned skill:{43u,47u})for(unsigned rr:{0u,12u,20u}){
        ++cases;Fixture f(npc,10);if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));}
        else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
        f.reversal(rr);f.hex(skill);f.attack(skill,false);check(f.hp()==0&&!f.has_reversal(600));
        if(npc){check(f.w.take_npc_deaths(f.source).size()==1&&f.w.resources(3)->health==1000);}
    }
    check(cases==632);
    std::cout<<"PASS "<<cases<<" Clumsiness/Ineptitude Reversal cases; player/NPC conversion receipts,one-shot stacks,actual Bonds,player protection anomaly,Spirit Shackles non-damage and healing death;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
