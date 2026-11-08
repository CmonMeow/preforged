#include "dagger_successful_dual_fixture_draft.h"

// Authoritative stance admission requires the exact typed rank duration.
static bool activate_bash(Fixture& f,unsigned effect,std::uint64_t at){
 const auto plan=effects::shield_bash(0);
 return plan&&f.world.status_mutation(f.target,363,effect,plan->rank,at,at+plan->duration,true);
}
static bool activate_balanced(Fixture& f,unsigned effect,std::uint64_t at){
 const auto plan=effects::balanced_stance(0);
 return plan&&f.world.status_mutation(f.source,371,effect,plan->rank,at,at+plan->duration,true);
}

static void baseline_cases(){
 for(unsigned id:{775u,776u,777u})for(bool protected_from_kd:{false,true}){
  Fixture f(id),baseline(id);f.arm();baseline.arm();check(f.start()==skills::casting::Reason::accepted);check(baseline.start()==skills::casting::Reason::accepted);
  const auto first=f.last_permit->release_time();f.update(first);baseline.update(first);
  check(f.last_result.accepted&&f.last_result.successful_second);
  const auto phase=f.last_result.successful_second;
  const auto original=f.world.cooldown_completion_stamp(f.source,f.last_use_event);check(bool(original));
  const auto first_snapshot=f.cast.cooldown_source_snapshot(f.source,first,f.hooks);check(bool(first_snapshot));
  check(f.world.publish_local_cooldowns(f.source,*first_snapshot));
  check(f.world.shield_equipment(f.target,effects::ShieldEquipment{1,55},12,first+1));
  check(activate_bash(f,910,first+1));
  if(protected_from_kd)check(activate_balanced(f,911,first+1));
  // Independent World tick settles existing conditions without issuing the
  // retained second physical event (776 first contact applies Bleeding).
  baseline.world.tick(phase->due());const auto expected_health=baseline.health();f.update(phase->due());
  check(f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::block);
  check(f.health()==expected_health&&f.world.knocked_down(f.source,phase->due())==!protected_from_kd);
  check(f.payments==1&&f.paid==skills::find_skill(id)->energy_cost()&&f.refunds==0&&f.completions==1&&f.copied_observations==1);
  check(f.world.cooldown_completion_stamp(f.source,f.last_use_event)==original&&f.copied_completion==original);
  const auto event=f.second_outcomes[0].event;
  const auto amendment=f.world.cooldown_amendment_source(f.source,event);check(bool(amendment));
  check(amendment->slot==original->slot&&amendment->skill_use==original->skill_use&&amendment->completion_stamp==original->stamp);
  check(amendment->stamp.order>original->stamp.order&&amendment->stamp.at==phase->due());
  check(amendment->until==std::max(phase->due(),amendment->before_until)+15000);
  const auto after=f.cast.cooldown_source_snapshot(f.source,phase->due(),f.hooks);check(bool(after));
  check(after->amendment_order==amendment->stamp.order&&f.world.publish_local_cooldowns(f.source,*after));
  const auto timer=std::find_if(after->timers.begin(),after->timers.end(),[&](const auto& t){return t.slot==original->slot;});
  check(timer!=after->timers.end()&&timer->until==amendment->until&&timer->origin);
  check(timer->origin->kind==effects::CooldownOriginKind::amendment&&timer->origin->token==event);
  auto stale=*after;stale.amendment_order=0;check(!f.world.publish_local_cooldowns(f.source,stale));
  auto forged=*after;for(auto& t:forged.timers)if(t.slot==original->slot)++t.until;
  check(!f.world.publish_local_cooldowns(f.source,forged));check(f.world.publish_local_cooldowns(f.source,*after));
  auto foreign=f.source;++foreign.generation;check(!f.world.cooldown_amendment_source(foreign,event));
  f.update(phase->due());check(f.world.cooldown_amendment_source(f.source,event)==amendment);
  check(f.payments==1&&f.completions==1&&f.copied_observations==1);
 }
 std::cout<<"PASS actual World second Shield Bash owner amendment, immutable first completion, replay and verified snapshot provenance\n";
}

