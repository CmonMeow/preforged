#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool good,std::source_location at=std::source_location::current()){
    if(!good)throw std::runtime_error("Restful Breeze cast line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned mode=0;mode<5;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        if(mode!=0){world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=100;
            npc.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition def;def.binding.map=146;def.binding.actor=def.binding.definition=100;
            def.relation=mode==2?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
            def.spawn_health=combat::Health{100,500};def.damageable=def.conditions_allowed=true;
            npc.combat_definition=def;w.configure_npcs({npc});}
        w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
        w.set_vitals(1,{500,500,100,100},100);
        if(mode==0){w.join(2,"Ally",100,0,box,0,{}, {},100,1);w.set_vitals(2,{100,500,100,100},100);}
        const auto source=*w.effect_scope(1),target=*w.effect_scope(mode==0?2:100);
        casting::Hooks h;h.resources=[&](auto id){return w.resources(id);};
        h.events=[&](const auto& packets){box->enqueue(packets);};
        h.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(mode==3&&id==100&&scope)++scope->generation;return scope;};
        h.target_valid=[](auto,float,bool){return false;};
        h.ally_enchantment_target_valid=[&](auto recipient,unsigned divine,auto at){return w.valid_ally_enchantment_target(source,recipient,divine,at);};
        h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(1,amount,source,at);};
        h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
        h.restful_breeze=[&](auto recipient,const regeneration::Plan& plan,unsigned divine,auto at)->casting::EffectCompletion{
            std::vector<world::Packet> visuals;std::function<void()> publish;
            const bool ok=w.stage_restful_breeze(source,recipient,plan,divine,at,&visuals,&publish);
            return {ok,std::move(visuals),{},std::move(publish),true};};
        if(mode==4)h.restful_breeze={};
        Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[886]=1;
        Slots bar{};bar[0]={886,0};attributes::Ranks ranks{};ranks[13]=12;
        casting::Runtime cast(1);box->drain();
        const auto started=cast.start(1000,{886,0,target.actor},bar,policy,ranks,true,h);
        check(started==(mode==2||mode==3?casting::Reason::target:mode==4?casting::Reason::unsupported:casting::Reason::accepted));
        if(mode>=2){check(w.resources(1)->energy==100);continue;}
        check(w.resources(1)->energy==95);box->drain();cast.update(2000,h);
        const auto packets=box->drain();check(!cast.active());
        const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
        const auto body=std::find(packets.begin(),packets.end(),*particles::target_body_2022(target.actor,886));
        const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&p[0]==65;});
        check(finish!=packets.end()&&body!=packets.end()&&status!=packets.end()&&finish<body&&body<status);
        w.tick(3000);
        check(mode==0?w.resources(2)->health==120:std::abs(w.npc_combat_snapshot(target)->health->current-120)<1e-7);
    }
    std::cout<<"PASS Restful Breeze paid player/NPC cast, scoped target rejection, finish/body/status order and +10 recipient regeneration\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
