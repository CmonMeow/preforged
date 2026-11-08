#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("NPC replacement World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;skills::effects::Scope source,target;bool elemental;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(bool use_elemental,unsigned energy=4,int pips=3,unsigned health=500):elemental(use_elemental){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),health};d.damageable=true;
        d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{energy,30};d.energy_pips=pips;n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Target",100,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        source=*w.effect_scope(100);target=*w.effect_scope(1);check(w.elemental_armor(target,60,60));
        combat::ai::FlarePolicy p;p.source=source;p.level=20;p.fire_magic=12;p.speed=300;p.acquire=1000;p.follow=1500;p.leash=1500;
        p.flight={100,32};p.evidence="synthetic regeneration/replacement fixture; authored resources and ordinary profile";
        if(elemental)check(w.configure_npc_elemental_ai(p,{{{194,0},*skills::effects::elemental(194,12),p.evidence}},4,1000));
        else check(w.configure_npc_flare_ai(p,1000));
        combat::ai::ordinary::Reviewed profile{10,10,0,5,true,combat::ai::ordinary::Delivery::ranged,
            2000,2000,1000,{100,32},p.evidence};check(w.configure_npc_ordinary_ai(source,profile,1000));box->drain();
    }
    std::optional<combat::npcs::control::Token> spell(){
        if(elemental){const auto s=w.npc_elemental_ai_snapshot(source);return s&&s->pending?std::optional(s->pending->token):std::nullopt;}
        return w.npc_flare_ai_snapshot(source)->cast;
    }
};
bool start(const world::Packet& p){
    const auto word=[&](std::size_t i){return std::uint32_t(p[i])|std::uint32_t(p[i+1])<<8|
        std::uint32_t(p[i+2])<<16|std::uint32_t(p[i+3])<<24;};
    return p.size()==18&&p[0]==159&&p[1]==0&&word(2)==60&&word(6)==100&&word(10)==1&&word(14)==194;
}
int main()try{
    for(bool elemental:{false,true}){
        {Fixture f(elemental);f.w.tick(1000);const auto ordinary=*f.w.npc_ordinary_pending(f.source);check(!f.spell());f.box->drain();
            f.w.tick(2000);const auto spell=f.spell();check(spell&&spell->begins==2000&&spell->epoch!=ordinary.token.epoch);
            check(!f.w.npc_ordinary_pending(f.source)&&f.w.npc_energy_snapshot(f.source)->current==0);
            const auto sent=f.box->drain();const auto stop=combat::weapon::stop_2022(100);
            check(std::count(sent.begin(),sent.end(),stop)==1);
            const auto a=std::find(sent.begin(),sent.end(),stop),b=std::find_if(sent.begin(),sent.end(),start);
            check(a!=sent.end()&&b!=sent.end()&&a<b);
            check(!f.w.complete_npc_action(ordinary.token,3000)&&f.w.take_attack_outcomes(f.source).empty());
            f.w.tick(3000);check(f.w.resources(1)->health==1000);f.w.tick(4000);check(f.w.resources(1)->health<1000);
            check(f.w.take_attack_outcomes(f.source).empty());}
        {Fixture f(elemental,0,0);f.w.tick(1000);const auto ordinary=*f.w.npc_ordinary_pending(f.source);f.box->drain();
            f.w.tick(2000);check(!f.spell()&&f.w.npc_ordinary_pending(f.source)->token==ordinary.token);
            const auto sent=f.box->drain();check(std::count(sent.begin(),sent.end(),combat::weapon::stop_2022(100))==0);}
        {Fixture f(elemental,4,3,1);check(f.w.apply_mind_wrack(f.target,f.source,12,false,1000));
            f.w.tick(1000);check(f.w.npc_ordinary_pending(f.source).has_value());f.box->drain();
            f.w.tick(2000);check(f.w.npc_combat_snapshot(f.source)->health->current==0&&f.w.npc_energy_snapshot(f.source)->current==0);
            check(!f.spell()&&!f.w.npc_ordinary_pending(f.source));const auto sent=f.box->drain();
            check(std::count(sent.begin(),sent.end(),combat::weapon::stop_2022(100))==1);
            check(std::none_of(sent.begin(),sent.end(),start));f.w.tick(4000);
            check(f.w.resources(1)->health==1000&&f.w.take_attack_outcomes(f.source).empty());}
        {Fixture f(elemental,6,3);f.w.tick(1000);check(f.spell().has_value());f.w.tick(2000);f.w.tick(2750);
            check(f.w.npc_ordinary_pending(f.source).has_value());f.w.tick(4750);check(!f.w.npc_ordinary_pending(f.source));
            f.w.tick(4850);check(f.w.npc_ordinary_pending(f.source).has_value());f.w.tick(5000);
            check(f.spell().has_value()&&!f.w.npc_ordinary_pending(f.source));
            check(f.w.settle_resources(f.target,5749));const auto before=f.w.resources(1)->health;
            check(f.w.settle_resources(f.target,5750));check(f.w.resources(1)->health+10==before);
            const auto outcomes=f.w.take_attack_outcomes(f.source);
            check(outcomes.size()==1&&outcomes[0].outcome==combat::weapon::AttackOutcome::hit&&outcomes[0].released==4750);}
    }
    std::cout<<"NPC same-tick spell replacement, Energy reactions and retained flights passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
