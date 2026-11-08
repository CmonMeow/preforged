#include "dagger_successful_dual_fixture_draft.h"
int main()try{
 for(unsigned skill:{775u,776u,777u})for(bool npc:{false,true}){
  Fixture f(skill,npc);f.arm();const auto hp=f.health();check(f.start()==skills::casting::Reason::accepted);
  const auto first=f.last_permit->release_time();f.update(first);
  check(f.last_result.accepted&&f.last_result.outcomes.size()==1&&f.last_result.successful_second);
  const auto phase=f.last_result.successful_second;check(phase->due()==first+(first-1000));
  const auto after_first=f.health();check(after_first<hp&&f.cast.active()&&f.payments==1);
  const auto first_event=f.last_result.outcomes[0].event;
  const auto first_packets=f.box->drain();check(std::count(first_packets.begin(),first_packets.end(),skills::casting::terminal(46,1))==1);
  check(std::count(first_packets.begin(),first_packets.end(),skills::cancellation_packet(1,{static_cast<std::uint16_t>(skill),0}))==0);
  f.update(phase->due()-1);check(f.health()==after_first&&f.second_outcomes.empty()&&f.box->drain().empty());
  f.update(phase->due());check(!f.cast.active()&&!phase->open()&&f.health()<after_first&&f.second_outcomes.size()==1);
  check(f.second_outcomes[0].event>first_event&&f.second_outcomes[0].released>=phase->due());
  check(f.paid==skills::find_skill(skill)->energy_cost()&&f.payments==1&&f.refunds==0&&f.completions==1&&f.copied_observations==1);
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),skills::casting::terminal(47,1))==1);
  check(std::count(packets.begin(),packets.end(),skills::cancellation_packet(1,{static_cast<std::uint16_t>(skill),0}))==1);
  const auto hp_after=f.health();auto replay=f.world.complete_dagger_successful_second_draft(phase,*f.cast.cooldown_source_snapshot(f.source,phase->due(),f.hooks),phase->due());
  check(!replay.accepted&&replay.outcomes.empty());f.update(phase->due());check(f.health()==hp_after&&f.payments==1);
 }
 {Fixture f(775);f.arm();check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
  const auto phase=f.last_result.successful_second;check(bool(phase));const auto hp=f.health();
  world::Instance other;check(!other.complete_dagger_successful_second_draft(phase,*f.cast.cooldown_source_snapshot(f.source,phase->due(),f.hooks),phase->due()).accepted&&phase->open());
  const auto scope=f.hooks.target_scope;f.hooks.target_scope={};f.box->drain();f.update(phase->due());f.cast.cancel(f.hooks,phase->due());
  check(f.cast.active()&&phase->open()&&f.health()==hp&&f.box->drain().empty());
  f.hooks.target_scope=scope;f.update(phase->due());check(!f.cast.active()&&!phase->open()&&f.health()<hp&&f.payments==1);}
 // A fatal first actual contact creates no synthetic second phase/event.
 for(bool source_dies:{false,true}){Fixture f(775);f.arm();
  if(source_dies){check(f.world.empathy(f.target,f.source,26,0,20,1000));f.world.set_vitals(1,{5,1000,30,30},100);}
  else f.world.set_vitals(2,{1,1000,30,30},100);
  check(f.start()==skills::casting::Reason::accepted);const auto first=f.last_permit->release_time();f.update(first);
  check(f.last_result.accepted&&!f.cast.active()&&!f.last_result.successful_second&&f.last_result.outcomes.size()==1&&f.payments==1);
  const auto hp=f.health();f.box->drain();f.update(first+500);check(f.health()==hp&&f.second_outcomes.empty()&&f.payments==1);
  const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),skills::casting::terminal(47,1))==packets.end());}
 // Every late phase failure cancels only the retained physical release. The
 // first contact, payment and single World-owned cooldown receipt remain real.
 for(unsigned mode=0;mode<7;++mode){Fixture f(775);f.arm();check(f.start()==skills::casting::Reason::accepted);
  const auto first=f.last_permit->release_time();f.update(first);auto phase=f.last_result.successful_second;check(bool(phase));
  const auto before=f.health();const auto at=first+1;
  if(mode==0){f.world.leave(1);f.world.join(1,"NewSource",0,0,f.box,0,{}, {},at,1);f.world.set_vitals(1,{1000,1000,30,30},100);}
  if(mode==1){f.world.leave(2);f.world.join(2,"NewTarget",60,0,std::make_shared<world::Outbox>(),0,{}, {},at,2);f.world.set_vitals(2,{1000,1000,30,30},100);}
  if(mode==2){auto gear=f.weapon;++gear.revision;check(f.world.ordinary_weapon(f.source,gear,20,at,combat::weapon::CriticalProfile{7,12,20}));}
  if(mode==3)check(f.world.gale(f.target,f.source,0,.5,at));
  if(mode==4)f.cast.cancel(f.hooks,at);
  if(mode==5)f.world.set_vitals(1,{0,1000,30,30},100);
  if(mode==6)f.world.set_vitals(2,{0,1000,30,30},100);
  f.box->drain();f.update(phase->due());check(!f.cast.active()&&!phase->open()&&f.second_outcomes.empty()&&f.payments==1&&f.completions==1);
  check(f.health()==(mode==1?1000:mode==6?0:before));
  const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),skills::casting::terminal(47,1))==packets.end());
  if(mode==0)check(packets.empty());
 }
 // Real timed Meteor interrupts between fronts. Its KD has ended at the
 // coarse callback, so post-settlement epoch validation must reject phase2.
 {Fixture f(775);f.arm();check(f.world.elemental_armor(f.source,60,{}));
  f.world.join(3,"Meteor caster",600,0,std::make_shared<world::Outbox>(),0,{}, {},1000,2);
  f.world.set_vitals(3,{1000,1000,30,30},100);const auto caster=*f.world.effect_scope(3);
  check(f.start()==skills::casting::Reason::accepted);
  check(f.world.fire_storm(caster,f.source,*effects::fire_storm(0,187),20,1300));
  f.update(f.last_permit->release_time());const auto phase=f.last_result.successful_second;check(bool(phase));
  const auto before=f.health();const auto epoch=f.world.control_epoch(f.source);f.box->drain();f.update(5000);
  check(!f.cast.active()&&!phase->open()&&f.second_outcomes.empty()&&f.health()==before&&f.payments==1);
  check(f.world.control_epoch(f.source)!=epoch);const auto packets=f.box->drain();
  check(std::find(packets.begin(),packets.end(),skills::casting::terminal(47,1))==packets.end());}
 // A canonical reserved/cancelled event is NOT an actual second attack.
 {Fixture f(775,false,true);f.arm();check(f.world.complete_vigorous_spirit(f.source,f.source,*effects::vigorous_spirit(0),0,1000));
  f.world.set_vitals(1,{500,1000,30,30},100);check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
  const auto phase=f.last_result.successful_second;check(bool(phase));const auto source_hp=f.world.resources(1)->health;
  const auto at=f.last_permit->release_time()+1;
  f.world.join(4,"Seed monk",-60,0,std::make_shared<world::Outbox>(),0,{}, {},at,1);f.world.set_vitals(4,{500,1000,30,30},100);
  check(f.world.party_action(1,158,4,at));check(f.world.party_action(4,154,1,at));
  check(f.world.complete_healing_seed(*f.world.effect_scope(4),f.source,*effects::healing_seed(12),0,at));
  check(f.world.empathy(f.target,f.source,26,0,20,at));const auto hp=f.health();f.box->drain();f.update(phase->due());
  check(!f.cast.active()&&!phase->open()&&f.health()==hp&&f.world.resources(1)->health==source_hp&&f.payments==1);
  check(f.second_outcomes.size()==1&&f.second_outcomes[0].outcome==combat::weapon::AttackOutcome::cancelled);
  const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),skills::casting::terminal(47,1))==packets.end());}
 // The same first-contact acceptance must reject an unmade reservation.
 {Fixture f(775,false,true);f.arm();check(f.world.complete_vigorous_spirit(f.source,f.source,*effects::vigorous_spirit(0),0,1000));
  f.world.set_vitals(1,{500,1000,30,30},100);check(f.start()==skills::casting::Reason::accepted);const auto at=1001ull;
  f.world.join(4,"Seed monk",-60,0,std::make_shared<world::Outbox>(),0,{}, {},at,1);f.world.set_vitals(4,{500,1000,30,30},100);
  check(f.world.party_action(1,158,4,at));check(f.world.party_action(4,154,1,at));
  check(f.world.complete_healing_seed(*f.world.effect_scope(4),f.source,*effects::healing_seed(12),0,at));
  check(f.world.empathy(f.target,f.source,26,0,20,at));const auto target_hp=f.health();f.box->drain();f.update(f.last_permit->release_time());
  check(!f.last_result.accepted&&!f.last_result.successful_second&&!f.cast.active());
  check(f.last_result.outcomes.size()==1&&f.last_result.outcomes[0].outcome==combat::weapon::AttackOutcome::cancelled);
  check(f.world.resources(1)->health==500&&f.health()==target_hp&&f.payments==1&&f.paid==5&&f.copied_observations==0);
  const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),skills::casting::terminal(46,1))==packets.end());
  check(std::find(packets.begin(),packets.end(),skills::casting::terminal(47,1))==packets.end());}
 // Actual World Echo commit yields a TemporarySkillSession copy1. Its grant
 // remains authority across BOTH physical phases, including expiry/base edits.
 for(unsigned mode=0;mode<3;++mode){Fixture f(775);f.arm();f.policy.secondary=5;f.policy.learned[74]=1;f.bar[1]={74,0};
  check(f.world.configure_adrenaline(f.source,f.bar,1000));const auto base=f.bar;
  effects::TemporarySkillSession session(f.source,f.bar);effects::TemporarySkillIds ids(f.source);const auto echo=*effects::echo(74);
  check(f.world.status_mutation(f.source,74,100,0,1000,1000+echo.armed_duration,true,skills::Slot{74,0}));
  check(f.start()==skills::casting::Reason::accepted);const auto first=f.last_permit->release_time();f.update(first);
  const auto original=f.last_result.successful_second;check(bool(original));
  const auto copied=effects::commit_echo_copy(f.world,session,ids,f.source,{74,0},{775,0},first,f.last_use_event);
  check(copied&&copied->changed&&copied->changed->after==skills::Slot{775,1});f.bar=session.bar();
  f.update(original->due());check(!f.cast.active());f.second_outcomes.clear();f.active_copy=1;
  f.hooks.admit_session_cast=[&](const auto& bar,const auto& policy,auto request,auto at)->std::optional<effects::HexCopySession::CastAdmission>{
   if(bar!=session.bar())return {};const auto slot=std::find(bar.begin(),bar.end(),skills::Slot{request.skill,request.copy});
   if(slot==bar.end())return {};return session.admit(f.source,policy,unsigned(slot-bar.begin()),*slot,at);
  };
  f.hooks.valid_session_cast=[&](const auto& admission,auto at){return session.valid(admission,f.policy,at);};
  f.clock=mode==2?first+echo.copy_duration-750:first+1001;f.arm();check(f.start()==skills::casting::Reason::accepted);
  const auto copied_first=f.last_permit->release_time();f.update(copied_first);const auto phase=f.last_result.successful_second;check(phase&&f.cast.active());
  check(f.copied_completion&&f.copied_completion->slot==skills::Slot{775,1});const auto hp=f.health();
  if(mode==1){check(bool(effects::replace_hex_copy_base(f.world,session,f.source,base,f.policy,copied_first+1)));f.bar=session.bar();}
  if(mode==2){const auto restored=effects::expire_hex_copy(f.world,session,f.source,phase->due());check(restored&&restored->size()==1);f.bar=session.bar();}
  f.box->drain();f.update(phase->due());check(!f.cast.active()&&!phase->open()&&f.payments==2&&f.paid==10&&f.refunds==0);
  check((f.health()<hp)==(mode==0));check(f.second_outcomes.size()==(mode==0?1u:0u));
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),skills::casting::terminal(47,1))==(mode==0?1:0));}
 // One logical Spiteful trigger, but Vigorous is per real physical attack.
 for(unsigned outcome=0;outcome<4;++outcome){Fixture f(775);f.arm();
  check(f.world.spiteful_spirit(f.target,f.source,*effects::spiteful_spirit(0),20,1000));
  check(f.world.complete_vigorous_spirit(f.source,f.source,*effects::vigorous_spirit(0),0,1000));
  f.world.set_vitals(1,{500,1000,30,30},100);
  if(outcome==1){check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,1000,10000));f.world.configure_attack_miss_draws([]{return 0.;});}
  if(outcome==2){check(f.world.status_mutation(f.target,259,50,12,1000,10000,true));f.world.configure_attack_block_draws([]{return 0.;});}
  if(outcome==3){skills::Slots iwbar{};iwbar[0]={33,0};check(f.world.configure_adrenaline(f.source,iwbar,1000));
    check(f.world.status_mutation(f.source,33,123,12,1000,31000,true));check(f.world.configure_adrenaline(f.source,f.bar,1000));}
  check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
  auto phase=f.last_result.successful_second;check(bool(phase));const auto heal=effects::vigorous_spirit(0)->healing;
  check(f.world.resources(1)->health==500+heal-effects::spiteful_spirit(0)->damage);
  f.update(phase->due());check(f.world.resources(1)->health==500+2*heal-effects::spiteful_spirit(0)->damage);
 }
 {Fixture f(775);f.arm();check(f.world.empathy(f.target,f.source,26,0,20,1000));
  f.world.set_vitals(1,{500,1000,30,30},100);check(f.start()==skills::casting::Reason::accepted);
  f.update(f.last_permit->release_time());auto phase=f.last_result.successful_second;check(bool(phase));
  check(f.world.resources(1)->health==490);f.update(phase->due());check(f.world.resources(1)->health==480&&f.payments==1);}
 // Same IAS quote owns both frontiers; exact rounding is declared policy.
 {Fixture f(775);f.arm();check(f.world.status_mutation(f.source,346,70,0,1000,9000,true));
  const auto quoted=f.world.attack_duration(f.source,500,1000,true);check(quoted&&*quoted>0);
  check(f.start()==skills::casting::Reason::accepted);const auto first=f.last_permit->release_time();check(first==1000+*quoted);
  f.update(first);const auto phase=f.last_result.successful_second;check(phase&&phase->due()==1000+2*(*quoted));
  const auto hp=f.health();f.update(phase->due()-1);check(f.health()==hp);f.update(phase->due());check(f.health()<hp&&f.payments==1);}
 std::cout<<"PASS retained paid dual first/second contacts, lifetime/equipment/control/cancel/death, one fee/observer completion and native47\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
