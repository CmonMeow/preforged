#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
using namespace fw::gw::skills;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Healing Ribbon cast line "+std::to_string(at.line()));
}
static world::Instance::Npc make_npc(){
    world::Instance::Npc n;n.id=n.definition.id=100;n.x=45;
    n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;
    d.binding.actor=d.binding.definition=100;d.binding.level=20;
    d.spawn_health={20,100};d.damageable=true;
    d.relation=combat::npcs::Relation::allied_to_players;
    d.healing_allowed=true;d.healing_evidence="fixture";
    n.combat_definition=d;return n;
}
int main()try{
    for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<7;++mode){
        world::Instance w;if(mode!=6)w.configure_npcs({make_npc()});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,box,0,{}, {},100,1);
        w.join(2,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Nearby",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(4,"Foe",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u,4u})w.set_vitals(id,{20,100,50,50},100);
        const auto source=*w.effect_scope(1);
        const unsigned selected=mode==1?100:2;
        Slots bar{};bar[0]={2062,0};
        Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[2062]=1;
        attributes::Ranks ranks{};ranks[13]=0;
        casting::Hooks h;
        h.events=[&](const auto& packets){box->enqueue(packets);};
        h.resources=[&](unsigned actor){return w.resources(actor);};
        h.consume_energy_at=[&](unsigned amount,auto now){
            check(amount==10);return w.consume_energy(1,amount,source,now);
        };
        h.target_scope=[&](unsigned actor){return w.effect_scope(actor);};
        h.healing_ribbon_target_valid=[&](auto target,auto now){
            return w.valid_gift_health_target(source,target,now);
        };
        if(mode==6)w.leave(3);
        if(version){const auto renewal=*effects::ether_renewal(version,15);
            check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));}
        unsigned stages=0;
        h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(source,skill,event,now);};
        h.healing_ribbon_completed=[&](auto target,const auto& plan,unsigned divine,auto event,auto now){
            ++stages;
            const auto visual=particles::targeted_body_2022(target.actor,source.actor,2062);
            check(bool(visual));
            std::function<void()> publish;
            const auto recipients=w.stage_healing_ribbon(source,target,plan,divine,now,&publish,event);
            if(!recipients)return casting::EffectCompletion{};
            check(recipients->size()==(mode==6?2:3));
            check(!w.stage_healing_ribbon(source,target,plan,divine,now,nullptr,event));
            return casting::EffectCompletion{true,{*visual},{},std::move(publish)};
        };
        casting::Runtime cast(1);
        check(casting::Runtime::effect_supported(2062));
        auto missing=h;missing.healing_ribbon_completed={};
        casting::Runtime gated(1);
        check(gated.start(1000,{2062,0,selected},bar,policy,ranks,true,missing)==
            casting::Reason::unsupported);
        check(cast.start(1000,{2062,0,1},bar,policy,ranks,true,h)==casting::Reason::target);
        check(cast.start(1000,{2062,0,4},bar,policy,ranks,true,h)==casting::Reason::target);
        check(cast.start(1000,{2062,0,selected},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(1)->energy==40);
        if(mode==2)cast.cancel(h);
        if(mode==4)h.healing_ribbon_completed={};
        if(mode==5)h.before_spell_effect={};
        h.healing_ribbon=[](auto,auto,auto,auto)->casting::EffectCompletion{throw std::runtime_error("Ribbon fallback");};
        if(mode==3){w.leave(2);w.join(2,"Replacement",40,0,
            std::make_shared<world::Outbox>(),0,{}, {},1200,1);
            w.set_vitals(2,{20,100,50,50},100);}
        cast.update(1999,h);check(stages==0);
        box->drain();cast.update(2000,h);
        const bool applied=mode<2||mode==6;
        check(stages==(applied?1u:0u));
        check(w.resources(1)->health==20+(applied&&version?20:0)+(mode==6?10:0));
        check(w.resources(1)->energy==40+(applied&&version?effects::ether_renewal(version,15)->energy_per_enchantment:0));
        check(mode==1?w.npc_combat_snapshot(*w.effect_scope(100))->health->current==(applied?40:20):
            w.resources(2)->health==(applied?40u:20u));
        const auto packets=box->drain();
        const auto visual=particles::targeted_body_2022(selected,1,2062);
        check(bool(visual));
        const auto target_at=std::find(packets.begin(),packets.end(),*visual);
        check((target_at!=packets.end())==applied);
        if(applied){
            const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
            const auto recipient=std::find(packets.begin(),packets.end(),
                particles::health_change_2022(selected,1,20,100));
            check(finish!=packets.end()&&recipient!=packets.end()&&
                  finish<target_at&&target_at<recipient);
            check(std::count(packets.begin(),packets.end(),*visual)==1);
        }
    }
    std::cout<<"PASS Healing Ribbon paid cast, primary visual, mixed healing and cancellation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
