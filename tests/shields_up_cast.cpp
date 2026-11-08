#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Shields Up cast line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<6;++mode){
        world::Instance w;
        auto box=std::make_shared<world::Outbox>();
        for(unsigned id:{1u,2u,3u}){
            w.join(id,"Actor",float(id*30),0,id==1?box:std::make_shared<world::Outbox>(),
                0,{}, {},100,id==3?2:1);
            w.set_vitals(id,{500,500,mode==2?5u:100u,100},100);
        }
        check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
        const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);
        Slots bar{};bar[0]={367,0};check(w.configure_adrenaline(owner,bar,100));
        casting::Runtime cast(1);casting::Hooks hooks;
        unsigned payments=0,completions=0;bool stale=false;
        hooks.resources=[&](auto id){return w.resources(id);};
        hooks.events=[&](const auto& packets){box->enqueue(packets);};
        hooks.consume_energy_at=[&](auto cost,auto at){++payments;check(cost==10);
            const bool paid=w.consume_energy(1,cost,owner,at);if(mode==3&&paid)stale=true;return paid;};
        hooks.target_scope=[&](auto id){auto scope=w.effect_scope(id);
            if(stale&&scope)++scope->generation;return scope;};
        hooks.target_valid=[](auto id,float,bool self){return id==1&&self;};
        if(mode!=1)hooks.shields_up=[&](auto source,const auto& plan,auto at){
            ++completions;std::function<void()> publish;
            const bool applied=mode!=4&&w.stage_shields_up_party(source,plan,at,&publish);
            return casting::EffectCompletion{applied,{}, {},std::move(publish)};};
        Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[367]=1;
        attributes::Ranks ranks{};ranks[21]=mode==5?21:rank;
        box->drain();const auto result=cast.start(200,{367,0,3},bar,policy,ranks,true,hooks);
        if(mode==1||mode==5){check(result==casting::Reason::unsupported&&payments==0&&completions==0);continue;}
        if(mode==2){check(result==casting::Reason::energy&&payments==1&&completions==0);continue;}
        if(result!=casting::Reason::accepted||cast.active()||payments!=1)
            std::cerr<<"mode="<<mode<<" rank="<<rank<<" reason="<<int(result)
                <<" active="<<cast.active()<<" payments="<<payments<<'\n';
        check(result==casting::Reason::accepted&&!cast.active()&&payments==1);
        if(mode==3||mode==4){check(completions==mode-3&&w.shields_up_player_armor(owner,200,ShieldsUpDamage::projectile_attack)==0);continue;}
        check(completions==1&&w.resources(1)->energy==90);
        for(const auto scope:{owner,ally}){
            check(w.shields_up_player_armor(scope,200,ShieldsUpDamage::projectile_attack)==60);
            check(w.shields_up_player_armor(scope,200,ShieldsUpDamage::projectile_spell)==0);
        }
        check(w.status_snapshot(foe,200)->empty());
        const auto packets=box->drain();
        const auto instant=casting::generic(48,1,367),body=*casting::skill_success_body_effect(1,367);
        const auto activation=std::find(packets.begin(),packets.end(),instant);
        const auto visual=std::find(packets.begin(),packets.end(),body);
        const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()>1&&packet[0]==65&&packet[1]==0;});
        check(activation!=packets.end()&&visual!=packets.end()&&status!=packets.end()&&
            activation<visual&&visual<status);
        check(std::count(packets.begin(),packets.end(),
            casting::slot_event(228,1,{367,0},30))==1);
        check(cast.start(201,{367,0,0},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
    }
    std::cout<<"Shields Up opt-in cast: rank, payment, scope, party snapshot, body ordering and recharge pass; production hook remains gated\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