static void reset_cases(){
 Fixture f(775);f.arm();f.policy.secondary=1;f.policy.learned[389]=1;f.bar[1]={389,0};
 check(f.world.configure_adrenaline(f.source,f.bar,f.clock));
 check(f.start()==skills::casting::Reason::accepted);
 const auto first=f.last_permit->release_time();f.update(first);
 const auto phase=f.last_result.successful_second;check(bool(phase));
 const auto original=f.world.cooldown_completion_stamp(f.source,f.last_use_event);check(bool(original));
 check(effects::completion_deadline(*original)>first);
 f.hooks.commit_flourish_reset=[&](const auto& proposal,const auto& permit,auto at){return f.world.commit_flourish_reset(proposal,permit,at);};
 const auto at=first+1;
 const auto reset=f.world.prepare_flourish_reset(f.source,at);check(bool(reset));
 check(f.cast.reset_flourish(*effects::flourish(0),*reset,at,1,f.hooks));
 auto reset_snapshot=f.cast.cooldown_source_snapshot(f.source,at,f.hooks);check(bool(reset_snapshot));
 check(reset_snapshot->reset_order==reset->stamp.order&&f.world.publish_local_cooldowns(f.source,*reset_snapshot));
 check(std::none_of(reset_snapshot->timers.begin(),reset_snapshot->timers.end(),[](const auto& timer){return timer.slot==skills::Slot{775,0};}));
 check(f.world.shield_equipment(f.target,effects::ShieldEquipment{1,55},12,at));
 check(activate_bash(f,910,at));
 f.update(phase->due());check(f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::block);
 const auto amendment=f.world.cooldown_amendment_source(f.source,f.second_outcomes[0].event);check(bool(amendment));
 check(amendment->before_until==0&&amendment->until==phase->due()+15000&&amendment->stamp.order>reset->stamp.order);
 check(f.world.cooldown_completion_stamp(f.source,f.last_use_event)==original&&f.copied_completion==original);
 check(f.payments==1&&f.completions==1&&f.copied_observations==1);
 const auto after=f.cast.cooldown_source_snapshot(f.source,phase->due(),f.hooks);check(bool(after));
 check(f.world.publish_local_cooldowns(f.source,*after));
 std::cout<<"PASS actual World Flourish reset followed by new second-contact Shield Bash fresh15; first completion immutable\n";
}

static void expired_cases(){
 Fixture f(775);f.arm();check(f.start()==skills::casting::Reason::accepted);
 const auto first=f.last_permit->release_time();f.update(first);
 const auto phase=f.last_result.successful_second;check(bool(phase));
 const auto original=f.world.cooldown_completion_stamp(f.source,f.last_use_event);check(bool(original));
 const auto until=effects::completion_deadline(*original);check(bool(until));
 const auto late=std::max(phase->due(),*until+1);check(late<first+10000);
 check(f.world.shield_equipment(f.target,effects::ShieldEquipment{1,55},12,first+1));
 check(activate_bash(f,910,first+1));
 f.update(late);check(f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::block);
 const auto amendment=f.world.cooldown_amendment_source(f.source,f.second_outcomes[0].event);check(bool(amendment));
 check(amendment->before_until==0&&amendment->until==late+15000&&amendment->stamp.at==late);
 check(f.world.cooldown_completion_stamp(f.source,f.last_use_event)==original&&f.copied_completion==original);
 check(f.payments==1&&f.completions==1&&f.copied_observations==1);
 const auto after=f.cast.cooldown_source_snapshot(f.source,late,f.hooks);check(bool(after));
 check(f.world.publish_local_cooldowns(f.source,*after));
 std::cout<<"PASS expired original base plus later actual Shield Bash yields owner fresh15, immutable first receipt\n";
}

static void independent_cases(){
 Fixture f(775);f.arm();check(f.world.diversion(f.target,f.source,*effects::diversion(12),f.clock));
 check(f.start()==skills::casting::Reason::accepted);
 const auto first=f.last_permit->release_time();f.update(first);
 const auto phase=f.last_result.successful_second;check(bool(phase));
 const auto original=f.world.cooldown_completion_stamp(f.source,f.last_use_event);check(bool(original));
 const auto base=effects::completion_deadline(*original);check(bool(base));
 const auto independent=f.world.skill_disabled_slot(f.source,{775,0},first);check(independent&&*independent>*base+15000);
 check(f.world.shield_equipment(f.target,effects::ShieldEquipment{1,55},12,first+1));
 check(activate_bash(f,910,first+1));
 f.update(phase->due());check(f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::block);
 const auto amendment=f.world.cooldown_amendment_source(f.source,f.second_outcomes[0].event);check(bool(amendment));
 check(amendment->before_until==*base&&amendment->until==*base+15000&&amendment->until<*independent);
 check(f.world.skill_disabled_slot(f.source,{775,0},phase->due())==independent);
 check(f.world.cooldown_completion_stamp(f.source,f.last_use_event)==original&&f.copied_completion==original);
 check(f.payments==1&&f.completions==1&&f.copied_observations==1);
 const auto after=f.cast.cooldown_source_snapshot(f.source,phase->due(),f.hooks);check(bool(after));
 check(f.world.publish_local_cooldowns(f.source,*after));
 const auto timer=std::find_if(after->timers.begin(),after->timers.end(),[](const auto& t){return t.slot==skills::Slot{775,0};});
 check(timer!=after->timers.end()&&timer->until==amendment->until);
 std::cout<<"PASS actual Diversion disable remains separate from later second Shield Bash local-base amendment\n";
}

