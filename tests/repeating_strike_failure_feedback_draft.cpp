#include "dagger_successful_dual_fixture_draft.h"
// Native159/kind38/reason2 is approved family inference for976, not a direct
//976 retail receipt or an English/localized UI assertion.
static void verify_failed(Fixture& f,unsigned expected_payments,unsigned expected_copy){
 const auto& result=f.last_result;check(result.accepted&&result.prerequisite_failed&&result.reset_recharge);
 check(result.failure_feedback&&!result.dual_failure_feedback&&!result.successful_second);
 check(result.outcomes.size()==1&&result.outcomes[0].skill_failed&&
       result.outcomes[0].outcome==combat::weapon::AttackOutcome::cancelled);
 check(f.until()==0&&f.payments==expected_payments&&f.paid==5*expected_payments&&f.refunds==0);
 const auto receipt=f.world.cooldown_completion_stamp(f.source,f.last_use_event);
 check(receipt&&receipt->slot==skills::Slot{976,expected_copy}&&receipt->base_ms==0&&receipt->extra_ms==0);
 check(effects::completion_deadline(*receipt)==f.clock&&f.copied_completion==receipt);
 const auto snapshot=f.cast.cooldown_source_snapshot(f.source,f.clock,f.hooks);
 check(snapshot&&f.world.publish_local_cooldowns(f.source,*snapshot));
 check(!f.world.complete_dagger_cast_draft(f.source,f.target,f.weapon,
       *combat::weapon::attack_skill(976,12,0,expected_copy,true),f.last_permit,f.clock,expected_copy).accepted);
 check(!result.failure_feedback->consume(f.source,f.target,976,expected_copy,f.last_permit));
 const auto ledger=f.world.take_attack_outcomes(f.source);check(ledger.size()==1&&ledger[0].skill_failed);
}
static void base_case(bool direct_world_consume){
 Fixture f(976);check(!skills::casting::Runtime::effect_supported(976));
 const auto complete=f.hooks.complete_dagger_attack;
 std::optional<std::vector<std::uint8_t>> direct;
 f.hooks.complete_dagger_attack=[&](auto target,const auto& weapon,const auto& plan,const auto& permit,auto at,unsigned copy){
  // Wrong caller copy must fail BEFORE claiming the exact original permit.
  check(!f.world.complete_dagger_cast_draft(f.source,target,weapon,plan,permit,at,copy+1).accepted&&permit->open());
  auto result=complete(target,weapon,plan,permit,at,copy);check(bool(result.failure_feedback));
  auto foreign=f.source;++foreign.generation;check(!result.failure_feedback->consume(foreign,target,976,copy,permit));
  foreign=target;++foreign.generation;check(!result.failure_feedback->consume(f.source,foreign,976,copy,permit));
  check(!result.failure_feedback->consume(f.source,target,976,copy+1,permit));
  check(!result.failure_feedback->consume(f.source,target,780,copy,permit));
  check(!result.failure_feedback->consume(f.source,target,976,copy,{}));
  if(direct_world_consume){direct=result.failure_feedback->consume(f.source,target,976,copy,permit);
   check(direct==combat::weapon::dagger_failure_wire_draft(f.source,target));
   // Separate World-capability test; Runtime path below must consume itself.
   result.failure_feedback.reset();}
  return result;
 };
 check(f.start()==skills::casting::Reason::accepted);const auto hp=f.health();f.box->drain();f.update(f.last_permit->release_time());
 check(f.health()==hp&&!f.cast.active()&&f.copied_observations==1);verify_failed(f,1,0);
 const auto expected=*combat::weapon::dagger_failure_wire_draft(f.source,f.target);check(expected.size()==18);
 const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),expected)==(direct_world_consume?0:1));
 check(std::count(packets.begin(),packets.end(),skills::casting::terminal(46,1))==1);
 check(std::count(packets.begin(),packets.end(),skills::casting::terminal(47,1))==0);
 f.update(f.clock);check(f.box->drain().empty()&&f.payments==1&&f.copied_observations==1);
}
static void echo_copy_case(){
 Fixture f(976);f.arm();f.policy.secondary=5;f.policy.learned[74]=1;f.bar[1]={74,0};
 check(f.world.configure_adrenaline(f.source,f.bar,f.clock));
 effects::TemporarySkillSession session(f.source,f.bar);effects::TemporarySkillIds ids(f.source);
 const auto echo=*effects::echo(74);
 check(f.world.status_mutation(f.source,74,100,0,f.clock,f.clock+echo.armed_duration,true,skills::Slot{74,0}));
 check(f.start()==skills::casting::Reason::accepted);const auto first=f.last_permit->release_time();f.update(first);
 const auto base_receipt=f.copied_completion;
 const auto grant=effects::commit_echo_copy(f.world,session,ids,f.source,{74,0},{976,0},first,f.last_use_event);
 check(grant&&grant->changed&&grant->changed->after==skills::Slot{976,1});f.bar=session.bar();f.active_copy=1;
 f.hooks.admit_session_cast=[&](const auto& bar,const auto& policy,auto request,auto at)->std::optional<effects::HexCopySession::CastAdmission>{
  if(bar!=session.bar())return {};const auto found=std::find(bar.begin(),bar.end(),skills::Slot{request.skill,request.copy});
  if(found==bar.end())return {};return session.admit(f.source,policy,unsigned(found-bar.begin()),*found,at);
 };
 f.hooks.valid_session_cast=[&](const auto& admission,auto at){return session.valid(admission,f.policy,at);};
 // A genuine new target has no retained off-hand stage: real late failure,
 // not miss/block and not a fabricated chain/expired copied grant.
 f.clock=first+1;f.world.join(3,"Copy failure foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},f.clock,2);
 f.world.set_vitals(3,{1000,1000,30,30},100);f.target=*f.world.effect_scope(3);check(f.world.physical_armor(f.target,60));
 check(f.world.ordinary_weapon(f.target,std::nullopt,20,f.clock,combat::weapon::CriticalProfile{1,0,20}));
 f.world.take_attack_outcomes(f.source);f.box->drain();check(f.start()==skills::casting::Reason::accepted);f.box->drain();
 f.update(f.last_permit->release_time());check(f.health()==1000&&f.copied_observations==2);verify_failed(f,2,1);
 const auto packets=f.box->drain();const auto wire=*combat::weapon::dagger_failure_wire_draft(f.source,f.target);
 check(std::count(packets.begin(),packets.end(),wire)==1);
 check(f.world.cooldown_completion_stamp(f.source,base_receipt->skill_use)==base_receipt);
 check(!f.last_result.failure_feedback->consume(f.source,f.target,976,0,f.last_permit));
}
int main()try{base_case(false);base_case(true);echo_copy_case();
 std::cout<<"PASS staged inferred976 failure feedback exact World owner/copy/permit/ledger/reset and genuine Echo copy1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
