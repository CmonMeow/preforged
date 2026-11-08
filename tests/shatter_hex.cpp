#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool v,std::source_location at=std::source_location::current()){
    if(!v)throw std::runtime_error("Shatter Hex regression line "+std::to_string(at.line()));
}
int main()try{
    using namespace skills;using namespace effects;
    check(shatter_hex(0)->damage==30&&shatter_hex(12)->damage==102&&shatter_hex(20)->damage==150&&!shatter_hex(21));
    check(shatter_hex(12)->ally&&shatter_hex(12)->radius==252&&!shatter_delusions(12)->ally);
    const std::string hash(64,'a');std::ostringstream placements,meta;
    placements<<"gw-npcs 1\n";meta<<"gw-npc-combat 2 "<<hash<<"\nsource test authored "<<hash<<" tests/shatter_hex.cpp\n";
    for(unsigned id:{100u,101u,102u}){
        const auto x=id==100?352:id==101?353:100;
        placements<<"npc 146 "<<id<<" 1 1234 0 1677721600 0 8 1 1 0 "<<x<<" 0 0 - -\n";
        meta<<"npc 146 "<<id<<" 1 1234 0 0 8 1 "<<(id==102?"protected - 1000 1000 80 0 0 test test test - -":"hostile-to-players - 1000 1000 80 1 1 test test test - test")<<" 60 60 test test\n";
    }
    std::istringstream p(placements.str()),m(meta.str());auto catalog=world::load_npc_catalog(p);
    world::bind_combat_catalog(catalog,combat::npcs::load_catalog(m,hash),hash);
    for(unsigned mode:{0u,1u,2u,3u}){
        world::Instance w;w.configure_npcs(catalog.for_map(146));
        for(unsigned id=1;id<=6;++id){const auto x=id==2?100:id==3?352:id==4?353:id==5?100:0;
            w.join(id,"Player",float(x),0,std::make_shared<world::Outbox>(),0,{}, {},100,id>=3&&id<=5?2:1);
            w.set_vitals(id,{1000,1000,100,100},100);
        }
        const auto caster=*w.effect_scope(1),ally=*w.effect_scope(2),enemy=*w.effect_scope(5);
        const auto plan=*shatter_hex(12);check(w.valid_shatter_ally(caster,2,1248,100)&&w.valid_shatter_ally(caster,102,1248,100));
        check(!w.valid_shatter_ally(caster,5,1248,100)&&!w.valid_shatter_ally(caster,100,1248,100));
        // Oldest Mesmer hex; newest Necromancer hex. Shatter Hex removes the
        // newest regardless of profession, and removes exactly one.
        if(mode){check(w.degeneration_hex(enemy,ally,*degeneration_hex(31,0),100));
            if(mode!=3)check(w.degeneration_hex(enemy,ally,*degeneration_hex(129,0),100));}
        auto stale=ally;++stale.generation;check(!w.shatter_delusions(caster,stale,plan,20,100));
        auto bad=plan;++bad.damage;check(!w.shatter_delusions(caster,ally,bad,20,100));
        check(!w.shatter_delusions(caster,enemy,plan,20,100));
        // Mode2 waits until all hexes expire; it must not trigger damage.
        const auto now=mode==2?20000u:100u;
        check(w.shatter_delusions(caster,ally,plan,20,now));
        const auto damage=mode==1||mode==3?102u:0u;
        check(w.resources(3)->health==1000-damage&&w.resources(4)->health==1000);
        check(w.resources(5)->health==1000-damage&&w.resources(6)->health==1000);
        check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==1000-damage);
        check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==1000);
        check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==1000);
        const auto remaining=*w.status_snapshot(ally,now);
        check(remaining.size()==(mode==1?1u:0u));if(mode==1)check(remaining[0].skill==31);
        check(w.shatter_delusions(caster,*w.effect_scope(102),plan,20,now)); // Valid protected ally, no hex.
    }
    for(unsigned rank:{0u,12u,20u}){
        world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u})w.set_vitals(id,{1000,1000,100,100},100);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.wound_spell(b,a,*wound_spell(44,12),20,100));
        check(w.valid_shatter_ally(a,1,1248,100)&&w.shatter_delusions(a,a,*shatter_hex(rank),20,100));
        check(w.has_condition(a,combat::conditions::Kind::deep_wound,100));
        check(w.resources(2)->health==1000-shatter_hex(rank)->damage);
        check(w.status_snapshot(a,100)->empty());
    }
    // Actual Runtime: one second activation,10Energy,10second recharge,
    // cancel/failure paths and stale target lifetime protection.
    for(unsigned mode:{0u,1u,2u,3u,4u}){
        using namespace casting;world::Instance w;auto box=std::make_shared<world::Outbox>();
        w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(3,"Foe",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        for(unsigned id:{1u,2u,3u})w.set_vitals(id,{1000,1000,100,100},100);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2),e=*w.effect_scope(3);
        if(mode!=4)check(w.degeneration_hex(e,b,*degeneration_hex(129,0),100));
        Hooks h;std::vector<Packet> packets;h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[&](auto n){return w.consume_energy(1,n);};
        h.target_scope=[&](auto id){return w.effect_scope(id);};h.shatter_ally_valid=[&](auto id,float range,auto at){return w.valid_shatter_ally(a,id,range,at);};
        h.shatter_delusions=[&](auto target,const auto& plan,auto at){return w.shatter_delusions(a,target,plan,20,at);};
        Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[67]=1;Slots bar{};bar[0]={67,0};attributes::Ranks ranks{};ranks[2]=12;Runtime runtime(1);
        check(runtime.start(100,{67,0,3},bar,policy,ranks,true,h)==casting::Reason::target);
        check(w.resources(1)->energy==100);
        check(runtime.start(100,{67,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(w.resources(1)->energy==90);
        if(mode==1)runtime.cancel(h);
        if(mode==2){w.leave(2);w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);}
        if(mode==3)runtime.interrupt(500,h);
        runtime.update(1100,h);check(w.resources(3)->health==(mode?1000u:898u));
        if(!mode){check(w.status_snapshot(b,1100)->empty());
            check(std::find(packets.begin(),packets.end(),slot_event(228,1,{67,0},find_skill(67)->recharge))!=packets.end());
            auto out=box->drain();const auto particle=*particles::targeted_body_2022(2,1,67);check(std::count(out.begin(),out.end(),particle)==1);}
    }
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
