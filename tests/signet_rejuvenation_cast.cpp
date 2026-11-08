#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
using namespace fw::gw::skills;
static void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Signet of Rejuvenation cast line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned mode=0;mode<7;++mode){
        world::Instance w;
        if(mode==3){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
            n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition body;
            body.binding.map=146;body.binding.actor=body.binding.definition=100;
            body.binding.level=20;body.spawn_health={20,100};body.damageable=true;
            body.relation=combat::npcs::Relation::allied_to_players;
            body.healing_allowed=true;body.healing_evidence="fixture";
            n.combat_definition=body;w.configure_npcs({n});
        }
        const auto box=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,box,0,{}, {},100,1);
        w.join(2,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{20,100,50,50},100);
        const auto source=*w.effect_scope(1);
        const unsigned selected=mode==3?100:mode==6?1:2;
        Slots bar{};bar[0]={887,0};
        Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[887]=1;
        attributes::Ranks ranks{};ranks[13]=0;
        casting::Hooks h;
        h.events=[&](const auto& packets){box->enqueue(packets);};
        h.resources=[&](unsigned actor){return w.resources(actor);};
        h.consume_energy_at=[&](unsigned amount,auto now){
            check(amount==0);return w.consume_energy(1,amount,source,now);
        };
        h.target_scope=[&](unsigned actor){return w.effect_scope(actor);};
        h.rejuvenation_target_valid=[&](auto target,auto now){
            return w.valid_signet_rejuvenation_target(source,target,now);
        };
        unsigned stages=0;
        h.rejuvenation=[&](auto target,const auto& plan,auto now){
            ++stages;
            const auto visual=particles::targeted_body_2022(target.actor,source.actor,887);
            check(bool(visual));
            std::function<void()> publish;
            const bool accepted=w.resources(target.actor)?
                w.stage_signet_rejuvenation_player(source,target,plan,now,&publish):
                w.stage_signet_rejuvenation_npc(source,target,plan,now,&publish);
            if(!accepted)return casting::EffectCompletion{};
            return casting::EffectCompletion{true,{*visual},{},std::move(publish)};
        };
        casting::Runtime cast(1);
        check(casting::Runtime::effect_supported(887));
        auto missing=h;missing.rejuvenation={};
        casting::Runtime gated(1);
        check(gated.start(1000,{887,0,selected},bar,policy,ranks,true,missing)==
            casting::Reason::unsupported);
        check(cast.start(1000,{887,0,3},bar,policy,ranks,true,h)==casting::Reason::target);
        check(cast.start(1000,{887,0,selected},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(1)->energy==50);
        if(mode==1)check(w.attack_activity(*w.effect_scope(2),3000,1200));
        if(mode==2)check(w.cast_activity(*w.effect_scope(2),281,1200,3000));
        if(mode==3)check(bool(w.begin_npc_action(*w.effect_scope(100),
            combat::npcs::control::Action::attack,1200,3000)));
        if(mode==4)cast.cancel(h);
        if(mode==5){w.leave(2);w.join(2,"Replacement",40,0,
            std::make_shared<world::Outbox>(),0,{}, {},1500,1);
            w.set_vitals(2,{20,100,50,50},100);}
        cast.update(1999,h);check(stages==0);
        box->drain();cast.update(2000,h);
        const bool applied=mode!=4&&mode!=5;
        check(stages==(applied?1u:0u));
        const unsigned gained=(mode==1||mode==2||mode==3)?30:15;
        const auto recipient_health=mode==3?
            w.npc_combat_snapshot(*w.effect_scope(100))->health->current:
            w.resources(selected)->health;
        check(recipient_health==(applied?20+gained:20));
        const auto packets=box->drain();
        const auto target=particles::targeted_body_2022(selected,1,887);
        check(bool(target));
        const auto body=std::find(packets.begin(),packets.end(),*target);
        check((body!=packets.end())==applied);
        if(applied){
            const auto receipt=std::find(packets.begin(),packets.end(),
                particles::health_change_2022(selected,1,gained,100));
            check(receipt!=packets.end()&&body<receipt);
        }
    }
    std::cout<<"PASS Signet of Rejuvenation paid cast, player/NPC heal, cancel and targeted 2022 body\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
