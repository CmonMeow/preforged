#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("optional pet observer line "+std::to_string(at.line()));
}
int main()try{
    {
        warrior_test::Fixture f(340,false);auto& w=f.world;casting::Hooks h;unsigned observed=0;
        h.resources=[&](unsigned id){return w.resources(id);};h.events=[](const auto&){};
        h.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
        h.consume_energy=[&](unsigned n){return w.consume_energy(1,n,f.source);};
        h.effect_snapshot=[&](auto now){return *w.status_snapshot(f.source,now);};
        h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool present){return w.status_mutation(f.source,skill,effect,rank,now,until,present);};
        h.pet_offensive_completed=[&](auto,auto,unsigned,auto,auto){++observed;};
        // Existing self/untargeted client request with target0 and NO scope API.
        Slots bar{};bar[0]={344,0};Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[344]=1;attributes::Ranks ranks{};
        casting::Runtime runtime(1);check(runtime.start(1000,{344,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
        runtime.update(2000,h);check(observed==0);
    }
    for(unsigned mode=0;mode<5;++mode){
        unsigned healed=0,observed=0;std::uint64_t owner_generation=5,target_generation=8;
        const effects::Scope owner{1,1,5},target{1,2,8};casting::Hooks h;
        h.resources=[](unsigned id)->std::optional<vitals::Resources>{return id==1||id==2?std::optional(vitals::Resources{100,1000,100,100}):std::nullopt;};
        h.events=[](const auto&){};h.target_valid=[](unsigned id,float range,bool){return id==2&&range==1248;};
        h.consume_energy=[](unsigned){return true;};h.heal=[&](unsigned id,unsigned,float,bool){check(id==2);++healed;return true;};
        h.pet_offensive_completed=[&](auto a,auto b,unsigned skill,auto sequence,auto now){check(a==owner&&b==target&&skill==281&&sequence&&now==2000);++observed;};
        if(mode!=0)h.target_scope=[&](unsigned id)->std::optional<effects::Scope>{
            if(id==1)return effects::Scope{1,1,owner_generation};if(id==2&&mode!=1)return effects::Scope{1,2,target_generation};return {};
        };
        Slots bar{};bar[0]={281,0};Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[281]=1;attributes::Ranks ranks{};
        casting::Runtime runtime(1);check(runtime.start(1000,{281,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        if(mode==2)++target_generation;if(mode==3)++owner_generation;
        runtime.update(1999,h);check(!healed);runtime.update(2000,h);runtime.update(2000,h);
        check(healed==1&&observed==(mode==4?1u:0u));
    }
    std::cout<<"Optional pet observer preserves self/target0/legacy heal admission and suppresses absent or stale observation scopes\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
