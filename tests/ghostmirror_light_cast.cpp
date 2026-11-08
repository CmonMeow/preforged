#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
using namespace fw::gw::skills;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Ghostmirror Light cast line "+std::to_string(at.line()));
}
static world::Instance::Npc make_npc(unsigned id,bool spirit,bool typed=true){
    world::Instance::Npc n;n.id=n.definition.id=id;n.x=60;
    n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition body;
    body.binding.map=146;body.binding.actor=body.binding.definition=id;
    body.binding.level=20;body.spawn_health={20,100};body.damageable=true;
    body.relation=combat::npcs::Relation::allied_to_players;
    if(typed){body.creature_type=spirit?"spirit":"human";
              body.creature_type_evidence="fixture";}
    if(!spirit){body.healing_allowed=true;body.healing_evidence="fixture";}
    n.combat_definition=body;return n;
}
int main()try{
    for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<8;++mode){
        world::Instance w;
        if(mode==1)w.configure_npcs({make_npc(101,true)});
        if(mode==2)w.configure_npcs({make_npc(100,false),make_npc(101,true)});
        if(mode==5)w.configure_npcs({make_npc(101,true,false)});
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Ritualist",0,0,box,0,{}, {},100,1);
        w.join(2,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{20,100,50,50},100);
        const auto source=*w.effect_scope(1);
        const unsigned selected=mode==2?100:2;
        Slots bar{};bar[0]={1741,0};
        Policy policy;policy.primary=8;policy.campaigns=255;policy.learned[1741]=1;
        attributes::Ranks ranks{};ranks[33]=0;
        casting::Hooks h;
        h.events=[&](const auto& packets){box->enqueue(packets);};
        h.resources=[&](unsigned actor){return w.resources(actor);};
        h.consume_energy_at=[&](unsigned amount,auto now){
            check(amount==5);return w.consume_energy(1,amount,source,now);
        };
        h.target_scope=[&](unsigned actor){return w.effect_scope(actor);};
        h.ghostmirror_light_target_valid=[&](auto target,auto now){
            return w.valid_gift_health_target(source,target,now);
        };
        if(version){const auto renewal=*effects::ether_renewal(version,15);
            check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));}
        unsigned stages=0;
        h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(source,skill,event,now);};
        h.ghostmirror_light_completed=[&](auto target,const auto& plan,auto event,auto now){
            ++stages;
            auto visual=particles::targeted_body_2022(target.actor,source.actor,1741);
            check(bool(visual));
            std::function<void()> publish;
            const bool accepted=w.resources(target.actor)?
                w.stage_ghostmirror_light_player(source,target,plan,now,&publish,event):
                w.stage_ghostmirror_light_npc(source,target,plan,now,&publish,event);
            if(!accepted)return casting::EffectCompletion{};
            check(!(w.resources(target.actor)?
                w.stage_ghostmirror_light_player(source,target,plan,now,nullptr,event):
                w.stage_ghostmirror_light_npc(source,target,plan,now,nullptr,event)));
            return casting::EffectCompletion{true,{*visual},{},std::move(publish)};
        };
        casting::Runtime cast(1);
        check(casting::Runtime::effect_supported(1741));
        auto missing=h;missing.ghostmirror_light_completed={};
        casting::Runtime gated(1);
        check(gated.start(1000,{1741,0,selected},bar,policy,ranks,true,missing)==
            casting::Reason::unsupported);
        check(cast.start(1000,{1741,0,1},bar,policy,ranks,true,h)==casting::Reason::target);
        check(cast.start(1000,{1741,0,3},bar,policy,ranks,true,h)==casting::Reason::target);
        check(cast.start(1000,{1741,0,selected},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(1)->energy==45);
        if(mode==3)cast.cancel(h);
        if(mode==6)h.ghostmirror_light_completed={};
        if(mode==7)h.before_spell_effect={};
        h.ghostmirror_light=[](auto,auto,auto)->casting::EffectCompletion{throw std::runtime_error("Ghostmirror fallback");};
        if(mode==4){w.leave(2);w.join(2,"Replacement",40,0,
            std::make_shared<world::Outbox>(),0,{}, {},1200,1);
            w.set_vitals(2,{20,100,50,50},100);}
        cast.update(1999,h);check(stages==0);
        box->drain();cast.update(2000,h);
        check(stages==(mode==3||mode==4||mode>=6?0u:1u));
        const bool applied=mode!=3&&mode!=4&&mode!=5&&mode<6;
        check(mode==2?w.npc_combat_snapshot(*w.effect_scope(100))->health->current==(applied?35:20):
            w.resources(2)->health==(applied?35u:20u));
        check(w.resources(1)->health==((mode==1||mode==2)?35u:20u)+(applied&&version?20:0));
        check(w.resources(1)->energy==45+(applied&&version?effects::ether_renewal(version,15)->energy_per_enchantment:0));
        const auto packets=box->drain();
        const auto caster=casting::skill_success_body_effect(1,1741);
        const auto target=particles::targeted_body_2022(selected,1,1741);
        check(bool(caster)&&bool(target));
        const auto body_at=std::find(packets.begin(),packets.end(),*caster);
        const auto target_at=std::find(packets.begin(),packets.end(),*target);
        check((body_at!=packets.end())==applied);
        check((target_at!=packets.end())==applied);
        if(applied){
            const auto recipient=std::find(packets.begin(),packets.end(),
                particles::health_change_2022(selected,1,15,100));
            check(recipient!=packets.end()&&body_at<target_at&&target_at<recipient);
        }
    }
    std::cout<<"PASS Ghostmirror Light normal cast, payment, two visuals, staged double heal and cancellation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
