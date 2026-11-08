#include "gw_well_corpse_cast.h"
#include <stdexcept>
#include <limits>
using namespace fw::gw::skills::effects;

template<class T> void check(const T& value){if(!static_cast<bool>(value))throw std::runtime_error("Well test failed");}
int main(){
    const Scope caster{42,25,3},corpse{42,70,8};
    const WellCorpseFact dead{corpse,11,0,100.f,200.f,true,true,false};
    for(const unsigned skill:{91u,92u,93u,94u})for(unsigned rank=0;rank<=20;++rank){
        const auto plan=well_cast_plan(skill,rank);check(plan);
        check(plan->energy==(skill==91?5u:skill==94?25u:10u));
        check(plan->activation==(skill==94?3000ull:1000ull));
        const auto claim=prepare_well_corpse_claim(*plan,caster,dead,true,true,1000);
        check(claim&&claim->corpse==corpse&&claim->corpse_revision==11);
        const auto consumed=consume_well_corpse_claim(*claim,dead);
        check(consumed&&consumed->consumed&&consumed->revision==12);
        check(!consume_well_corpse_claim(*claim,*consumed));
        auto recycled=dead;++recycled.scope.generation;
        check(!consume_well_corpse_claim(*claim,recycled));
        auto stale=dead;++stale.revision;
        check(!consume_well_corpse_claim(*claim,stale));
        auto shifted=dead;shifted.x+=1;
        check(!consume_well_corpse_claim(*claim,shifted));
        auto forged=*claim;++forged.plan.energy;
        check(!consume_well_corpse_claim(forged,dead));
    }
    check(!well_cast_plan(90,0)&&!well_cast_plan(95,0)&&!well_cast_plan(91,21));
    const auto plan=*well_cast_plan(92,12);
    check(!prepare_well_corpse_claim(plan,caster,dead,false,true,1000));
    check(!prepare_well_corpse_claim(plan,caster,dead,true,false,1000));
    check(!prepare_well_corpse_claim(plan,corpse,dead,true,true,1000));
    auto changed=dead;changed.dead=false;
    check(!prepare_well_corpse_claim(plan,caster,changed,true,true,1000));
    changed=dead;changed.eligible=false;
    check(!prepare_well_corpse_claim(plan,caster,changed,true,true,1000));
    changed=dead;changed.consumed=true;
    check(!prepare_well_corpse_claim(plan,caster,changed,true,true,1000));
    changed=dead;changed.x=std::numeric_limits<float>::quiet_NaN();
    check(!prepare_well_corpse_claim(plan,caster,changed,true,true,1000));
    changed=dead;changed.scope.instance=43;
    check(!prepare_well_corpse_claim(plan,caster,changed,true,true,1000));
    check(!prepare_well_corpse_claim(plan,caster,dead,true,true,0));
}
