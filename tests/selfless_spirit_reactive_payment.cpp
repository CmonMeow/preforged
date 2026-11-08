#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills::effects;

static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Selfless reactive payment line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;
    Scope owner,ally,foe;
    Fixture(unsigned alias,unsigned energy,bool lethal,bool pvp){
        for(unsigned id:{1u,2u,3u}){
            w.join(id,"Actor",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},1000,id==3?2:1);
            w.set_vitals(id,{id==1&&lethal?1u:500u,500,id==1?energy:100u,100},100);
        }
        owner=*w.effect_scope(1);ally=*w.effect_scope(2);foe=*w.effect_scope(3);
        check(w.complete_selfless_spirit(owner,*selfless_spirit(alias),0,1000));
        check(w.apply_mind_wrack(foe,owner,12,pvp,1000));
    }
    void reacted(bool lethal){
        check(w.resources(1)->energy==0);
        check(w.resources(1)->health==(lethal?0u:417u));
        const auto statuses=w.status_snapshot(owner,1000);check(bool(statuses));
        for(const auto& status:*statuses)check(status.skill!=49&&status.skill!=2734);
        // A later zero-cost observation cannot replay the terminal damage.
        if(!lethal){check(w.consume_energy(1,0,owner,1000));check(w.resources(1)->health==417);}
    }
};
int main()try{
    for(unsigned alias:{1952u,2095u})for(bool pvp:{false,true})for(bool lethal:{false,true}){
        // Identical payment/resource clock excludes fractional regeneration:
        // ten ordinary Energy becomes seven only for the semantic other ally.
        {Fixture f(alias,7,lethal,pvp);
            check(f.w.consume_selfless_energy(f.owner,{194,0},f.ally,10,1000));
            f.reacted(lethal);
        }
        {Fixture f(alias,7,lethal,pvp);
            check(f.w.activate_selfless_overcast_energy(f.owner,219,f.ally,10,1,1000));
            f.reacted(lethal);
            if(!lethal){
                check(f.w.overcast_energy_snapshot(f.owner)->curve.burden_units()==500000);
                check(!f.w.activate_selfless_overcast_energy(f.owner,219,f.ally,0,1,1000));
                check(f.w.resources(1)->health==417);
            }
        }
        for(unsigned glyph:{198u,199u,203u}){
            const unsigned cost=glyph==199?7:17;
            Fixture f(alias,cost,lethal,pvp);
            check(f.w.apply_elemental_glyph(f.owner,50,*elemental_glyph(glyph),1000));
            const auto expected=f.w.elemental_glyph_snapshot(f.owner,1000);check(bool(expected));
            const auto paid=f.w.pay_selfless_elemental_glyph_use(f.owner,{194,0},f.ally,20,1,1000,expected);
            check(bool(paid)!=lethal);
            f.reacted(lethal);
            if(lethal){
                check(!f.w.elemental_glyph_snapshot(f.owner,1000));
                check(!f.w.settle_elemental_glyph_use(f.owner,1,GlyphOutcome::completed,1000).accepted);
            }else{
                check(paid->energy_cost==cost);
                check(!f.w.pay_selfless_elemental_glyph_use(f.owner,{194,0},f.ally,0,2,1000,expected));
                check(f.w.settle_elemental_glyph_use(f.owner,1,GlyphOutcome::completed,1000).accepted);
                check(!f.w.settle_elemental_glyph_use(f.owner,1,GlyphOutcome::completed,1000).accepted);
                check(f.w.resources(1)->health==417);
            }
        }
    }
    for(unsigned alias:{1952u,2095u}){
        // A floored zero debit leaves a positive balance and the hex intact.
        Fixture f(alias,1,false,false);
        check(f.w.consume_selfless_energy(f.owner,{194,0},f.ally,2,1000));
        check(f.w.resources(1)->energy==1&&f.w.resources(1)->health==500);
        check(f.w.consume_energy(1,1,f.owner,1000));f.reacted(false);
    }
    std::cout<<"PASS Selfless World ordinary/Overcast/glyph exact-zero Mind Wrack, aliases, PvE/PvP, lethal cleanup, lease/replay and zero-floor cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
