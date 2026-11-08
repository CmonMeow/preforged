#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bonds Reversal line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned offset){unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(p[offset+i])<<(8*i);return n;}
bool cold_loss(const world::Packet& p,unsigned actor){return p.size()==18&&p[0]==162&&word(p,2)==16&&word(p,6)==actor&&std::bit_cast<float>(word(p,14))<0;}
struct Fixture {
    world::Instance w;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope source,npc,victim,other;
    Fixture(unsigned hp=500,unsigned npc_hp=1000,int armor=60){
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(npc_hp),1000};d.damageable=d.conditions_allowed=true;
        d.lightning_armor=60;d.lightning_armor_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?700:800,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==2?hp:500u,id==2?1000u:500u,100,100},100);}
        source=*w.effect_scope(1);npc=*w.effect_scope(100);victim=*w.effect_scope(2);other=*w.effect_scope(3);
        check(w.elemental_armor(victim,60,armor)&&w.elemental_armor(other,60,60));
        check(w.stage_npc_bonds_of_torment(npc,victim,*effects::bonds_of_torment_npc(),100));
        check(w.stage_npc_bonds_of_torment(npc,other,*effects::bonds_of_torment_npc(),100));
    }
    void reversal(unsigned rank){check(w.status_mutation(victim,307,900,rank,100,8100,true));}
    bool has_reversal(unsigned at=200){const auto entries=*w.status_snapshot(victim,at);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
    combat::Hit hit(unsigned amount,unsigned at=200){return w.damage_npc(source,npc,{combat::Kind::armor_ignoring,double(amount),20,42},1248,at);}
};
double incoming(unsigned amount,int armor,unsigned reduction){return std::max(0.,amount*std::exp2((60.-armor)/40.)-reduction);}
double converted(unsigned hp,double raw,unsigned rank){const double heal=std::min(raw,double(effects::reversal_fortune(rank)->maximum_healing));return std::max(0.,std::min(1000.,hp+heal)-(raw-heal));}
int main()try {
    unsigned cases=0;
    for(unsigned rank:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(unsigned amount:{10u,72u,200u})
    for(bool lethal_caster:{false,true})for(int armor:{40,60,100})for(unsigned reduction:{0u,3u}){
        ++cases;Fixture f(hp,lethal_caster?amount:1000,armor);
        if(reduction)check(f.w.protective_enchantment(f.victim,f.victim,*skills::regeneration::reviewed(299,0),0,100));
        f.reversal(rank);f.box->drain();const auto hit=f.hit(amount);check(hit.reason==combat::Reason::accepted&&hit.applied==amount&&hit.killed==lethal_caster);
        const double raw=incoming(amount,armor,reduction),heal=std::min(raw,double(effects::reversal_fortune(rank)->maximum_healing));
        const auto wanted=converted(hp,raw,rank);check(std::abs(double(f.w.resources(2)->health)-wanted)<1.01&&!f.has_reversal());
        check(f.w.resources(3)->health==500-amount); // Independent link, no conversion.
        const auto packets=f.box->drain();unsigned losses=0;
        for(const auto& p:packets)if(cold_loss(p,2)){++losses;check(word(p,10)==100);check(std::abs(double(std::bit_cast<float>(word(p,14)))+(raw-heal)/1000)<1e-6);}
        check(losses==unsigned(raw>heal));
        check(f.w.take_npc_deaths(f.source).size()==unsigned(lethal_caster));
        if(!lethal_caster&&wanted>0){check(f.hit(10,300).reason==combat::Reason::accepted);
            check(std::abs(double(f.w.resources(2)->health)-std::max(0.,wanted-incoming(10,armor,reduction)))<1.01);}
    }
    check(cases==324);
    // Fully reduced damage retains Reversal under the current explicit zero
    // input policy. The unrelated second recipient still takes its own hit.
    {++cases;Fixture f;check(f.w.protective_enchantment(f.victim,f.victim,*skills::regeneration::reviewed(299,0),0,100));f.reversal(12);
        check(f.hit(3).applied==3&&f.w.resources(2)->health==500&&f.has_reversal()&&f.w.resources(3)->health==497);}
    // Healing debt kills the first recipient without a false damage modifier;
    // the remaining captured propagation intent must still run once.
    for(bool lethal_caster:{false,true})for(unsigned rank:{0u,12u,20u}){
        ++cases;Fixture f(10,lethal_caster?200:1000);check(f.w.apply_condition(f.source,f.victim,combat::conditions::Kind::deep_wound,100,10000));f.reversal(rank);f.box->drain();
        check(f.hit(200).applied==200&&f.w.resources(2)->health==0&&!f.has_reversal()&&f.w.resources(3)->health==300);
        const auto packets=f.box->drain();check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return cold_loss(p,2);}));
        check(f.w.take_npc_deaths(f.source).size()==unsigned(lethal_caster));
    }
    // Thunderclap's completion vector must retain the converted remaining
    // retaliation, not the original raw cold hit. Heal/removal timing remains
    // the shared deferred-publication policy and needs rendered verification.
    {++cases;Fixture f;f.reversal(0);f.box->drain();std::vector<world::Packet> completion;
        check(f.w.lightning(f.source,f.npc,*effects::lightning(228,12,0),20,200,&completion));
        const auto loss=1000-f.w.npc_combat_snapshot(f.npc)->health->current;
        check(std::abs(double(f.w.resources(2)->health)-converted(500,loss,0))<1.01);
        check(std::any_of(completion.begin(),completion.end(),[](const auto& p){return cold_loss(p,2);}));
        const auto immediate=f.box->drain();check(std::none_of(immediate.begin(),immediate.end(),[](const auto& p){return cold_loss(p,2);}));
    }
    check(cases==332);
    std::cout<<"PASS "<<cases<<" Bonds Reversal cases; armor/protection first, independent links, lethal caster, healing death, one-use, zero policy and deferred remaining hit;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
