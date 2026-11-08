#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Iron Palm World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance world;
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    skills::effects::Scope caster,target;
    explicit Fixture(bool npc,std::optional<bool> control=true,unsigned hp=1000){
        if(npc){
            world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
            n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;
            d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{double(hp),1000};
            d.damageable=d.conditions_allowed=true;
            d.healing_allowed=true;d.healing_evidence="authored Iron Palm conversion fixture";
            d.knockdown_allowed=control;
            d.knockdown_evidence=control.has_value()?"test":"";
            n.combat_definition=d;world.configure_npcs({n});
        }
        world.join(1,"Caster",0,0,box,0,{}, {},100,1);
        world.set_vitals(1,{1000,1000,30,30},100);
        if(!npc){
            world.join(2,"Target",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            world.set_vitals(2,{hp,1000,30,30},100);
        }
        caster=*world.effect_scope(1);target=*world.effect_scope(npc?100:2);
        box->drain();
    }
    double health()const{return target.actor==100?
        world.npc_combat_snapshot(target)->health->current:
        double(world.resources(2)->health);}
    bool cast(std::uint64_t at=1000,std::vector<world::Packet>* packets=nullptr){
        return world.iron_palm(caster,target,*skills::effects::iron_palm(12),20,at,packets);
    }
};
int main()try{
    {
        Fixture f(false,true,500);auto& w=f.world;
        w.join(3,"Monk",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(3,{400,1000,100,100},100);const auto monk=*w.effect_scope(3);
        check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
        const auto seed=*skills::effects::healing_seed(12);
        check(w.complete_healing_seed(monk,f.target,seed,0,100));
        check(w.degeneration_hex(f.caster,f.target,*skills::effects::degeneration_hex(101,0),100));
        check(w.status_mutation(f.target,1399,80,12,100,6100,true));
        f.box->drain();std::function<void()> publish;
        check(w.iron_palm(f.caster,f.target,*skills::effects::iron_palm(12),20,1000,nullptr,&publish));
        check(f.health()==464+seed.healing&&w.resources(3)->health==400+seed.healing);
        check(f.box->drain().empty());publish();const auto packets=f.box->drain();
        const auto damage=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[1]==0&&p[2]==55&&p[3]==0&&p[4]==0&&p[5]==0;});
        const auto heal=std::find(packets.begin(),packets.end(),skills::particles::health_change_2022(2,3,seed.healing,1000));
        const auto down=std::find(packets.begin(),packets.end(),combat::knockdown_2022(2,2.f));
        check(damage!=packets.end()&&heal!=packets.end()&&down!=packets.end()&&damage<heal&&heal<down);
        publish();check(f.box->drain().empty());
    }
    for(bool npc:{false,true})for(unsigned mode:{0u,1u,2u,3u}){
        Fixture f(npc,true,mode==0?500:mode==3?10:30);
        auto& w=f.world;
        check(w.degeneration_hex(f.caster,f.target,*skills::effects::degeneration_hex(101,0),100));
        if(npc)check(w.stage_npc_shield_absorption(f.target,12,100));
        else check(w.status_mutation(f.target,1399,80,12,100,6100,true));
        if(mode==1||mode==3){
            if(npc)check(w.stage_npc_reversal(f.target,mode==3?0:12,100));
            else check(w.status_mutation(f.target,307,81,mode==3?0:12,100,8100,true));
        }
        std::function<void()> publish;f.box->drain();
        check(w.iron_palm(f.caster,f.target,*skills::effects::iron_palm(12),20,1000,nullptr,&publish));
        check(bool(publish)&&f.box->drain().empty());publish();
        const unsigned first=mode==0?464:mode==1?66:mode==2?0:4;
        check(f.health()==first);
        check(w.knocked_down(f.target,1000)==(mode!=2));
        check(w.dagger_stage(f.caster,f.target,1000)==(mode==2?0u:1u));
        if(mode==0||mode==1){
            check(f.cast(1100)&&f.health()==first-31);
            check(f.cast(1200)&&f.health()==first-57);
        }
    }
    for(bool npc:{false,true}){
        Fixture plain(npc);
        std::vector<world::Packet> packets;
        check(plain.cast(1000,&packets)&&plain.health()==959);
        check(!plain.world.knocked_down(plain.target,1000));
        check(plain.world.dagger_stage(plain.caster,plain.target,1000)==1);
        check(plain.world.dagger_stage(plain.caster,plain.target,16000)==0);
        check(!packets.empty());
        const auto chain=skills::effects::iron_palm_chain_2022(plain.caster.actor,plain.target.actor,1);
        check(packets.back()==chain);
        check(std::count(packets.begin(),packets.end(),chain)==1);
        const auto premature=plain.box->drain();
        for(const auto& packet:packets)
            check(std::find(premature.begin(),premature.end(),packet)==premature.end());
        plain.box->enqueue(packets);
        const auto published=plain.box->drain();
        check(published==packets);
        packets.clear();
        check(plain.cast(1100,&packets));
        check(plain.health()==918);
        check(std::find(packets.begin(),packets.end(),chain)==packets.end());
        check(plain.world.dagger_stage(plain.caster,plain.target,16000)==1);
        check(plain.world.dagger_stage(plain.caster,plain.target,16100)==0);
        Fixture conditioned(npc);
        if(npc)check(conditioned.world.degeneration_hex(conditioned.caster,conditioned.target,
            *skills::effects::degeneration_hex(101,0),100));
        else check(conditioned.world.apply_condition(conditioned.caster,conditioned.target,
            combat::conditions::Kind::poison,100,5000));
        packets.clear();
        check(conditioned.cast(1000,&packets));
        check(conditioned.health()<=959);
        check(conditioned.world.knocked_down(conditioned.target,1000));
        const auto down=combat::knockdown_2022(conditioned.target.actor,2.f);
        const auto down_at=std::find(packets.begin(),packets.end(),down);
        // Legacy vectors now defer cosmetics only; authoritative damage has
        // already been published. Paid casts use per-Outbox publication.
        check(down_at!=packets.end());
        const auto authority=conditioned.box->drain();
        check(std::any_of(authority.begin(),authority.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[1]==0&&p[2]==55&&p[3]==0&&p[4]==0&&p[5]==0;}));
        check(std::find(authority.begin(),authority.end(),down)==authority.end());
        check(packets.back()==chain&&down_at<packets.end()-1);
        Fixture lethal(npc,true,30);
        if(npc)check(lethal.world.degeneration_hex(lethal.caster,lethal.target,
            *skills::effects::degeneration_hex(101,0),100));
        else check(lethal.world.apply_condition(lethal.caster,lethal.target,
            combat::conditions::Kind::poison,100,5000));
        packets.clear();
        check(lethal.cast(1000,&packets)&&lethal.health()==0);
        check(std::find(packets.begin(),packets.end(),chain)==packets.end());
        check(!lethal.world.knocked_down(lethal.target,1000));
        check(lethal.world.dagger_stage(lethal.caster,lethal.target,1000)==0);
        Fixture stale(npc);auto wrong=stale.target;++wrong.generation;
        packets={{1,2}};
        check(!stale.world.iron_palm(stale.caster,wrong,*skills::effects::iron_palm(12),20,1000,&packets));
        check(packets.empty()&&stale.box->drain().empty());
        check(stale.health()==1000);
        Fixture distant(npc);
        distant.world.move(1,-1000,0,100);
        distant.world.tick(1000);
        packets={{1,2}};
        check(!distant.cast(1000,&packets)&&packets.empty());
        check(distant.health()==1000);
    }
    Fixture immune(true,false);
    check(immune.world.degeneration_hex(immune.caster,immune.target,
        *skills::effects::degeneration_hex(101,0),100));
    check(immune.cast()&&immune.health()==959);
    check(!immune.world.knocked_down(immune.target,1000));
    Fixture unknown(true,std::nullopt);
    check(unknown.world.degeneration_hex(unknown.caster,unknown.target,
        *skills::effects::degeneration_hex(101,0),100));
    std::vector<world::Packet> rejected{{1,2}};
    check(!unknown.cast(1000,&rejected)&&rejected.empty()&&unknown.health()==1000);
    Fixture player_hex(false);
    check(player_hex.world.degeneration_hex(player_hex.caster,player_hex.target,
        *skills::effects::degeneration_hex(101,0),100));
    check(player_hex.cast()&&player_hex.world.knocked_down(player_hex.target,1000));
    for(bool npc:{false,true}){
        Fixture expiry(npc);
        const auto clear=skills::effects::iron_palm_chain_2022(expiry.caster.actor,expiry.target.actor,0);
        check(expiry.cast(1000));expiry.box->drain();
        check(expiry.cast(1100));expiry.box->drain();
        expiry.world.tick(15999);
        auto before=expiry.box->drain();check(std::find(before.begin(),before.end(),clear)==before.end());
        expiry.world.tick(16000);
        auto early=expiry.box->drain();check(std::find(early.begin(),early.end(),clear)==early.end());
        expiry.world.tick(16100);
        auto due=expiry.box->drain();check(std::count(due.begin(),due.end(),clear)==1);
        expiry.world.tick(16200);
        auto repeated=expiry.box->drain();check(std::find(repeated.begin(),repeated.end(),clear)==repeated.end());
    }
    {
        Fixture replaced(false);
        const auto old_target=replaced.target;
        const auto clear=skills::effects::iron_palm_chain_2022(replaced.caster.actor,old_target.actor,0);
        check(replaced.cast(1000));replaced.box->drain();
        replaced.world.leave(old_target.actor);
        replaced.world.join(old_target.actor,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        check(*replaced.world.effect_scope(old_target.actor)!=old_target);
        replaced.box->drain();replaced.world.tick(16000);
        auto packets=replaced.box->drain();
        check(std::find(packets.begin(),packets.end(),clear)==packets.end());
    }
    std::cout<<"PASS Iron Palm World player/NPC damage, control, survival-gated chain publication and rejected scopes\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
