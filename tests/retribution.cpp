#include "gw_retribution_contacts.h"
#include "gw_maintained_energy.h"
#include "gw_mending_wire.h"
#include <iostream>
#include <source_location>
#include <string>
#include <limits>
#include <fstream>
using namespace fw::gw;using namespace skills::effects;using namespace combat::weapon;
void check(bool ok,std::source_location where=std::source_location::current()){
 if(!ok)throw std::runtime_error("Retribution line "+std::to_string(where.line()));
}
combat::Hit hit(Scope target,double amount,double hp=500,double cap=1000000){
 combat::Actor actor{target.actor,target.generation,combat::Health{hp,1000},{},0,true};actor.incoming_damage_cap=cap;
 return combat::apply(actor,{combat::Kind::armor_ignoring,amount,20,1},true);
}
std::string hex(const combat::Packet& packet){std::string out;for(auto c:packet){out+="0123456789abcdef"[c>>4];out+="0123456789abcdef"[c&15];}return out;}
int main(int argc,char** argv)try{
 std::ofstream fixture;if(argc>1){fixture.open(argv[1]);check(bool(fixture));}
 const Scope caster{1,1,1},target{1,2,1},attacker{1,3,1};unsigned cases=0,reapplications=0;
 check(!retribution(21));
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=*retribution(rank);check(plan.maximum==5+rank&&plan.percent==33&&plan.energy==10&&plan.upkeep==1&&plan.activation==2000&&plan.aftercast==750&&plan.recharge==0);
  Registry registry(target);MaintainedEnchantments ownership;
  const auto application=prepare_retribution(plan,caster,target,1,100,registry,ownership);check(application.has_value());
  check(registry.size()==0&&ownership.size()==0&&application->registry.size()==1&&application->ownership.count(caster,100)==1);
  const auto& lease=application->lease;check(retribution_lease_matches(lease,application->registry,application->ownership,100));
  auto replaced_identity=application->registry;check(replaced_identity.apply(target,lease.entry,Replace::same_skill).accepted);
  check(!retribution_lease_matches(lease,replaced_identity,application->ownership,100));
  check(!prepare_retribution_end(lease,100,replaced_identity,application->ownership));
  auto forged_sequence=lease;++forged_sequence.application;
  check(!retribution_lease_matches(forged_sequence,application->registry,application->ownership,100));
  if(rank){const auto weaker=prepare_retribution(*retribution(rank-1),caster,target,2,200,application->registry,application->ownership);
   check(weaker&&weaker->lease.entry.rank==rank&&weaker->lease.entry.instance==2&&weaker->lease.application!=application->lease.application);}
  const auto wire=mending_wire(caster,target,lease.entry),ended_wire=mending_end_wire(caster,target,lease.entry);
  check(wire&&ended_wire&&wire->upkeep.size()==20&&wire->effect.size()==20&&wire->upkeep[0]==62&&wire->effect[0]==64);
  check(ended_wire->upkeep==combat::Packet({63,0,1,0,0,0,1,0,0,0})&&ended_wire->effect==combat::Packet({67,0,2,0,0,0,1,0,0,0}));
  if(fixture.is_open())fixture<<"{\"rank\":"<<rank<<",\"upkeep\":\""<<hex(wire->upkeep)<<"\",\"effect\":\""<<hex(wire->effect)<<"\",\"upkeep_end\":\""<<hex(ended_wire->upkeep)<<"\",\"effect_end\":\""<<hex(ended_wire->effect)<<"\"}\n";
  const ResolvedAttack terminal{attacker,target,1,110,200,AttackOutcome::hit,200,AttackDelivery::melee};
  for(double amount:{0.,.01,1.,3.,10.,30.,100.,1000.})for(double cap:{5.,1000000.})for(bool undead:{false,true}){
   ++cases;const auto settled=hit(target,amount,500,cap);
   const auto reaction=retribution_reaction(lease,terminal,settled,true,{undead,{}},20);
   if(amount==0){check(!reaction);continue;}
   check(reaction.has_value());
   const auto expected=std::min(double(plan.maximum),std::max(1.,std::floor(settled.damage*33/100)))*(undead?2:1);
   check(std::abs(reaction->damage.amount-expected)<1e-9&&reaction->damage.skill==248&&reaction->caster==caster&&reaction->attacker==attacker);
   check(reaction->damage.kind==combat::Kind::armor_ignoring&&combat::valid(reaction->damage));
  }
  // No reflection from blocked/missed/retired attacks, unknown allegiance or
  // traits, unqualified/forged contact, or damage for another actor lifetime.
  const auto valid_hit=hit(target,30);
  for(auto outcome:{AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction,AttackOutcome::target_retired,AttackOutcome::cancelled,AttackOutcome::replaced}){
   auto bad=terminal;bad.outcome=outcome;check(!retribution_reaction(lease,bad,valid_hit,true,{false,{}},20));++cases;
  }
  check(!retribution_reaction(lease,terminal,valid_hit,{}, {false,{}},20));
  check(!retribution_reaction(lease,terminal,valid_hit,false,{false,{}},20));
  check(!retribution_reaction(lease,terminal,valid_hit,true,{{},{}},20));
  check(!retribution_reaction(lease,terminal,valid_hit,true,{false,{}},0));
  for(unsigned mode=0;mode<10;++mode){
   ++cases;auto bad=terminal;auto damaged=valid_hit;auto retained=lease;
   if(mode==0)bad.event=0;if(mode==1)bad.before_damage.reset();if(mode==2)bad.before_damage=199;
   if(mode==3)++damaged.generation;if(mode==4)++damaged.target;if(mode==5)damaged.applied=0;
   if(mode==6)damaged.damage=std::numeric_limits<double>::quiet_NaN();if(mode==7)retained.entry.expires=201;
   if(mode==8)retained.entry.cause=attacker.actor;if(mode==9)retained.entry.applied=201;
   check(!retribution_reaction(retained,bad,damaged,true,{false,{}},20));
  }
  // Overkill differs from clipped Health loss. This is the documented private
  // damage-before-Health-clipping policy awaiting a retail combined receipt.
  const auto overkill=hit(target,100,1);check(overkill.applied==1);
  check(retribution_reaction(lease,terminal,overkill,true,{false,{}},20)->damage.amount==plan.maximum);
  check(retribution_reaction(lease,terminal,hit(target,11),true,{false,{}},20)->damage.amount==3);
  RetributionContacts contacts(target);
  check(contacts.release(target,attacker,1,110,AttackDelivery::melee));
  check(!contacts.release(target,attacker,1,110,AttackDelivery::melee));
  const QualifiedHit qualified{attacker,target,1,110,200,AttackDelivery::melee};
  check(contacts.prepare(target,lease,qualified,application->registry,application->ownership));
  check(!contacts.prepare(target,lease,qualified,application->registry,application->ownership));
  check(contacts.consume(target,terminal,valid_hit,true,{false,{}},20).has_value()&&contacts.size()==0);
  check(!contacts.consume(target,terminal,valid_hit,true,{false,{}},20));
  // Terminal mismatch is consumed before any caller can invoke a reaction.
  check(contacts.release(target,attacker,2,110,AttackDelivery::ranged));
  auto q=qualified;q.event=2;q.delivery=AttackDelivery::ranged;
  check(contacts.prepare(target,lease,q,application->registry,application->ownership));
  auto wrong=terminal;wrong.event=2;check(!contacts.consume(target,wrong,valid_hit,true,{false,{}},20));
  wrong.delivery=AttackDelivery::ranged;check(!contacts.consume(target,wrong,valid_hit,true,{false,{}},20));
  // Different releases may arrive out of order, without sharing one journal
  // across different attacker generations or recipient lifetimes.
  for(unsigned event:{3u,4u}){check(contacts.release(target,attacker,event,110,AttackDelivery::ranged));q.event=event;check(contacts.prepare(target,lease,q,application->registry,application->ownership));}
  for(unsigned event:{4u,3u}){auto t=wrong;t.event=event;check(contacts.consume(target,t,valid_hit,true,{false,{}},20).has_value());}
  check(contacts.release(target,attacker,5,110,AttackDelivery::melee));check(contacts.retire_attacker(target,attacker)&&contacts.size()==0);
  check(contacts.retire(target)&&!contacts.release(target,attacker,6,110,AttackDelivery::melee));
  const auto end=prepare_retribution_end(lease,200,application->registry,application->ownership);check(end&&end->registry.size()==0&&end->ownership.count(caster,200)==0);
  check(!prepare_retribution_end(lease,200,end->registry,end->ownership));
  auto forged=lease;forged.caster.generation++;check(!prepare_retribution_end(forged,200,application->registry,application->ownership));
  forged=lease;forged.entry.skill=290;check(!prepare_retribution_end(forged,200,application->registry,application->ownership));
  check(!prepare_retribution_end(lease,99,application->registry,application->ownership));
  // A second maintainer replaces the old scoped lease atomically.
  auto next=caster;next.actor=4;
  const auto replacement=prepare_retribution(plan,next,target,2,200,application->registry,application->ownership);
  check(replacement&&replacement->removed.size()==1&&replacement->ownership.count(caster,200)==0&&replacement->ownership.count(next,200)==1);
  check(!retribution_lease_matches(lease,replacement->registry,replacement->ownership,200));
  for(unsigned new_rank=0;new_rank<=20;++new_rank){++reapplications;
   const auto reapplied=prepare_retribution(*retribution(new_rank),next,target,2,200,application->registry,application->ownership);
   check(reapplied&&reapplied->lease.entry.rank==std::max(rank,new_rank)&&reapplied->lease.caster==next&&
         reapplied->ownership.count(caster,200)==0&&reapplied->ownership.count(next,200)==1&&
         !retribution_lease_matches(lease,reapplied->registry,reapplied->ownership,200));
   const ResolvedAttack contact{attacker,target,99,190,200,AttackOutcome::hit,200,AttackDelivery::melee};
   const auto damage=retribution_reaction(reapplied->lease,contact,hit(target,200),true,{false,false},20);
   check(damage&&damage->caster==next&&damage->damage.amount==5+std::max(rank,new_rank));
   const auto ended=prepare_retribution_end(reapplied->lease,200,reapplied->registry,reapplied->ownership);
   check(ended&&ended->ownership.count(next,200)==0);
  }
  // Shared upkeep counts real owned recipients; it is not a count of effects
  // on the caster. Dropping at empty Energy proposes the owned recipient end.
  const auto upkeep=application->ownership.prepare_upkeep(caster,0,true,200);
  check(upkeep&&upkeep->pips==0&&upkeep->releases.size()==1&&upkeep->releases[0].target==target&&upkeep->releases[0].effect==1);
  auto committed=application->ownership;check(committed.commit_upkeep(*upkeep)&&committed.count(caster,200)==0);
 }
 std::cout<<"PASS "<<cases<<" staged Retribution arithmetic/qualification and "<<reapplications<<" rank-preserving reapplications plus owned upkeep, replacement/end, duplicate and out-of-order contact tests; policy-only; no paid Runtime/gateway248 binding\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
