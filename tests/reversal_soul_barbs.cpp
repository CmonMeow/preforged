#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Soul Barbs Reversal line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned offset){unsigned value=0;for(unsigned i=0;i<4;++i)value|=unsigned(p[offset+i])<<(8*i);return value;}
bool negative(const world::Packet& p){return p.size()==18&&p[0]==162&&word(p,2)==55&&std::bit_cast<float>(word(p,14))<0;}
struct Fixture {
    world::Instance w;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    effects::Scope source,target,trigger;
    bool npc;
    Fixture(bool is_npc,unsigned hp=500,bool recovery=false):npc(is_npc){
        if(recovery)w.configure_recovery({{1000,2000,3000,4000,5000,6000,7000}});
        world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;
        d.relation=combat::npcs::Relation::hostile_to_players;d.healing_allowed=true;d.healing_evidence="synthetic";
        n.combat_definition=d;w.configure_npcs({n});
        for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==2?100:0,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{id==2?hp:1000u,1000,100,100},100);}
        source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);trigger=*w.effect_scope(3);
    }
    double hp()const{return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
    void reversal(unsigned rank,unsigned now=100,unsigned id=900){check(npc?w.stage_npc_reversal(target,rank,now):w.status_mutation(target,307,id,rank,now,now+8000,true));}
    void barbs(unsigned rank=12){check(w.soul_barbs(source,target,*effects::soul_barbs(rank),20,100));}
    void application(unsigned now=200){check(w.backfire(trigger,target,*effects::backfire(28,0),20,now));}
    void source_state(unsigned state){
        if(state==1)check(w.kill(source,100));
        if(state>=2)w.leave(1);
        if(state==3){w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);}
    }
    void wound(){
        if(npc){check(w.apply_condition(trigger,trigger,combat::conditions::Kind::deep_wound,100,10000));check(w.condition_transfer(trigger,target,*effects::condition_transfer(132,12),100));}
        else check(w.apply_condition(trigger,target,combat::conditions::Kind::deep_wound,100,10000));
    }
};
double expected(unsigned hp,unsigned amount,int rank){const double healing=rank<0?0:std::min(double(amount),double(effects::reversal_fortune(rank)->maximum_healing));return std::max(0.,std::min(1000.,hp+healing)-(amount-healing));}
int main()try {
    unsigned cases=0;
    for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u})
    for(unsigned hp:{10u,500u,1000u})for(unsigned state:{0u,1u,2u,3u}){
        ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);f.barbs(rank);check(f.hp()==hp);
        f.source_state(state);f.box->drain();f.application();
        const auto wanted=expected(hp,effects::soul_barbs(rank)->damage,rr);check(std::abs(f.hp()-wanted)<1.01);
        const auto packets=f.box->drain();
        const auto healing=rr<0?0:std::min(effects::soul_barbs(rank)->damage,effects::reversal_fortune(rr)->maximum_healing);
        check(std::count_if(packets.begin(),packets.end(),negative)==unsigned(state<2&&effects::soul_barbs(rank)->damage>healing));
        for(const auto& p:packets)if(negative(p))check(word(p,6)==f.target.actor&&word(p,10)==1);
        if(npc&&state<2)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));
        f.w.tick(201);check(std::abs(f.hp()-wanted)<1.01);if(npc&&state<2)check(f.w.take_npc_deaths(f.source).empty());
        if(state==3)check(f.w.resources(1)->health==1000);
    }
    check(cases==288);
    // Renewal replaces the old Reversal before the application reaction. Both
    // recipients must consume the NEW rank, immediately, not on a later spell.
    for(bool npc:{false,true}){++cases;Fixture f(npc);f.reversal(0);f.barbs();f.reversal(12,200,901);check(f.hp()==527);
        f.application(300);check(f.hp()==500);f.w.tick(301);check(f.hp()==500);}
    // Soul Barbs renewal itself reacts at the OLD observer's rank and cause;
    // only subsequent applications belong to the new caster/rank.
    for(bool npc:{false,true})for(unsigned state:{0u,1u,2u,3u}){
        ++cases;Fixture f(npc);f.reversal(12);f.barbs(20);f.source_state(state);f.box->drain();
        check(f.w.soul_barbs(f.trigger,f.target,*effects::soul_barbs(0),20,200)&&f.hp()==535);
        f.box->drain();f.application(300);check(f.hp()==520);const auto packets=f.box->drain();
        check(std::count_if(packets.begin(),packets.end(),negative)==1);
        for(const auto& p:packets)if(negative(p))check(word(p,10)==3);
    }
    // Vulnerability amplifies the incoming reaction before conversion.
    for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
        ++cases;Fixture f(false,hp);if(rr>=0)f.reversal(rr);f.barbs(rank);
        check(f.w.status_mutation(f.target,346,950,0,100,8100,true));f.application();
        check(std::abs(f.hp()-expected(hp,2*effects::soul_barbs(rank)->damage,rr))<1.01);
    }
    // A lethal application reaction must not publish the new Soul Barbs entry
    // after death or enqueue a second death when its journal is drained again.
    for(bool npc:{false,true})for(unsigned state:{0u,1u,2u,3u})for(bool renew:{false,true}){
        ++cases;Fixture f(npc,10);f.reversal(12);f.barbs();f.wound();f.source_state(state);f.box->drain();
        if(renew)check(f.w.soul_barbs(f.trigger,f.target,*effects::soul_barbs(0),20,200));else f.application();
        check(f.hp()==0);const auto packets=f.box->drain();
        const auto death=std::find_if(packets.begin(),packets.end(),[&](const auto& p){return p.size()==10&&p[0]==240&&word(p,2)==f.target.actor&&(word(p,6)&0x10);});check(death!=packets.end());
        if(renew)check(std::none_of(death,packets.end(),[&](const auto& p){return p.size()>=8&&p[0]==65&&word(p,2)==f.target.actor&&p[6]==100&&p[7]==0;}));
        if(npc&&state<2)check(f.w.take_npc_deaths(f.source).size()==1);
        f.w.tick(201);check(f.hp()==0);if(npc&&state<2)check(f.w.take_npc_deaths(f.source).empty());
    }
    // Suppressing post-death publication must still record the caster's
    // enemy-targeting event and reset its automatic recovery quiet window.
    for(bool npc:{false,true}){++cases;Fixture f(npc,10,true);f.w.set_vitals(3,{500,1000,100,100},100);f.barbs();
        f.w.tick(1200);check(f.w.soul_barbs(f.trigger,f.target,*effects::soul_barbs(0),20,1200)&&f.hp()==0);
        const auto hp=f.w.resources(3)->health;check(hp==501);f.w.tick(2000);check(f.w.resources(3)->health==hp);
    }
    check(cases==352);
    std::cout<<"PASS "<<cases<<" Soul Barbs Reversal cases; application journals, old-observer/new-enchantment renewal, vulnerability, persistent attribution and healing death;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
