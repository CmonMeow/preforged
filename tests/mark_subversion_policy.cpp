#include "gw_mark_subversion.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mark Subversion policy line "+std::to_string(at.line()));
}
int main()try{
 const Scope owner{1,2,3},ally{1,4,5};
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=mark_subversion(rank);check(plan&&plan->amount==interpolate(10,92,rank)&&plan->duration==6000&&plan->recharge==30000);
  Registry registry(owner);check(registry.apply(owner,{1,1,127,rank,Category::hex,100,6100},Replace::same_skill).accepted);
  const SubversionSpellTarget selected{owner,ally,SpellTargetRelation::ally};
  const auto r=mark_subversion_reaction(registry,owner,281,selected,1000);check(r&&r->plan==*plan&&r->hex.cause==1);
  check(mark_subversion_reaction(registry,owner,281,{owner,owner,SpellTargetRelation::ally},1000).has_value());
  for(auto relation:{SpellTargetRelation::unknown,SpellTargetRelation::foe})check(!mark_subversion_reaction(registry,owner,281,{owner,ally,relation},1000));
  for(unsigned skill:{0u,1u,320u,456u})check(!mark_subversion_reaction(registry,owner,skill,selected,1000));
  auto stale=owner;++stale.generation;
  check(!mark_subversion_reaction(registry,stale,281,selected,1000));
  check(!mark_subversion_reaction(registry,owner,281,{owner,stale,SpellTargetRelation::ally},1000));
  auto foreign=ally;++foreign.instance;
  check(!mark_subversion_reaction(registry,owner,281,{owner,foreign,SpellTargetRelation::ally},1000));
  check(!mark_subversion_reaction(registry,owner,281,selected,99)&&!mark_subversion_reaction(registry,owner,281,selected,6100));
  const auto transfer=steal_life_amount(plan->amount,{50,500},{500,500});check(transfer&&transfer->stolen==plan->amount);
 }
 check(!mark_subversion(21));
 {
  const Scope caster{1,1,7};const MarkSubversionOwnership credit{caster,owner,1,100};
  Registry empty(owner);const auto plan=*mark_subversion(12);
  const auto applied=prepare_mark_subversion_application(empty,caster,owner,1,plan,100);
  check(applied&&applied->entry.expires==6100&&!empty.lookup(owner,1));
  const auto replaced=prepare_mark_subversion_application(applied->registry,Scope{1,6,9},owner,2,*mark_subversion(0),200);
  check(replaced&&replaced->change.removed.size()==1&&replaced->entry.cause==6);
  auto forged=plan;++forged.amount;check(!prepare_mark_subversion_application(empty,caster,owner,1,forged,100));
  check(!prepare_mark_subversion_application(empty,caster,owner,1,plan,UINT64_MAX-5999));
  check(!prepare_mark_subversion_application(applied->registry,caster,owner,1,plan,200));
  const auto wrong_duration=hex_duration(128,10000,caster,{},100);
  check(wrong_duration&&!prepare_mark_subversion_application(empty,caster,owner,1,plan,100,wrong_duration));
  Registry registry(owner);check(registry.apply(owner,{1,1,127,12,Category::hex,100,6100},Replace::same_skill).accepted);
  const SubversionSpellTarget selected{owner,ally,SpellTargetRelation::ally};
  const auto prepare=[&](const Registry& r,const MarkSubversionOwnership& c,Scope source,Scope victim,Scope target,unsigned spell,std::uint64_t event,Time now,combat::Health health= {50,500}){
   return prepare_mark_subversion_failure(r,c,source,victim,target,spell,selected,event,now,health,{500,500});
  };
  const auto result=prepare(registry,credit,caster,owner,ally,281,1,1000);
  check(result&&result->health.stolen==76&&result->health.gained==76&&result->event==1&&result->consumed.instance==1);
  check(mark_subversion_reaction(registry,owner,281,selected,1000).has_value());
  check(!mark_subversion_reaction(result->registry,owner,281,selected,1000));
  check(!prepare(result->registry,credit,caster,owner,ally,281,2,1000));
  auto stale=caster;++stale.generation;check(!prepare(registry,credit,stale,owner,ally,281,1,1000));
  stale=owner;++stale.generation;check(!prepare(registry,credit,caster,stale,ally,281,1,1000));
  stale=ally;++stale.generation;check(!prepare(registry,credit,caster,owner,stale,281,1,1000));
  check(!prepare(registry,credit,caster,owner,ally,281,0,1000));
  check(!prepare(registry,credit,caster,owner,ally,281,1,6100));
  check(!prepare(registry,credit,caster,owner,ally,320,1,1000));
  check(!prepare(registry,credit,caster,owner,ally,281,1,1000,{0,500}));
  auto wrong=credit;++wrong.effect;check(!prepare(registry,wrong,caster,owner,ally,281,1,1000));
  wrong=credit;--wrong.applied;check(!prepare(registry,wrong,caster,owner,ally,281,1,1000));
  const auto capped=prepare(registry,credit,caster,owner,ally,281,1,1000,{500,500});check(capped&&capped->health.stolen==76&&capped->health.gained==0);
  const auto lethal=prepare_mark_subversion_failure(registry,credit,caster,owner,ally,281,selected,1,1000,{50,500},{10,500});
  check(lethal&&lethal->health.killed&&lethal->health.stolen==11&&lethal->health.gained==11);
 }
 std::cout<<"PASS Mark of Subversion native policy/ranks,scoped allied spell target predicate,expiry and shared life steal;one-use failure/World/Runtime/rendering pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
