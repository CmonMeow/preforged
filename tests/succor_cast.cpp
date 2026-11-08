#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <algorithm>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Succor cast line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned mode=0;mode<4;++mode){
        const bool opt_in=mode!=0,completion=mode>=2,npc=mode==3;
        world::Instance w;auto out=std::make_shared<world::Outbox>();
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=500;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,500};
            d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});}
        w.join(1,"Monk",0,0,out,0,{}, {},100,1);
        if(!npc){w.join(2,"Ally",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            w.set_vitals(2,{500,500,40,40},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));}
        w.set_vitals(1,{500,500,40,40},100);
        const unsigned target_actor=npc?100:2;
        const auto source=*w.effect_scope(1),target=*w.effect_scope(target_actor);
        skills::Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[308]=1;
        skills::Slots bar{};bar[0]={308,0};attributes::Ranks ranks{};
        skills::casting::Runtime cast(1);skills::casting::Hooks h;
        unsigned paid=0,committed=0;std::vector<world::Packet> events;
        h.resources=[&](auto actor){return w.resources(actor);};
        h.target_scope=[&](auto actor){return w.effect_scope(actor);};
        h.target_valid=[](auto,float,bool){return false;};
        h.events=[&](const auto& batch){events.insert(events.end(),batch.begin(),batch.end());out->enqueue(batch);};
        h.consume_energy_at=[&](unsigned amount,auto at){++paid;return w.consume_energy(1,amount,source,at);};
        h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
        h.succor_target_valid=[&](auto recipient,auto divine,auto at){return w.succor_target_valid(source,recipient,divine,at);};
        if(opt_in)h.succor=[&](auto recipient,const auto&,auto divine,auto at){
            ++committed;std::vector<world::Packet> visual;
            const bool ok=w.complete_succor(source,recipient,divine,at,completion?nullptr:&visual,completion);
            return skills::casting::EffectCompletion{ok,std::move(visual),{},{},false,completion};
        };
        check(skills::casting::Runtime::effect_supported(308));
        const auto reason=cast.start(100,{308,0,target_actor},bar,policy,ranks,true,h);
        check(reason==(opt_in?skills::casting::Reason::accepted:skills::casting::Reason::unsupported));
        check(paid==unsigned(opt_in)&&w.resources(1)->energy==(opt_in?35u:40u));
        if(!opt_in){check(committed==0);continue;}
        cast.update(1099,h);check(w.status_snapshot(target,1099)->empty());out->drain();
        cast.update(1100,h);check(committed==1);
        const auto status=w.status_snapshot(target,1100);
        check(status&&status->size()==1&&status->front().skill==308);
        check(w.maintained_enchantment_count(source,1100)==1);
        const auto visual=skills::particles::targeted_body_2022(target_actor,1,308);
        const auto packets=out->drain();check(visual&&std::count(packets.begin(),packets.end(),*visual)==1);
        if(completion){
            const auto finish=combat::detail::integer(58,1,0),body=combat::detail::integer(21,1,547);
            const auto wire=*skills::effects::mending_wire(source,target,status->front());
            const auto end=packets.end(),a=std::find(packets.begin(),end,finish),b=std::find(packets.begin(),end,body),
                c=std::find(packets.begin(),end,*visual),d=std::find(packets.begin(),end,wire.effect);
            check(a!=end&&b!=end&&c!=end&&d!=end&&a<b&&b<c&&c<d);
            check(std::count(packets.begin(),end,finish)==1&&std::count(packets.begin(),end,body)==1);
        }
        cast.update(1900,h);
        check(cast.start(2000,{308,0,target_actor},bar,policy,ranks,true,h)==skills::casting::Reason::recharge);
        check(paid==1&&committed==1);
    }
    std::cout<<"PASS Succor paid player/NPC cast, native finish/caster/target/status order, completion and missing-hook gate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
