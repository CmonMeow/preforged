#include "gw_succor.h"
#include "gw_overcast_activation.h"
#include <iostream>
#include <stdexcept>
#include <source_location>
#include <string>
using namespace fw::gw;
using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("Succor completion line "+std::to_string(at.line()));}
int main()try{
 const Scope caster{1,1,1},target{1,2,1};
 const auto app=prepare_succor(caster,target,7,100,Registry(target));check(bool(app));
 SuccorSpellConsumer consumer(app->lease);
 vitals::Resources resources{100,480,0,40};energy::Regeneration regeneration;
 regeneration.advance(resources,500,1); // one third Energy, zero whole points
 unsigned calls=0;std::uint64_t lost=0;
 auto debit=[&](Scope owner,unsigned amount){
  check(owner==caster&&amount==1);
  const auto proposal=overcast::prepare_loss(overcast::CeilingCurve(owner,100,1),owner,resources,regeneration,0,amount,100,0,overcast::LossRule::drain_to_zero);
  if(!proposal)return false;
  resources=proposal->state.resources;regeneration=proposal->state.regeneration;
  lost+=proposal->lost_units;++calls;return true;
 };
 check(consumer.completed(caster,target,app->registry,281,10,100,debit)==1);
 check(calls==1&&lost==50000&&resources.energy==0);
 check(!consumer.completed(caster,target,app->registry,281,10,100,debit)&&calls==1);
 check(!consumer.completed(caster,target,app->registry,281,9,101,debit)&&calls==1);
 check(consumer.completed(caster,target,app->registry,281,11,101,debit)==1&&calls==2&&lost==50000);
 // Speculative state can be discarded before production publication.
 auto speculative=consumer;
 check(!speculative.completed(caster,target,app->registry,281,12,102,[](Scope,unsigned){return false;}));
 check(speculative.sequence()==consumer.sequence());
 // Replacement cannot replay an old lease even at the same timestamp.
 const auto replacement=prepare_succor(caster,target,8,102,app->registry);check(bool(replacement));
 check(consumer.completed(caster,target,replacement->registry,281,12,102,debit)==0&&calls==2);
 SuccorSpellConsumer fresh(replacement->lease);
 check(!fresh.completed(caster,{1,2,2},replacement->registry,281,13,103,debit));
 check(fresh.completed(caster,target,replacement->registry,297,13,103,debit)==0&&calls==2);
 std::cout<<"PASS Succor fractional terminal debit, depleted Energy, duplicate ordering, retry and replacement boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