static void interposed_cases(){
 Fixture f(775);f.arm();check(f.start()==skills::casting::Reason::accepted);
 const auto first=f.last_permit->release_time();f.update(first);
 const auto original=f.world.cooldown_completion_stamp(f.source,f.last_use_event);check(bool(original));
 const auto phase=f.last_result.successful_second;check(bool(phase));
 check(activate_balanced(f,911,first+1));
 check(f.world.shield_equipment(f.target,effects::ShieldEquipment{1,55},12,first+1));
 check(activate_bash(f,910,first+1));
 constexpr std::uint64_t other_use=999;
 check(other_use!=original->skill_use&&f.world.prepare_diversion_skill_use(f.source,{775,0},other_use,phase->due()));
 f.update(phase->due());check(f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::block);
 const auto amendment=f.world.cooldown_amendment_source(f.source,f.second_outcomes[0].event);check(bool(amendment));
 check(amendment->skill_use==original->skill_use&&amendment->completion_stamp==original->stamp);
 check(f.world.finalize_diversion_skill_use(f.source,other_use,phase->due(),0,0).has_value());
 const auto other=f.world.cooldown_completion_stamp(f.source,other_use);check(other&&other->extra_ms==0);
 check(f.world.cooldown_completion_stamp(f.source,original->skill_use)==original);
 check(f.payments==1&&f.completions==1&&f.copied_observations==1);
 std::cout<<"PASS later same-slot owner use cannot capture earlier retained Shield Bash amendment\n";
}

static void retirement_cases(){
 Fixture f(775);check(f.world.energy_capacity(1,7,0,f.clock,{10,0}));std::uint64_t last_order=0;
 for(unsigned cycle=0;cycle<70;++cycle){
  const std::uint64_t began=1000+std::uint64_t(cycle)*25000;
  f.world.tick(began);f.update(began);f.world.set_vitals(2,{1000,1000,30,30},100);f.arm();
  check(f.start()==skills::casting::Reason::accepted);
  const auto first=f.last_permit->release_time();f.update(first);
  const auto original=f.world.cooldown_completion_stamp(f.source,f.last_use_event);check(bool(original));
  const auto phase=f.last_result.successful_second;check(bool(phase));
  check(f.world.shield_equipment(f.target,effects::ShieldEquipment{cycle+1,55},12,first+1));
  check(activate_bash(f,910+cycle,first+1));
  f.update(phase->due());check(f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::block);
  const auto receipt=f.world.cooldown_amendment_source(f.source,f.second_outcomes[0].event);check(bool(receipt));
  check(receipt->stamp.order>last_order);last_order=receipt->stamp.order;
  const auto snapshot=f.cast.cooldown_source_snapshot(f.source,phase->due(),f.hooks);check(bool(snapshot));
  check(snapshot->amendment_order==last_order&&f.world.publish_local_cooldowns(f.source,*snapshot));
  check(f.world.cooldown_completion_stamp(f.source,f.last_use_event)==original);
  check(f.payments==cycle+1&&f.completions==cycle+1&&f.copied_observations==cycle+1);
 }
 std::cout<<"PASS70 actual paid Shield Bash continuations after acknowledged snapshot retirement, monotone amendment watermark\n";
}

