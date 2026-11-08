#include "gw_succor.h"
#include "gw_energy_regeneration.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool b,std::source_location l=std::source_location::current()){if(!b)throw std::runtime_error("Succor line "+std::to_string(l.line()));}
int main()try{
 const Scope caster{1,1,1},target{1,2,1};Registry empty(target);
 check(succor()&&succor()->health_pips==1&&succor()->energy_pips==1&&succor()->upkeep==1);
 const auto app=prepare_succor(caster,target,7,100,empty);check(app&&empty.size()==0&&app->registry.size()==1);
 const auto plan=succor_contribution(app->lease,caster,target,app->registry,100);check(plan.has_value());
 // The Energy contribution is usable by the existing fractional integrator.
 energy::Regeneration regen;vitals::Resources r{100,480,2,40};regen.advance(r,1500,int(plan->energy_pips));check(r.energy==2);regen.advance(r,1500,int(plan->energy_pips));check(r.energy==3);
 SuccorSpellConsumer consumer(app->lease);unsigned calls=0;
 auto loss=[&](Scope owner,unsigned amount){check(owner==caster&&amount==1);++calls;r.energy-=std::min(r.energy,amount);return true;};
 check(consumer.completed(caster,target,app->registry,281,1,100,loss)==1&&r.energy==2&&calls==1);
 check(!consumer.completed(caster,target,app->registry,281,1,100,loss)&&calls==1);
 check(consumer.completed(caster,target,app->registry,297,2,101,loss)==0&&calls==1); // signet
 check(consumer.completed(caster,target,app->registry,349,3,102,loss)==0&&calls==1); // stance
 check(!consumer.completed(caster,target,app->registry,281,4,101,loss));
 check(!consumer.completed({1,1,2},target,app->registry,281,4,103,loss));
 check(!consumer.completed(caster,{1,2,2},app->registry,281,4,103,loss));
 check(!consumer.completed(caster,target,app->registry,999999,4,103,loss));
 check(!consumer.completed(caster,target,app->registry,281,4,103,[](Scope,unsigned){return false;})&&consumer.sequence()==3);
 check(consumer.completed(caster,target,app->registry,281,4,103,loss)==1&&r.energy==1);
 check(consumer.completed(caster,target,app->registry,281,5,104,loss)==1&&r.energy==0);
 check(consumer.completed(caster,target,app->registry,281,6,105,loss)==1&&r.energy==0); // loss floors at zero in authority
 auto removed=app->registry;check(removed.remove(target,7).has_value());
 check(!succor_contribution(app->lease,caster,target,removed,106));
 check(consumer.completed(caster,target,removed,281,7,106,loss)==0&&calls==4);
 check(!prepare_succor(caster,caster,8,100,Registry(caster)));
 check(!prepare_succor({2,1,1},target,8,100,empty));check(!prepare_succor(caster,{1,2,2},8,100,empty));
 check(!prepare_succor({1,1,0},target,8,100,empty));check(!prepare_succor(caster,target,0,100,empty));
 check(!succor_contribution(app->lease,caster,target,app->registry,99));check(!succor_contribution(app->lease,{1,1,2},target,app->registry,100));
 const auto replacement=prepare_succor({1,3,1},target,8,106,app->registry);check(replacement&&replacement->removed.size()==1&&!succor_contribution(app->lease,caster,target,replacement->registry,106));
 auto forged=app->registry;check(forged.apply(target,{7,1,308,1,Category::enchantment,100,{}},Replace::same_skill).accepted);check(!succor_contribution(app->lease,caster,target,forged,100));
 forged=app->registry;check(forged.apply(target,{7,1,308,0,Category::enchantment,100,200},Replace::same_skill).accepted);check(!succor_contribution(app->lease,caster,target,forged,100));
 std::cout<<"PASS detached Succor registry, scoped modifiers, fractional Energy consumer, exactly-once spell loss, removal/replacement and stale lifetime guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
