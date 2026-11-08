#include "gw_npc_ordinary_ai.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace ordinary=combat::ai::ordinary;namespace control=combat::npcs::control;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("NPC ordinary AI line "+std::to_string(at.line()));
}
const skills::effects::Scope source{1,100,1},target{1,2,1};
ordinary::Reviewed profile(){return {5,10,0,5,true,ordinary::Delivery::ranged,500,1500,1000,{100,32},"Authored test profile, not measured retail numbers"};}
ordinary::Pose pose(skills::effects::Scope scope,float x){return {scope,x,0,0,true,0};}
int main()try{
    {auto p=profile();p.evidence.clear();check(!p.valid());p=profile();p.period_ms=499;check(!p.valid());p=profile();p.damage_type=99;check(!p.valid());}
    {ordinary::Ledger ledger(source,profile());control::State state(source);
     auto token=*state.begin(source,control::Action::attack,100,600);
     check(!ledger.accept(source,token,1500)&&!ledger.accept({2,2,1},token,1500)&&!ledger.accept({1,65536,1},token,1500));
     auto wrong=token;wrong.kind=control::Action::spell;check(!ledger.accept(target,wrong,1500));
     check(ledger.accept(target,token,1500));
     const auto old=ledger.prepare_release(token,pose(source,0),pose(target,100),0,true,600);
     const auto newer=ledger.prepare_release(token,pose(source,0),pose(target,101),0,true,601);
     check(!ledger.commit_release(old)&&ledger.commit_release(newer));}
    {ordinary::Ledger ledger(source,profile(),1);control::State state(source);
     auto token=*state.begin(source,control::Action::attack,100,600);
     check(ledger.accept(target,token,1500));check(!ledger.accept(target,token,1500));
     check(ledger.prepare_release(token,pose(source,0),pose(target,100),state.epoch(),true,599).status==ordinary::ReleaseStatus::deferred);
     auto future=pose(target,100);future.clock=601;
     check(ledger.prepare_release(token,pose(source,0),future,state.epoch(),true,600).status==ordinary::ReleaseStatus::deferred);
     auto p=ledger.prepare_release(token,pose(source,0),pose(target,100),state.epoch(),true,600);
     check(p.status==ordinary::ReleaseStatus::ready&&p.release->arrives==1600);
     check(state.complete(token,600)&&ledger.commit_release(p));check(!ledger.commit_release(p));
     // Source cancellation after release cannot revoke the immutable flight.
     check(state.cancel(source,700));check(!ledger.ready(1600));
     auto impact=ledger.prepare_impact(p.attack.id,pose(target,100),true,1599);check(impact.status==ordinary::Contact::deferred&&!ledger.commit_impact(impact));
     impact=ledger.prepare_impact(p.attack.id,pose(target,132),true,1600);check(impact.status==ordinary::Contact::contact);
     check(ledger.commit_impact(impact)&&!ledger.commit_impact(impact)&&ledger.ready(1600));
     check(!ledger.accept(target,token,1500));}
    for(unsigned mode=0;mode<5;++mode){ordinary::Ledger ledger(source,profile());control::State state(source);
     auto token=*state.begin(source,control::Action::attack,100,600);check(ledger.accept(target,token,1500));
     auto victim=pose(target,100);auto attacker=pose(source,0);auto epoch=state.epoch();bool clear=true;
     if(mode==0)victim.scope.generation++;if(mode==1)victim.x=1001;if(mode==2)attacker.alive=false;if(mode==3)epoch++;if(mode==4)clear=false;
     auto p=ledger.prepare_release(token,attacker,victim,epoch,clear,600);
     check(p.status==ordinary::ReleaseStatus::stopped&&ledger.commit_release(p)&&ledger.released_ids().empty());}
    for(unsigned mode=0;mode<4;++mode){ordinary::Ledger ledger(source,profile());control::State state(source);
     auto token=*state.begin(source,control::Action::attack,100,600);check(ledger.accept(target,token,1500));
     auto p=ledger.prepare_release(token,pose(source,0),pose(target,100),0,true,600);check(ledger.commit_release(p));
     auto victim=pose(target,100);bool clear=true;
     if(mode==0)victim.scope.generation++;if(mode==1)victim.x=133;if(mode==2)clear=false;if(mode==3)victim.clock=1601;
     auto impact=ledger.prepare_impact(p.attack.id,victim,clear,1600);
     check(impact.status==(mode==0?ordinary::Contact::retired:mode==1?ordinary::Contact::miss:mode==2?ordinary::Contact::obstruction:ordinary::Contact::deferred));
     check(ledger.commit_impact(impact)==(mode!=3));}
    {auto p=profile();p.delivery=ordinary::Delivery::melee;ordinary::Ledger ledger(source,p);control::State state(source);
     auto token=*state.begin(source,control::Action::attack,100,600);check(ledger.accept(target,token,1500));
     auto release=ledger.prepare_release(token,pose(source,0),pose(target,100),0,true,600);
     check(release.release->arrives==600&&ledger.commit_release(release,false)&&ledger.released_ids().empty());}
    {ordinary::Ledger ledger(source,profile());control::State state(source);
     auto token=*state.begin(source,control::Action::attack,100,600);check(ledger.accept(target,token,1500));check(ledger.cancel(100));
     check(ledger.prepare_release(token,pose(source,0),pose(target,100),0,true,600).status==ordinary::ReleaseStatus::invalid);
     check(!ledger.ready(1599)&&ledger.ready(1600));
     token.sequence++;token.begins=UINT64_MAX-500;token.completes=UINT64_MAX;
     check(!ledger.accept(target,token,1500));}
    std::cout<<"NPC ordinary attack ledger passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
