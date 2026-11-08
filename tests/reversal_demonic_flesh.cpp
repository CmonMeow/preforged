#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Demonic NPC conversion line "+std::to_string(at.line()));
}
struct Fixture{
    world::Instance w;effects::Scope owner,target,npc;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    Fixture(unsigned hp){
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=10;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=d.conditions_allowed=true;
        d.spawn_health=combat::Health{double(hp),1000};d.healing_allowed=true;d.healing_evidence="synthetic reviewed NPC healing";
        n.combat_definition=d;w.configure_npcs({n});
        for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
            w.set_vitals(id,{1000,1000,100,100},100);}
        owner=*w.effect_scope(1);target=*w.effect_scope(2);npc=*w.effect_scope(100);
    }
    void apply(unsigned rank=12){check(w.demonic_flesh(owner,1,*effects::demonic_flesh(rank),1000));}
    bool use(unsigned event=1,unsigned at=2000){return w.demonic_flesh_skill_used({owner,target,194,event,at,true,false},20);}
    double hp(){return w.npc_combat_snapshot(npc)->health->current;}
    bool reversal(unsigned now){const auto entries=*w.status_snapshot(npc,now);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
};
int main()try{
    unsigned cases=0;
    for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,500u,1000u}){
        Fixture f(hp);f.apply(rank);const double raw=effects::demonic_flesh(rank)->damage;
        if(rr>=0)check(f.w.stage_npc_reversal(f.npc,rr,1000));f.box->drain();check(f.use());
        const double converted=rr<0?0:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));
        const double expected=std::max(0.,std::min(1000.,hp+converted)-(raw-converted));
        check(f.hp()==expected&&!f.reversal(2000));check(f.w.resources(2)->health==1000);
        check(!f.use());f.w.tick(2100);check(f.hp()==expected);
        const auto deaths=f.w.take_npc_deaths(f.owner);check(deaths.size()==unsigned(expected<=0));
        f.w.tick(2200);check(f.w.take_npc_deaths(f.owner).empty());++cases;
    }
    for(unsigned rr:{0u,12u,20u}){
        Fixture f(500);f.apply();check(f.w.stage_npc_shield_absorption(f.npc,12,1000));
        check(f.w.stage_npc_reversal(f.npc,rr,1000));check(f.use());
        check(f.hp()==512&&!f.reversal(2000));check(f.use(2,2100));check(f.hp()==505);++cases;
    }
    for(unsigned rr:{0u,12u,20u}){
        Fixture f(10);f.apply();check(f.w.stage_npc_reversal(f.npc,rr,1000));
        check(f.w.apply_condition(f.owner,f.owner,combat::conditions::Kind::deep_wound,1000,10000));
        check(f.w.condition_transfer(f.owner,f.npc,*effects::condition_transfer(132,12),1000));
        check(f.use()&&f.hp()==0&&!f.reversal(2000));check(f.w.take_npc_deaths(f.owner).size()==1);
        f.w.tick(2100);check(f.w.take_npc_deaths(f.owner).empty());++cases;
    }
    for(unsigned mode=0;mode<3;++mode){
        Fixture f(500);f.apply();const auto receipt=f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);
        check(receipt&&receipt->id);check(f.w.stage_npc_reversal(f.npc,12,5000));
        check(f.w.commit_demonic_flesh_use(*receipt));f.w.tick(2000);check(f.hp()==500);
        if(mode==1)check(f.w.kill(f.owner,2100));
        if(mode==2)f.w.leave(1);
        f.w.tick(5000);check(f.hp()==(mode==2?500:517));
        if(mode!=2){check(!f.reversal(5000)&&!f.w.commit_demonic_flesh_use(*receipt));}
        f.w.tick(5100);check(f.hp()==(mode==2?500:517));++cases;
    }
    {Fixture f(500);f.apply();const auto receipt=*f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);
        check(f.w.stage_npc_reversal(f.npc,0,2000));check(f.w.stage_npc_reversal(f.npc,20,2100));
        check(f.w.commit_demonic_flesh_use(receipt));f.w.tick(2200);
        check(f.hp()==517&&!f.reversal(2200));++cases;}
    std::cout<<cases<<" NPC Demonic Flesh conversions: current defenses, one-use delivery, deferred clocks, replacement, caster death/departure and healing-caused death\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
