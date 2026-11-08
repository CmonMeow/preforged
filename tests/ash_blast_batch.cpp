#include "gw_ash_blast_batch.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Ash Blast batch line "+std::to_string(at.line()));
}
AshBlastRecipient recipient(Scope scope,unsigned effect,float distance,bool burning){
    AshBlastRecipient r{scope,effect,distance,Registry(scope),{}, {},true};
    r.actor.id=scope.actor;
    r.actor.generation=scope.generation;r.actor.health=combat::Health{480,480};
    r.actor.earth_armor=60;r.actor.damageable=true;r.hostile=true;
    if(burning)check(r.conditions.apply(combat::conditions::Kind::burning,25,100,6000));
    return r;
}
int main()try{
    const Scope caster{1,25,1},center{1,30,1},adjacent{1,31,1};
    const auto plan=*ash_blast(0);
    std::vector<AshBlastRecipient> targets{
        recipient(center,7,0,true),recipient(adjacent,8,166,false)};
    const auto batch=prepare_ash_blast_batch(plan,caster,center,targets,20,100);
    check(batch&&batch->targets.size()==2&&batch->center==center);
    check(batch->targets[0].second.hex.has_value());
    check(!batch->targets[1].second.hex.has_value());
    check(batch->targets[0].second.actor.health->current==445);
    check(batch->targets[1].second.actor.health->current==445);
    check(targets[0].actor.health->current==480&&targets[1].actor.health->current==480);
    check(targets[0].registry.size()==0&&targets[1].registry.size()==0);
    auto failed=targets;failed[1].actor.earth_armor.reset();
    check(!prepare_ash_blast_batch(plan,caster,center,failed,20,100));
    check(failed[0].actor.health->current==480&&failed[0].registry.size()==0);
    failed=targets;failed[1].effect=7;
    check(bool(prepare_ash_blast_batch(plan,caster,center,failed,20,100)));
    failed=targets;failed[1].scope=center;
    check(!prepare_ash_blast_batch(plan,caster,center,failed,20,100));
    failed=targets;failed[1].distance_from_center=166.01f;
    check(!prepare_ash_blast_batch(plan,caster,center,failed,20,100));
    failed=targets;failed[0].distance_from_center=1;
    check(!prepare_ash_blast_batch(plan,caster,center,failed,20,100));
    failed=targets;failed[1].hostile=false;
    check(!prepare_ash_blast_batch(plan,caster,center,failed,20,100));
    check(!prepare_ash_blast_batch(plan,caster,{1,32,1},targets,20,100));
    check(!prepare_ash_blast_batch(plan,caster,center,targets,0,100));
    std::cout<<"Ash Blast multi-target copy/preflight and all-or-nothing rejection pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
