#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <fstream>
#include <iomanip>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Javelin conversion World line "+std::to_string(at.line()));
}
world::Instance::Npc actor(unsigned id,unsigned hp,bool hostile=true){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?100:120;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding={146,id,id,0,0,0,0,20};
    d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
    d.spawn_health=combat::Health{double(hp),1000};d.damageable=d.conditions_allowed=true;
    d.lightning_armor=80;d.physical_armor=60;d.healing_allowed=true;d.healing_evidence="synthetic known healing";
    d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;n.combat_definition=d;return n;
}
struct NpcFixture{
    world::Instance w;effects::Scope source,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    NpcFixture(unsigned hp,bool ally=false){
        auto actors=std::vector{actor(100,hp)};if(ally)actors.push_back(actor(101,1000,false));w.configure_npcs(actors);
        w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        source=*w.effect_scope(1);target=*w.effect_scope(100);check(w.configure_projectiles({1000,2}));
    }
    void launch(unsigned rank=12){check(w.lightning_javelin(source,target,*effects::lightning_javelin(rank),20,5,1000));}
    double hp(){return w.npc_combat_snapshot(target)->health->current;}
    bool has(unsigned skill,unsigned at=1100){const auto entries=*w.status_snapshot(target,at);return std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.skill==skill;});}
};
int main(int argc,char** argv)try{
    check(argc<=2);std::ofstream output;if(argc==2){output.open(argv[1]);check(output.good());}
    unsigned cases=0;
    for(unsigned rank:{0u,12u,20u}){
        NpcFixture baseline(1000);baseline.launch(rank);baseline.w.tick(1100);const double raw=1000-baseline.hp();check(raw>0);
        for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,500u,1000u})for(unsigned action=0;action<3;++action)for(unsigned protection:{0u,16u,17u}){
            NpcFixture f(hp);if(rr>=0)check(f.w.stage_npc_reversal(f.target,rr,100));
            if(protection)check(f.w.npc_interrupt_stance(f.target,protection,12,10000,100));
            std::optional<combat::npcs::control::Token> token;
            if(action)token=f.w.begin_npc_action(f.target,action==1?combat::npcs::control::Action::attack:combat::npcs::control::Action::spell,1000,5000);
            check(!action||token.has_value());f.box->drain();f.launch(rank);f.w.tick(1100);
            const double converted=rr<0?0:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));
            const double expected=std::max(0.,std::min(1000.,hp+converted)-(raw-converted));
            check(std::abs(f.hp()-expected)<.001&&!f.has(307));
            const auto notifications=f.box->drain();
            check(std::count(notifications.begin(),notifications.end(),combat::detail::integer(49,f.target.actor,0))==unsigned(expected>0&&action==1&&!protection));
            if(output&&rank==12&&action==1&&!protection&&((hp==500&&(rr==-1||rr==0))||(hp==10&&rr==12))){
                output<<"{\"reversal_rank\":"<<rr<<",\"initial_health\":"<<hp<<",\"final_health\":"<<std::setprecision(17)<<f.hp()<<",\"packets\":[";
                bool comma=false;for(const auto& packet:notifications){
                    if(comma)output<<',';comma=true;output<<'"';
                    for(auto byte:packet)output<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(byte);
                    output<<std::dec<<'"';
                }
                output<<"]}\n";
            }
            if(expected>0){
                check(f.w.npc_energy_snapshot(f.target)->current==40-(action==1&&protection==17?effects::interpolate(10,4,12):0));
                if(protection==16)check(f.has(16)==(action!=1));
            }
            if(token)check(f.w.complete_npc_action(*token,5000)==(expected>0&&(action==2||protection!=0)));
            check(f.w.take_npc_deaths(f.source).size()==unsigned(expected<=0));++cases;
        }
    }
    // An actual ordinary controller must not deliver its cancelled windup.
    for(unsigned protection:{0u,16u,17u}){
        NpcFixture f(1,true);check(f.w.stage_npc_reversal(f.target,12,100));
        if(protection)check(f.w.npc_interrupt_stance(f.target,protection,12,10000,100));
        const auto ally=*f.w.effect_scope(101);
        check(f.w.configure_npc_opposed_ordinary(f.target,{20,20,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},"synthetic owned windup"},1000));
        check(f.w.begin_npc_opposed_ordinary(f.target,ally,1000).has_value());f.launch();f.w.tick(1100);check(f.hp()>1);
        f.w.tick(1750);check((f.w.npc_combat_snapshot(ally)->health->current<1000)==(protection!=0));++cases;
    }
    // Conversion healing can kill independently of remaining lightning loss.
    for(unsigned rr:{0u,12u,20u}){
        NpcFixture f(10);check(f.w.stage_npc_reversal(f.target,rr,100));
        check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));
        check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));
        const auto token=f.w.begin_npc_action(f.target,combat::npcs::control::Action::attack,1000,5000);check(token.has_value());
        f.launch();f.w.tick(1100);check(f.hp()==0&&!f.has(307));check(!f.w.complete_npc_action(*token,5000));
        check(f.w.take_npc_deaths(f.source).size()==1);++cases;
    }
    // Player ordinary attacks use the same protection policy, not bare epochs.
    for(bool attacking:{false,true})for(unsigned protection:{0u,16u,17u})for(int rr:{-1,0,12,20}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.configure_projectiles({1000,2}));
        check(w.lightning_armor(target,80)&&w.physical_armor(source,60));
        if(rr>=0)check(w.status_mutation(target,307,81,rr,100,8100,true));
        if(protection)check(w.status_mutation(target,protection,82,12,100,10100,true));
        if(attacking){check(w.ordinary_weapon(target,combat::weapon::Owned{12,1,{20,20,0},12,false,false,27},20));check(w.ordinary_attack(target,1,1000));w.tick(1000);}
        const auto epoch=*w.attack_interrupt_epoch(target);
        check(w.lightning_javelin(source,target,*effects::lightning_javelin(12),20,5,1000));w.tick(1100);
        check(*w.attack_interrupt_epoch(target)==epoch+unsigned(attacking&&!protection));
        check(w.resources(2)->energy==100-(attacking&&protection==17?effects::interpolate(10,4,12):0));++cases;
    }
    std::cout<<cases<<" Javelin NPC conversion/survival, attack-only interruption, Mantra protection and real controller/player attack cases passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
