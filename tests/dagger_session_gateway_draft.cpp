#include "dagger_successful_dual_fixture_draft.h"
#include "gw_dagger_session_gateway.h"
#include <utility>
// Compile-only API checks: no temporary borrowed optional may escape into hooks.
template<class Owner> concept GatewayOwnerArgument=requires(
        skills::casting::Hooks& hooks,world::Instance& instance,Owner&& owner){
 skills::casting::bind_dagger_session_gateway(hooks,instance,std::forward<Owner>(owner));
};
static_assert(GatewayOwnerArgument<std::optional<effects::Scope>&>);
static_assert(GatewayOwnerArgument<const std::optional<effects::Scope>&>);
static_assert(!GatewayOwnerArgument<std::optional<effects::Scope>>);
static_assert(!GatewayOwnerArgument<const std::optional<effects::Scope>>);

static void clear_gateway(skills::casting::Hooks& h){
 h.prepare_dagger_attack={};h.complete_dagger_attack={};h.complete_dagger_failed_second={};
 h.complete_dagger_successful_second={};h.cooldown_amendment_source={};
}
int main()try{
 using namespace skills::casting;
 for(unsigned id:{775u,776u,777u,778u,779u,780u,782u,783u,976u,3061u,3251u})check(!Runtime::effect_supported(id));
 {Fixture f(775);f.arm();clear_gateway(f.hooks);std::optional owner=f.source;
  check(!bind_dagger_session_gateway(f.hooks,f.world,owner));
  check(!f.hooks.prepare_dagger_attack&&!f.hooks.complete_dagger_attack&&!f.hooks.complete_dagger_failed_second&&
        !f.hooks.complete_dagger_successful_second&&!f.hooks.cooldown_amendment_source);
  check(f.start()==Reason::unsupported&&f.payments==0&&f.world.take_attack_outcomes(f.source).empty());}
 {Fixture f(775);clear_gateway(f.hooks);std::optional owner=f.source;f.hooks.cooldown_completion_source={};
  check(!bind_dagger_session_gateway(f.hooks,f.world,owner,DaggerSessionEnrollment::reviewed));
  check(!f.hooks.prepare_dagger_attack&&!f.hooks.complete_dagger_attack);}
 for(bool failed:{false,true}){
  Fixture f(775);if(!failed)f.arm();clear_gateway(f.hooks);std::optional owner=f.source;
  check(bind_dagger_session_gateway(f.hooks,f.world,owner,DaggerSessionEnrollment::reviewed));
  const auto prepare=f.hooks.prepare_dagger_attack;f.hooks.prepare_dagger_attack=[&](auto t,const auto& w,const auto& p,auto b,auto r){return f.last_permit=prepare(t,w,p,b,r);};
  const auto complete=f.hooks.complete_dagger_attack;f.hooks.complete_dagger_attack=[&](auto t,const auto& w,const auto& p,const auto& permit,auto at,unsigned copy){return f.last_result=complete(t,w,p,permit,at,copy);};
  check(f.start()==Reason::accepted);const auto first=f.last_permit->release_time();f.update(first);
  check(f.last_result.accepted&&f.last_result.prerequisite_failed==failed&&f.payments==1&&f.copied_observations==1);
  const auto due=failed?f.last_result.dual_failure_feedback->second_due():f.last_result.successful_second->due();
  if(failed){auto foreign_hooks=f.hooks;std::optional foreign_owner=f.target;
   check(bind_dagger_session_gateway(foreign_hooks,f.world,foreign_owner,DaggerSessionEnrollment::reviewed));
   check(!foreign_hooks.complete_dagger_failed_second(f.last_result.dual_failure_feedback,due).accepted);
   check(f.last_result.dual_failure_feedback->physical_open());}
  f.box->drain();f.update(due);check(!f.cast.active()&&f.payments==1&&f.copied_observations==1);
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),terminal(47,1))==1);
  auto stale=f.source;f.world.leave(1);f.world.join(1,"Replacement",0,0,f.box,0,{}, {},due+1,1);
  check(f.world.effect_scope(1)!=owner);check(!f.hooks.cooldown_amendment_source(1));
  check(!f.hooks.prepare_dagger_attack(f.target,f.weapon,*combat::weapon::attack_skill(775,12,0,0,true),due+1,due+501));
 }
 std::cout<<"dagger Session gateway candidate PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
