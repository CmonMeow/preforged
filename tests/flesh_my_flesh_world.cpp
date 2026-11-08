#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <algorithm>
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Flesh of My Flesh line "+std::to_string(at.line()));
}
struct Fixture{
    world::Instance w;
    std::shared_ptr<world::Outbox> caster_box=std::make_shared<world::Outbox>();
    std::shared_ptr<world::Outbox> target_box=std::make_shared<world::Outbox>();
    skills::effects::Scope source,target;
    Fixture(unsigned caster_health=480,unsigned target_max=480){
        w.join(1,"Ritualist",0,0,caster_box);w.join(2,"Ally",100,0,target_box);
        w.set_vitals(1,{caster_health,480,40,40},100);
        w.set_vitals(2,{target_max,target_max,40,40},100);
        check(w.party_action(1,158,2,0)&&w.party_action(2,154,1,0));
        source=*w.effect_scope(1);target=*w.effect_scope(2);
        check(w.kill(target,1000));caster_box->drain();target_box->drain();
    }
};
int main()try{
    using namespace skills;using namespace effects;
    const auto plan=*resurrection(791,12);
    check(plan.caster_half_health&&plan.caster_health&&plan.energy_percent==17);
    check(resurrection(791,0)->energy_percent==5&&resurrection(791,20)->energy_percent==20);
    {
        Fixture f;auto forged=plan;++forged.energy_percent;
        std::function<void()> publication;
        check(!f.w.flesh_my_flesh(f.source,f.target,forged,1000,&publication));
        check(!f.w.resurrect_target(f.source,f.target,plan,1000));
        check(f.w.resources(1)->health==480&&f.w.resources(2)->health==0);
        check(!f.w.flesh_my_flesh(f.source,{f.target.instance,f.target.actor,f.target.generation+1},plan,1000));
        check(f.w.flesh_my_flesh(f.source,f.target,plan,1000,&publication));
        check(f.w.resources(1)->health==240&&f.w.resources(2)->health==240&&f.w.resources(2)->energy==7);
        check(f.caster_box->drain().empty()&&f.target_box->drain().empty());
        check(bool(publication));publication();check(!f.caster_box->drain().empty());
        check(!f.target_box->drain().empty());publication();check(f.caster_box->drain().empty());
        check(!f.w.flesh_my_flesh(f.source,f.target,plan,1000));
    }
    {
        Fixture f(1);check(f.w.flesh_my_flesh(f.source,f.target,*resurrection(791,0),1000));
        check(f.w.resources(1)->health==1&&f.w.resources(2)->health==1);
    }
    {
        Fixture f(400,100);check(f.w.flesh_my_flesh(f.source,f.target,plan,1000));
        check(f.w.resources(1)->health==200&&f.w.resources(2)->health==100);
    }
    {
        Fixture f;casting::Runtime runtime(1);casting::Hooks h;unsigned commits=0,pays=0;
        h.resources=[&](unsigned id){return f.w.resources(id);};
        h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
        h.consume_energy_at=[&](unsigned cost,auto now){++pays;return f.w.consume_energy(1,cost,f.source,now);};
        h.target_scope=[&](unsigned id){return f.w.effect_scope(id);};
        h.resurrection_target_valid=[&](unsigned id,float range){return f.w.resurrection_target_valid(f.source,id,range);};
        h.flesh_my_flesh=[&](auto target,const auto& value,auto now)->casting::EffectCompletion{
            ++commits;std::function<void()> publication;
            const bool accepted=f.w.flesh_my_flesh(f.source,target,value,now,&publication);
            std::vector<world::Packet> visuals;
            if(accepted)visuals.push_back(*particles::targeted_body_2022(target.actor,f.source.actor,791));
            return {accepted,std::move(visuals),std::nullopt,std::move(publication),true};
        };
        Slots bar{};bar[0]={791,0};Policy policy;policy.primary=8;policy.campaigns=255;policy.learned[791]=1;
        attributes::Ranks ranks{};ranks[33]=12;
        auto pvp=policy;pvp.pvp_match=true;casting::Runtime unreviewed_pvp(1);
        check(unreviewed_pvp.start(1000,{791,0,2},bar,pvp,ranks,true,h)==casting::Reason::unsupported);
        check(pays==0&&f.w.resources(1)->energy==40);
        const auto admission=runtime.start(1000,{791,0,2},bar,policy,ranks,true,h);
        if(admission!=casting::Reason::accepted)throw std::runtime_error("Flesh admission "+std::to_string(static_cast<unsigned>(admission)));
        check(pays==1&&commits==0&&f.w.resources(1)->health==480);
        runtime.update(4999,h);check(commits==0&&f.w.resources(2)->health==0);
        runtime.update(5000,h);check(commits==1&&f.w.resources(1)->health==240&&f.w.resources(2)->health==240);
        const auto packets=f.caster_box->drain();
        const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
        const auto body=std::find(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,791));
        const auto delta=[&](unsigned actor){return std::find_if(packets.begin(),packets.end(),[&](const auto& packet){
            return packet.size()==14&&packet[0]==162&&packet[1]==0&&packet[2]==55&&
                packet[3]==0&&packet[4]==0&&packet[5]==0&&packet[6]==actor;
        });};
        const auto source_delta=delta(1),target_delta=delta(2);
        const auto revive=std::find_if(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()==10&&packet[0]==241&&packet[1]==0&&packet[2]==2;
        });
        check(finish!=packets.end()&&body!=packets.end()&&source_delta!=packets.end()&&
            revive!=packets.end()&&target_delta!=packets.end()&&
            finish<body&&body<source_delta&&source_delta<revive&&revive<target_delta);
    }
    std::cout<<"Flesh of My Flesh World and paid cast passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
