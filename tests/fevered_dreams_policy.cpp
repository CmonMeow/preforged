#include "gw_fevered_dreams.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered line "+std::to_string(at.line()));}
int main()try{const Scope caster{7,9,1},target{7,1,2};Registry r(target),cr(caster);combat::conditions::State empty;
 for(bool pvp:{false,true})for(unsigned rank:{0u,12u,15u,20u}){const auto p=*fevered_dreams(rank,pvp);check(p.skill==(pvp?3289u:55u)&&p.dazed_duration==(pvp?0u:1000u*interpolate(1,3,rank)));auto a=prepare_fevered_dreams(r,cr,caster,target,1,p,100);check(a&&r.size()==0&&a->registry.size()==1&&current_fevered_application(*a,a->registry,caster,target,100)&&!current_fevered_application(*a,a->registry,{7,9,99},target,100));check(!owned_fevered_dreams(a->registry,a->owner,100+p.duration));
  auto unconditioned=fevered_condition_reaction(a->registry,a->owner,{target,1,100,true},empty,empty,FeveredDazedSpan::attempted_duration,true);check(unconditioned&&unconditioned->pulses.empty());
  auto bleed=empty;check(bleed.apply(Kind::bleeding,5,100,10000));auto applied=fevered_condition_reaction(a->registry,a->owner,{target,2,200,true},bleed,bleed,FeveredDazedSpan::attempted_duration,true);check(applied&&applied->pulses.size()==(pvp?0u:1u)&&applied->dazed_applied==!pvp&&applied->target.has(Kind::dazed,200)==!pvp);
  auto first=fevered_condition_reaction(a->registry,a->owner,{target,3,200,true},empty,bleed,FeveredDazedSpan::attempted_duration);check(first&&first->pulses.size()==1&&!first->dazed_applied&&first->pulses[0].duration==9900);
  auto poisoned=bleed;check(poisoned.apply(Kind::poison,6,200,5000));auto second=fevered_condition_reaction(a->registry,a->owner,{target,4,200,true},bleed,poisoned,FeveredDazedSpan::attempted_duration);check(second&&second->pulses.size()==(pvp?1u:2u)&&second->dazed_applied==!pvp);
  auto refresh=poisoned;check(refresh.apply(Kind::poison,6,300,20000));auto ignored=fevered_condition_reaction(a->registry,a->owner,{target,5,300,true},poisoned,refresh,FeveredDazedSpan::attempted_duration);check(ignored&&ignored->pulses.empty()&&!ignored->dazed_applied);
  auto lost=poisoned;lost.remove(Kind::bleeding);auto removed=fevered_condition_reaction(a->registry,a->owner,{target,6,300,true},poisoned,lost,FeveredDazedSpan::attempted_duration);check(removed&&removed->pulses.empty());
  auto renewal=prepare_fevered_dreams(a->registry,cr,caster,target,2,*fevered_dreams(0,!pvp),300);check(renewal&&renewal->removed.size()==1&&renewal->registry.size()==1&&!owned_fevered_dreams(renewal->registry,a->owner,300));
 }
 check(fevered_dreams(12)->duration==22000&&fevered_dreams(12,true)->duration==13000&&!fevered_dreams(21));
 // Mantra of Persistence extends the hex, never the generated Dazed timer.
 check(cr.apply(caster,{7,9,14,12,Category::stance,100,10100},Replace::category).accepted);auto extended=prepare_fevered_dreams(r,cr,caster,target,1,*fevered_dreams(12),100);check(extended&&extended->entry.expires==29580);Registry retired({7,9,99});check(!prepare_fevered_dreams(r,retired,caster,target,1,*fevered_dreams(12),100));
 // Both explicit renewal span policies preserve the target's longer timer.
 auto before=empty;check(before.apply(Kind::dazed,4,100,10000));auto after=before;check(after.apply(Kind::poison,5,200,5000));
 for(auto span:{FeveredDazedSpan::attempted_duration,FeveredDazedSpan::retained_remaining}){auto react=fevered_condition_reaction(extended->registry,extended->owner,{target,7,200,true},before,after,span);check(react&&react->dazed_renewed&&react->target.lookup(Kind::dazed)->expires==10100&&react->pulses.back().duration==(span==FeveredDazedSpan::attempted_duration?3000u:9900u));}
 FeveredEvents events(target);check(events.accept({target,1,200,true})&&!events.accept({target,1,201,true})&&!events.accept({target,2,199,true})&&events.accept({target,3,201,true}));check(!events.accept({{7,1,99},4,202,true})&&!events.accept({target,4,202,false}));
 auto forged=*fevered_dreams(12);++forged.dazed_duration;check(!prepare_fevered_dreams(r,cr,caster,target,1,forged,100));check(!prepare_fevered_dreams(r,cr,caster,caster,1,*fevered_dreams(12),100)&&!prepare_fevered_dreams(r,cr,caster,target,0,*fevered_dreams(12),100)&&!prepare_fevered_dreams(r,cr,caster,target,1,*fevered_dreams(12),UINT64_MAX-21999));
 std::cout<<"PASS Fevered PvE/PvP, new-vs-renewed conditions, application Dazed anomaly, explicit renewal span policies, owned aliases/Persistence and event lifetime guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