static void authority_retry_cases(){
 // Invalid snapshot refuses BEFORE reservation; opaque source basis refuses
 // AFTER reservation but BEFORE actual release. Neither can fake phase47.
 for(unsigned mode=0;mode<3;++mode){Fixture f(775);f.arm();
  if(mode==1){f.policy.secondary=1;f.policy.learned[389]=1;f.bar[1]={389,0};check(f.world.configure_adrenaline(f.source,f.bar,f.clock));}
  check(f.world.empathy(f.target,f.source,26,0,20,1000));
  check(f.world.spiteful_spirit(f.target,f.source,*effects::spiteful_spirit(0),20,1000));
  check(f.world.complete_vigorous_spirit(f.source,f.source,*effects::vigorous_spirit(0),0,1000));
  f.world.set_vitals(1,{500,1000,30,30},100);check(f.start()==skills::casting::Reason::accepted);
  f.update(f.last_permit->release_time());const auto phase=f.last_result.successful_second;check(bool(phase));
  check(f.world.resources(1)->health==490);const auto hp=f.health();const auto original=f.copied_completion;
  check(f.world.shield_equipment(f.target,effects::ShieldEquipment{1,55},12,phase->due()-1));
  check(activate_bash(f,910,phase->due()-1));
  const auto actual=f.hooks.complete_dagger_successful_second;
  if(mode==2)f.hooks.complete_dagger_successful_second={};
  else f.hooks.complete_dagger_successful_second=[&](const auto& cap,const auto& before,auto at){
   auto unavailable=before;if(!mode)unavailable.completion_order=0;
   else for(auto& timer:unavailable.timers)if(timer.slot==skills::Slot{775,0})timer.origin.reset();
   auto result=f.world.complete_dagger_successful_second_draft(cap,unavailable,at);f.second_outcomes=result.outcomes;return result;
  };
  f.world.take_attack_outcomes(f.source);f.box->drain();f.update(phase->due());
  check(f.cast.active()&&phase->open()&&f.health()==hp&&f.world.resources(1)->health==490&&f.payments==1&&f.copied_observations==1);
  const auto refused=f.world.take_attack_outcomes(f.source);check(refused.size()==(mode==1?1u:0u));
  for(const auto& event:refused)check(event.outcome==combat::weapon::AttackOutcome::cancelled&&!event.skill_failed);
  const auto withheld=f.box->drain();check(std::find(withheld.begin(),withheld.end(),skills::casting::terminal(47,1))==withheld.end());
  f.hooks.complete_dagger_successful_second=actual;
  if(mode==1){
   f.hooks.commit_flourish_reset=[&](const auto& plan,const auto& permit,auto at){return f.world.commit_flourish_reset(plan,permit,at);};
   const auto permit=f.world.prepare_flourish_reset(f.source,phase->due()+1);check(bool(permit));
   check(f.cast.reset_flourish(*effects::flourish(0),*permit,phase->due()+1,1,f.hooks));
  }
  f.second_outcomes.clear();f.update(phase->due()+2);
  check(!f.cast.active()&&!phase->open()&&f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::block);
  check(f.world.resources(1)->health==485&&f.payments==1&&f.completions==1&&f.copied_observations==1&&f.copied_completion==original);
  if(mode==1)check(f.second_outcomes[0].event>refused[0].event); // canceled reservation never bound the continuation
  const auto receipt=f.world.cooldown_amendment_source(f.source,f.second_outcomes[0].event);check(bool(receipt));
  if(mode==1)check(receipt->before_until==0);
 }
}
static void postphysical_import_cases(){
 for(bool retire:{false,true}){Fixture f(775);f.arm();check(f.start()==skills::casting::Reason::accepted);
  f.update(f.last_permit->release_time());const auto phase=f.last_result.successful_second;check(bool(phase));
  const auto original=f.copied_completion;
  check(f.world.shield_equipment(f.target,effects::ShieldEquipment{1,55},12,phase->due()-1));
  check(activate_bash(f,910,phase->due()-1));
  check(activate_balanced(f,911,phase->due()-1));
  const auto reader=f.hooks.cooldown_amendment_source;f.hooks.cooldown_amendment_source={};
  f.world.take_attack_outcomes(f.source);f.box->drain();f.update(phase->due());
  check(!phase->open()&&f.cast.active()&&f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::block);
  const auto receipt=f.world.cooldown_amendment_source(f.source,f.second_outcomes[0].event);check(bool(receipt));
  check(!f.cast.cooldown_source_snapshot(f.source,phase->due(),f.hooks));
  check(f.world.take_attack_outcomes(f.source).size()==1);
  const auto first_packets=f.box->drain();check(std::count(first_packets.begin(),first_packets.end(),skills::casting::terminal(47,1))==1);
  f.update(phase->due()+1);check(f.cast.active()&&f.world.take_attack_outcomes(f.source).empty()&&f.box->drain().empty());
  if(retire){
   f.world.leave(1);f.world.join(1,"Replacement",0,0,f.box,0,{}, {},phase->due()+2,1);f.world.set_vitals(1,{1000,1000,30,30},100);
   const auto resources=*f.world.resources(1);f.box->drain();f.update(phase->due()+3);
   check(!f.cast.active()&&f.box->drain().empty()&&f.world.resources(1)->health==resources.health&&f.world.resources(1)->energy==resources.energy);
   continue;
  }
  f.hooks.cooldown_amendment_source=reader;f.hooks.cooldown_completion_source={}; // old completion history need not remain queryable
  f.update(phase->due()+2);check(!f.cast.active()&&f.world.take_attack_outcomes(f.source).empty());
  check(f.payments==1&&f.completions==1&&f.copied_observations==1&&f.copied_completion==original);
  const auto snapshot=f.cast.cooldown_source_snapshot(f.source,phase->due()+2,f.hooks);check(snapshot&&snapshot->amendment_order==receipt->stamp.order);
  check(f.world.publish_local_cooldowns(f.source,*snapshot));f.box->drain();
  check(f.cast.consume_cooldown_amendment(*receipt,phase->due()+3,f.hooks)&&f.box->drain().empty());
  auto forged=*receipt;++forged.until;check(!f.cast.consume_cooldown_amendment(forged,phase->due()+3,f.hooks));
  forged=*receipt;++forged.slot.copy;check(!f.cast.consume_cooldown_amendment(forged,phase->due()+3,f.hooks));
  forged=*receipt;++forged.stamp.owner.generation;check(!f.cast.consume_cooldown_amendment(forged,phase->due()+3,f.hooks));
  const auto events=f.hooks.events;f.hooks.events={};check(!f.cast.consume_cooldown_amendment(*receipt,phase->due()+3,f.hooks));f.hooks.events=events;
  check(f.box->drain().empty());
  f.update(receipt->until+1);f.box->drain();const auto expired=f.cast.cooldown_source_snapshot(f.source,receipt->until+1,f.hooks);check(bool(expired));
  check(f.cast.consume_cooldown_amendment(*receipt,receipt->until+1,f.hooks)&&f.box->drain().empty());
  check(f.cast.cooldown_source_snapshot(f.source,receipt->until+1,f.hooks)==expired);
 }
}
static void actual_copy_amendment_case(){
 Fixture f(775);f.arm();f.policy.secondary=5;f.policy.learned[74]=1;f.bar[1]={74,0};check(f.world.configure_adrenaline(f.source,f.bar,f.clock));
 effects::TemporarySkillSession session(f.source,f.bar);effects::TemporarySkillIds ids(f.source);const auto echo=*effects::echo(74);
 check(f.world.status_mutation(f.source,74,100,0,f.clock,f.clock+echo.armed_duration,true,skills::Slot{74,0}));
 check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());const auto base_receipt=f.copied_completion;
 const auto first=f.last_permit->release_time();const auto original_phase=f.last_result.successful_second;check(bool(original_phase));
 const auto copied=effects::commit_echo_copy(f.world,session,ids,f.source,{74,0},{775,0},first,f.last_use_event);check(copied&&copied->changed&&copied->changed->after==skills::Slot{775,1});
 f.bar=session.bar();f.update(original_phase->due());check(!f.cast.active());f.active_copy=1;f.second_outcomes.clear();
 f.hooks.admit_session_cast=[&](const auto& bar,const auto& policy,auto request,auto at)->std::optional<effects::HexCopySession::CastAdmission>{
  if(bar!=session.bar())return {};const auto slot=std::find(bar.begin(),bar.end(),skills::Slot{request.skill,request.copy});
  if(slot==bar.end())return {};return session.admit(f.source,policy,unsigned(slot-bar.begin()),*slot,at);
 };
 f.hooks.valid_session_cast=[&](const auto& grant,auto at){return session.valid(grant,f.policy,at);};
 f.clock=first+1001;f.arm();check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
 const auto original=f.copied_completion;check(original&&original->slot==skills::Slot{775,1});const auto phase=f.last_result.successful_second;check(bool(phase));
 check(f.world.shield_equipment(f.target,effects::ShieldEquipment{1,55},12,phase->due()-1));
 check(activate_bash(f,910,phase->due()-1));
 f.update(phase->due());check(f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::block);
 const auto receipt=f.world.cooldown_amendment_source(f.source,f.second_outcomes[0].event);check(receipt&&receipt->slot==skills::Slot{775,1}&&receipt->skill_use==original->skill_use&&receipt->completion_stamp==original->stamp);
 check(f.world.cooldown_completion_stamp(f.source,base_receipt->skill_use)==base_receipt&&f.copied_completion==original);
 check(f.payments==2&&f.paid==10&&f.completions==2&&f.copied_observations==2);
 const auto snapshot=f.cast.cooldown_source_snapshot(f.source,phase->due(),f.hooks);check(snapshot&&f.world.publish_local_cooldowns(f.source,*snapshot));
}
int main()try{baseline_cases();reset_cases();expired_cases();independent_cases();interposed_cases();retirement_cases();authority_retry_cases();postphysical_import_cases();actual_copy_amendment_case();std::cout<<"PASS shared actual World contact amendment suite\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
