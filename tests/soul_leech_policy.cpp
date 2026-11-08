#include "gw_soul_leech.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Soul Leech policy line "+std::to_string(at.line()));
}
int main()try{
 const Scope owner{1,2,3};
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=soul_leech(rank);check(bool(plan));
  check(plan->amount==interpolate(16,80,rank)&&plan->duration==10000&&plan->energy==10&&plan->recharge==15000);
  Registry registry(owner);check(registry.apply(owner,{1,1,128,rank,Category::hex,100,10100},Replace::same_skill).accepted);
  for(unsigned spell:{281u,28u,157u,903u}){
   const auto r=soul_leech_reaction(registry,owner,spell,1000);check(r&&r->hex.cause==1&&r->plan==*plan);
   const auto transfer=steal_life_amount(r->plan.amount,{50,200},{500,500});
   check(transfer&&transfer->stolen==plan->amount&&transfer->source.current==50+plan->amount);
  }
  for(unsigned other:{0u,1u,320u,456u})check(!soul_leech_reaction(registry,owner,other,1000));
  check(!soul_leech_reaction(registry,owner,281,99));
  check(soul_leech_reaction(registry,owner,281,100).has_value());
  check(!soul_leech_reaction(registry,owner,281,10100));
  auto stale=owner;++stale.generation;check(!soul_leech_reaction(registry,stale,281,1000));
  check(registry.apply(owner,{2,4,128,0,Category::hex,200,10200},Replace::same_skill).accepted);
  check(soul_leech_reaction(registry,owner,281,1000)->hex.cause==4);
 }
 check(!soul_leech(21));
 {
  const Scope source{1,1,7};Registry original(owner);const auto plan=*soul_leech(12);
  const auto staged=prepare_soul_leech_application(original,source,owner,10,plan,100);
  check(staged&&staged->change.accepted&&staged->change.removed.empty()&&staged->entry.expires==10100);
  check(!soul_leech_reaction(original,owner,281,100)); // Proposal never mutates input.
  check(soul_leech_reaction(staged->registry,owner,281,100)->plan.amount==67);
  const Scope next_source{1,4,8};
  const auto renewed=prepare_soul_leech_application(staged->registry,next_source,owner,11,*soul_leech(0),200);
  check(renewed&&renewed->change.removed.size()==1&&renewed->change.removed[0].instance==10);
  check(soul_leech_reaction(renewed->registry,owner,281,1000)->hex.cause==4);
  check(soul_leech_reaction(staged->registry,owner,281,1000)->hex.cause==1);
  auto forged=plan;++forged.amount;
  check(!prepare_soul_leech_application(original,source,owner,10,forged,100));
  check(!prepare_soul_leech_application(original,source,owner,10,plan,UINT64_MAX-9999));
  check(!prepare_soul_leech_application(staged->registry,source,owner,10,plan,200));
  auto stale=owner;++stale.generation;
  check(!prepare_soul_leech_application(original,source,stale,10,plan,100));
  check(!prepare_soul_leech_application(original,owner,owner,10,plan,100));
  auto foreign=source;++foreign.instance;
  check(!prepare_soul_leech_application(original,foreign,owner,10,plan,100));
  check(!prepare_soul_leech_application(original,source,owner,0,plan,100));
  const auto duration=hex_duration(128,10000,source,{},100);
  check(duration&&prepare_soul_leech_application(original,source,owner,10,plan,100,duration)->entry.expires==10100);
  const auto wrong=hex_duration(28,10000,source,{},100);
  check(wrong&&!prepare_soul_leech_application(original,source,owner,10,plan,100,wrong));
  const SoulLeechOwnership credit{source,owner,10,100};
  const auto transfer=prepare_soul_leech_transfer(staged->registry,credit,source,owner,281,1,200,{50,200},{500,500});
  check(transfer&&transfer->event==1&&transfer->at==200&&transfer->health.stolen==67&&transfer->health.gained==67);
  auto reused_source=source;++reused_source.generation;
  check(!prepare_soul_leech_transfer(staged->registry,credit,reused_source,owner,281,1,200,{50,200},{500,500}));
  check(!prepare_soul_leech_transfer(staged->registry,credit,source,stale,281,1,200,{50,200},{500,500}));
  check(!prepare_soul_leech_transfer(staged->registry,credit,source,owner,281,0,200,{50,200},{500,500}));
  auto old=credit;--old.applied;
  check(!prepare_soul_leech_transfer(staged->registry,old,source,owner,281,1,200,{50,200},{500,500}));
  old=credit;++old.effect;
  check(!prepare_soul_leech_transfer(staged->registry,old,source,owner,281,1,200,{50,200},{500,500}));
  check(!prepare_soul_leech_transfer(renewed->registry,credit,source,owner,281,1,200,{50,200},{500,500}));
  check(!prepare_soul_leech_transfer(staged->registry,credit,source,owner,281,1,10100,{50,200},{500,500}));
  check(!prepare_soul_leech_transfer(staged->registry,credit,source,owner,281,1,200,{0,200},{500,500}));
  check(!prepare_soul_leech_transfer(staged->registry,credit,source,owner,320,1,200,{50,200},{500,500}));
  check(!prepare_soul_leech_transfer(staged->registry,credit,source,owner,281,1,200,{50,200},{500,500},-1));
  const auto reduced=prepare_soul_leech_transfer(staged->registry,credit,source,owner,281,1,200,{50,200},{500,500},15);
  check(reduced&&reduced->health.stolen==52&&reduced->health.gained==52);
 }
 const auto lethal=steal_life_amount(soul_leech(12)->amount,{50,200},{10,500});
 check(lethal&&lethal->killed&&lethal->stolen==11&&lethal->gained==11);
 const auto capped=steal_life_amount(soul_leech(12)->amount,{200,200},{500,500});
 check(capped&&capped->stolen==67&&capped->gained==0);
 check(!steal_life_amount(67,{0,200},{500,500}));
 std::cout<<"PASS Soul Leech native policy,rank0..20,spell-only scoped live hex,expiry/replacement and existing life-steal authority;World/Runtime/rendering pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
