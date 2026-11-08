#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Cooldown Runtime source line "+std::to_string(at.line()));}
int main()try{
 warrior_test::Fixture f(403,false);auto& w=f.world;Slots bar{};bar[0]={281,0};check(w.configure_adrenaline(f.source,bar,100));
 bool forged=false;unsigned unsequenced=0;casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy_at=[&](unsigned n,auto at){return w.consume_energy(1,n,f.source,at);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](auto id,float,bool){return id==1;};h.heal=[](auto,auto,float,bool){return true;};
 h.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(w.cast_activity(f.source,slot,begins,ends));};h.control_epoch=[&](){return w.control_epoch(f.source);};h.control_event_time=[&](auto after){return w.control_event_time(f.source,after);};
 h.prepare_skill_use=[&](auto slot,auto event,auto at){return w.prepare_diversion_skill_use(f.source,slot,event,at);};h.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool instant){return w.finalize_diversion_skill_use(f.source,event,at,base,existing,instant);};h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(f.source,event);};
 h.cooldown_completion_source=[&](auto event){auto source=w.cooldown_completion_stamp(f.source,event);if(forged&&source)source->slot={403,0};return source;};h.cooldown_control_source=[&](auto after){return w.cooldown_control_source(f.source,after);};h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(f.source,slot,at);};h.current_skillbar=[&]()->std::optional<Slots>{return bar;};
 h.diagnostic=[&](auto,const char* phase){if(std::string_view(phase).starts_with("cooldown_unsequenced_"))++unsequenced;};
 Policy p;p.primary=3;p.campaigns=255;p.learned[281]=1;attributes::Ranks ranks{};casting::Runtime runtime(1);
 check(runtime.cooldown_source_snapshot(f.source,100,h)->timers.empty());
 check(w.publish_local_cooldowns(f.source,*runtime.cooldown_source_snapshot(f.source,100,h))&&w.local_cooldown_snapshot(f.source)->verified.empty());
 auto unprocessed=*runtime.cooldown_source_snapshot(f.source,100,h);
 check(runtime.start(1000,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);runtime.update(2000,h);
 auto snapshot=*runtime.cooldown_source_snapshot(f.source,2000,h);check(snapshot.timers.size()==1);auto timer=snapshot.timers.front();
 check(snapshot.completion_order==2&&!w.local_cooldown_snapshot(f.source));
 unprocessed.at=2000;check(!w.publish_local_cooldowns(f.source,unprocessed)&&!w.local_cooldown_snapshot(f.source));
 check(timer.slot==Slot{281,0}&&timer.until==4000&&timer.origin&&timer.origin->kind==effects::CooldownOriginKind::completion&&timer.origin->stamp.at==2000&&timer.origin->stamp.order==2);
 check(w.publish_local_cooldowns(f.source,snapshot)&&w.local_cooldown_snapshot(f.source)->verified==std::vector<Slot>{{281,0}});
 auto invalid=snapshot;++invalid.timers.front().until;check(!w.publish_local_cooldowns(f.source,invalid)&&!w.local_cooldown_snapshot(f.source));
 check(w.publish_local_cooldowns(f.source,snapshot));invalid=snapshot;--invalid.at;check(!w.publish_local_cooldowns(f.source,invalid)&&!w.local_cooldown_snapshot(f.source));check(w.publish_local_cooldowns(f.source,snapshot));
 const auto revision=snapshot.revision;check(w.publish_local_cooldowns(f.source,*runtime.cooldown_source_snapshot(f.source,4000,h))&&w.local_cooldown_snapshot(f.source)->snapshot.timers.empty());runtime.update(4000,h);check(runtime.cooldown_source_snapshot(f.source,4000,h)->timers.empty()&&runtime.cooldown_source_snapshot(f.source,4000,h)->revision>revision);
 check(runtime.start(5000,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);
 unprocessed=*runtime.cooldown_source_snapshot(f.source,5500,h);
 check(w.interrupt_cast(f.source,281,5500)&&!w.local_cooldown_snapshot(f.source));
 check(!w.publish_local_cooldowns(f.source,unprocessed));runtime.update(5501,h);
 timer=runtime.cooldown_source_snapshot(f.source,5501,h)->timers.front();check(timer.until==7500&&timer.origin&&timer.origin->kind==effects::CooldownOriginKind::control&&timer.origin->token==1&&timer.origin->stamp.at==5500&&timer.origin->stamp.order==3);
 check(w.publish_local_cooldowns(f.source,*runtime.cooldown_source_snapshot(f.source,5501,h))&&w.local_cooldown_snapshot(f.source)->verified.empty()); // Non-attack control duration remains opaque.
 runtime.update(7500,h);check(runtime.start(8000,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);
 check(w.status_mutation(f.source,16,123,12,8100,8100+timing::stance(16,12)->duration,true));check(!w.interrupt_cast(f.source,281,8500));runtime.update(9000,h);
 timer=runtime.cooldown_source_snapshot(f.source,9000,h)->timers.front();check(timer.until==11000&&timer.origin&&timer.origin->kind==effects::CooldownOriginKind::completion&&timer.origin->stamp.order==4);
 runtime.update(11000,h);check(runtime.start(12000,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);runtime.interrupt(12500,h);
 timer=runtime.cooldown_source_snapshot(f.source,12500,h)->timers.front();check(timer.until==14500&&!timer.origin&&unsequenced==1);
 runtime.update(14500,h);forged=true;check(runtime.start(15000,{281,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);runtime.update(16000,h);
 timer=runtime.cooldown_source_snapshot(f.source,16000,h)->timers.front();check(timer.until==18000&&!timer.origin&&unsequenced==2);
 auto stale=f.source;++stale.generation;check(!runtime.cooldown_source_snapshot(stale,16000,h));
 w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);check(!runtime.cooldown_source_snapshot(*w.effect_scope(1),16000,h));
 {warrior_test::Fixture x(403,false);auto& world=x.world;check(world.cast_activity(x.source,400,100,5000));const auto epoch=*world.control_epoch(x.source);check(world.interrupt_cast(x.source,400,300));
 const auto control=*world.cooldown_control_source(x.source,epoch);
 effects::LocalCooldownSnapshot state{x.source,1,300,{{{400,0},6300,effects::CooldownOrigin{*control.stamp,effects::CooldownOriginKind::control,control.epoch}}}};
 state.control_epoch=control.epoch;
 check(world.publish_local_cooldowns(x.source,state)&&world.local_cooldown_snapshot(x.source)->verified==std::vector<Slot>{{400,0}});
 auto bad=state;bad.revision=2;++bad.timers.front().until;check(!world.publish_local_cooldowns(x.source,bad)&&!world.local_cooldown_snapshot(x.source));check(world.publish_local_cooldowns(x.source,state));
 bad=state;bad.revision=2;bad.timers.front().origin->token=9999;check(world.publish_local_cooldowns(x.source,bad)&&world.local_cooldown_snapshot(x.source)->verified.empty());
 auto duplicate=bad;duplicate.revision=3;duplicate.timers.push_back(duplicate.timers.front());check(!world.publish_local_cooldowns(x.source,duplicate)&&!world.local_cooldown_snapshot(x.source));
 }
 for(unsigned mode=0;mode<8;++mode){
  warrior_test::Fixture x(400,false);auto& world=x.world;Slots attacks{};attacks[0]={400,0};check(world.configure_adrenaline(x.source,attacks,100));
  casting::Runtime cast(1);casting::Hooks hooks;std::vector<world::Packet> wire;bool external=false,unavailable=false;
  hooks.events=[&](const auto& batch){wire.insert(wire.end(),batch.begin(),batch.end());};hooks.resources=[&](auto id){return world.resources(id);};
  hooks.consume_energy_at=[&](unsigned n,auto at){return world.consume_energy(1,n,x.source,at);};hooks.target_scope=[&](auto id){return world.effect_scope(id);};
  hooks.weapon_snapshot=[&](){return std::optional{x.weapon};};hooks.melee_target=[&](auto id,auto at){return world.melee_target(x.source,id,at,400);};
  hooks.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){return world.melee_attack(x.source,target,gear,plan,at);};
  hooks.attack_activity=[&](auto until){world.attack_activity(x.source,until,1000);};hooks.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(world.cast_activity(x.source,slot,begins,ends));};
  hooks.control_epoch=[&](){return world.control_epoch(x.source);};hooks.control_event_time=[&](auto after){return world.control_event_time(x.source,after);};hooks.attack_interrupt_epoch=[&](){return world.attack_interrupt_epoch(x.source);};
  hooks.cooldown_control_source=[&](auto after){return mode==1?std::optional<effects::CooldownControl>{}:world.cooldown_control_source(x.source,after);};
  hooks.weapon_attack_timing=[&](auto at){return world.weapon_attack_timing(x.source,at);};hooks.current_skillbar=[&]()->std::optional<Slots>{return attacks;};
  hooks.skill_disabled_slot=[&](auto slot,auto at)->std::optional<std::uint64_t>{if(unavailable)return {};return external?9000:world.skill_disabled_slot(x.source,slot,at);};
  if(mode==5){
   hooks.prepare_skill_use=[&](auto slot,auto event,auto at){return world.prepare_diversion_skill_use(x.source,slot,event,at);};
   hooks.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool instant){return world.finalize_diversion_skill_use(x.source,event,at,base,existing,instant);};
   hooks.abort_skill_use=[&](auto event){return world.abort_diversion_skill_use(x.source,event);};
   hooks.cooldown_completion_source=[&](auto event){return world.cooldown_completion_stamp(x.source,event);};
  }
  Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[400]=1;attributes::Ranks effective{};effective[25]=12;
  check(cast.start(1000,{400,0,2},attacks,policy,effective,true,hooks)==casting::Reason::accepted);
  check(world.configure_adrenaline(x.source,attacks,1200));
  if(mode==5)cast.update(2000,hooks);else{
   check(world.interrupt_cast(x.source,400,1500));
   if(mode==6){
    unavailable=true;cast.update(1600,hooks);
    const auto unhandled=*cast.cooldown_source_snapshot(x.source,1600,hooks);
    check(unhandled.control_epoch==0&&!world.publish_local_cooldowns(x.source,unhandled));unavailable=false;
    // The retained reset reaches the session before it processes an older
    // actual World interrupt. That interrupt must not recreate its recharge.
    combat::weapon::AttackCooldowns empty{x.source,1,0,1600,{{{400,0},0,0}}};
    const combat::weapon::DeterminedOutcome failure{x.source,1000,1600,true};
    const combat::weapon::DeterminedNotice early{{x.source,4,1600},*combat::weapon::prepare_determined_recharge(empty,failure)};
    check(cast.consume_attack_reset(early,1600,hooks)==effects::OrderedAttackCooldowns::Result::accepted);
    wire.clear();cast.update(1700,hooks);
    check(cast.cooldown_source_snapshot(x.source,1700,hooks)->timers.empty());
    check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{400,0},6))==0);
    check(std::count(wire.begin(),wire.end(),casting::slot_event(229,1,{400,0}))==1);
    continue;
   }
   cast.update(1500,hooks);
  }
  const std::uint64_t at=mode==5?2100:1600,until=mode==5?8000:7500;
  const auto before=*cast.cooldown_source_snapshot(x.source,at,hooks);check(before.timers.size()==1&&before.timers.front().until==until);
  if(mode==7){
   check(before.reset_order==0&&world.publish_local_cooldowns(x.source,before));
   const auto permit=world.prepare_cooldown_checkpoint(x.source,before,at);check(bool(permit));
   check(cast.apply_cooldown_checkpoint(*permit,at,hooks)&&cast.cooldown_source_snapshot(x.source,at,hooks)==before);
   check(!world.acknowledge_attack_reset(permit->watermark));
   auto invented=before;invented.reset_order=permit->watermark.order;
   check(!world.publish_local_cooldowns(x.source,invented)&&!world.prepare_cooldown_checkpoint(x.source,before,at));
   check(world.publish_local_cooldowns(x.source,before));continue;
  }
  if(mode==5)check(before.timers.front().origin&&before.timers.front().origin->kind==effects::CooldownOriginKind::completion);
  combat::weapon::AttackCooldowns state{x.source,1,0,mode==3?1200u:at,{{{400,0},mode==3?0u:until,0}}};
  const combat::weapon::DeterminedOutcome failure{x.source,999,state.at,true};
  const combat::weapon::DeterminedNotice notice{{x.source,mode==3?2u:4u,state.at},*combat::weapon::prepare_determined_recharge(state,failure)};
  using R=effects::OrderedAttackCooldowns::Result;wire.clear();
  auto wrong=notice;++wrong.stamp.owner.generation;check(cast.consume_attack_reset(wrong,at,hooks)==R::invalid&&wire.empty());
  if(mode==4){unavailable=true;check(cast.consume_attack_reset(notice,at,hooks)==R::invalid&&wire.empty()&&cast.cooldown_source_snapshot(x.source,at,hooks)==before);unavailable=false;}
  external=mode==2;
  check(cast.consume_attack_reset(notice,at,hooks)==(mode==1?R::invalid:R::accepted));
  const auto after=*cast.cooldown_source_snapshot(x.source,at,hooks);
  if(mode==1){check(after==before&&wire.empty());continue;}
  check(after.timers.empty()==(mode!=3));
  check(std::count(wire.begin(),wire.end(),casting::slot_event(229,1,{400,0}))==unsigned(mode!=2&&mode!=3));
  if(mode==2)check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{400,0},8))==1);
  const auto count=wire.size();unavailable=true;
  check(cast.consume_attack_reset(notice,at+1,hooks)==R::duplicate&&wire.size()==count);
  unavailable=false;
  if(mode==2||mode==3)check(cast.start(at+1,{400,0,2},attacks,policy,effective,true,hooks)==casting::Reason::recharge);
  if(mode==3){
   // A complete checkpoint must retain live sources even while off-bar.
   attacks={};attacks[0]={281,0};check(world.configure_adrenaline(x.source,attacks,1700));
   const auto image=*cast.cooldown_source_snapshot(x.source,1700,hooks);
   // Synthetic reset fixture: reuse real World source verification, but supply
   // the isolated Runtime test's reset prefix (no World reset producer yet).
   auto proof=image;proof.reset_order=0;check(world.publish_local_cooldowns(x.source,proof));
   auto accepted=*world.local_cooldown_snapshot(x.source);accepted.snapshot=image;
   const effects::CooldownCheckpointPermit permit{accepted,world.cooldown_bar_snapshot(x.source)->published};
   auto tampered=permit;++tampered.accepted.snapshot.revision;
   check(!cast.apply_cooldown_checkpoint(tampered,1700,hooks)&&cast.cooldown_source_snapshot(x.source,1700,hooks)==image);
   check(cast.apply_cooldown_checkpoint(permit,1700,hooks)&&cast.apply_cooldown_checkpoint(permit,1700,hooks));
   check(cast.consume_attack_reset(notice,1700,hooks)==R::stale);
   attacks[0]={400,0};check(world.configure_adrenaline(x.source,attacks,1800));
   combat::weapon::AttackCooldowns eligible{x.source,1,0,1800,{{{400,0},7500,0}}};
   const combat::weapon::DeterminedOutcome next{x.source,1001,1800,true};
   const combat::weapon::DeterminedNotice reset{world.cooldown_bar_snapshot(x.source)->published,*combat::weapon::prepare_determined_recharge(eligible,next)};
   check(cast.consume_attack_reset(reset,1800,hooks)==R::accepted&&cast.cooldown_source_snapshot(x.source,1800,hooks)->timers.empty());
  }
  if(mode==0){
   check(world.configure_adrenaline(x.source,attacks,1700)&&world.configure_adrenaline(x.source,attacks,1700));
   const auto image=*cast.cooldown_source_snapshot(x.source,1700,hooks);auto proof=image;proof.reset_order=0;
   check(world.publish_local_cooldowns(x.source,proof));auto accepted=*world.local_cooldown_snapshot(x.source);accepted.snapshot=image;
   const effects::CooldownCheckpointPermit permit{accepted,world.cooldown_bar_snapshot(x.source)->published};
   cast.morale_boost(hooks);const auto changed=*cast.cooldown_source_snapshot(x.source,1700,hooks);
   check(!cast.apply_cooldown_checkpoint(permit,1700,hooks)&&cast.cooldown_source_snapshot(x.source,1700,hooks)==changed);
  }
 }
 // Full production-shaped path: Runtime release -> World ledger miss ->
 // retained notice -> local base reset -> acknowledgment -> real checkpoint.
 {warrior_test::Fixture x(402,false);auto& world=x.world;Slots attacks{};attacks[0]={402,0};
  check(world.configure_adrenaline(x.source,attacks,100));
  check(world.apply_condition(x.target,x.source,combat::conditions::Kind::blind,100,5000));
  casting::Runtime cast(1);casting::Hooks hooks;std::vector<world::Packet> wire;
  hooks.events=[&](const auto& batch){wire.insert(wire.end(),batch.begin(),batch.end());};hooks.resources=[&](auto id){return world.resources(id);};
  hooks.consume_energy_at=[&](unsigned n,auto at){check(n==5);return world.consume_energy(1,n,x.source,at);};hooks.target_scope=[&](auto id){return world.effect_scope(id);};
  hooks.weapon_snapshot=[&](){return std::optional{x.weapon};};hooks.melee_target=[&](auto id,auto at){return world.melee_target(x.source,id,at,402);};
  hooks.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){return world.melee_attack(x.source,target,gear,plan,at);};
  hooks.completed_bow_attack=[&](auto target,const auto& gear,const auto& plan,auto event,auto at,auto base,auto existing,auto& visuals){
   const auto result=world.completed_bow_attack(x.source,target,gear,plan,event,at,base,existing,&visuals);
   // A direct future resource event and tick interleave before local import.
   if(result){check(world.settle_resources(x.source,at+100));world.tick(at+100);}
   return result;
  };
  hooks.attack_activity=[&](auto until){world.attack_activity(x.source,until,1000);};hooks.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(world.cast_activity(x.source,slot,begins,ends));};
  hooks.control_epoch=[&](){return world.control_epoch(x.source);};hooks.control_event_time=[&](auto after){return world.control_event_time(x.source,after);};hooks.attack_interrupt_epoch=[&](){return world.attack_interrupt_epoch(x.source);};
  hooks.cooldown_control_source=[&](auto after){return world.cooldown_control_source(x.source,after);};hooks.weapon_attack_timing=[&](auto at){return world.weapon_attack_timing(x.source,at);};
  hooks.current_skillbar=[&]()->std::optional<Slots>{return attacks;};hooks.skill_disabled_slot=[&](auto slot,auto at){return world.skill_disabled_slot(x.source,slot,at);};
  hooks.prepare_skill_use=[&](auto slot,auto event,auto at){return world.prepare_diversion_skill_use(x.source,slot,event,at);};
  hooks.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool instant){return world.finalize_diversion_skill_use(x.source,event,at,base,existing,instant);};
  hooks.abort_skill_use=[&](auto event){return world.abort_diversion_skill_use(x.source,event);};hooks.cooldown_completion_source=[&](auto event){return world.cooldown_completion_stamp(x.source,event);};
  Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[402]=1;attributes::Ranks ranks{};ranks[25]=12;
  check(cast.start(1000,{402,0,2},attacks,policy,ranks,true,hooks)==casting::Reason::accepted);
  cast.update(1500,hooks);const auto before=*cast.cooldown_source_snapshot(x.source,1500,hooks);check(before.timers.size()==1&&before.timers[0].until==11500);
  check(!world.publish_local_cooldowns(x.source,before));world.tick(1600);
  const auto notices=*world.pending_attack_resets(x.source);check(notices.size()==1&&!world.local_cooldown_snapshot(x.source));
  check(!world.publish_local_cooldowns(x.source,before));wire.clear();
  using R=effects::OrderedAttackCooldowns::Result;
  check(cast.consume_attack_reset(notices[0],1600,hooks)==R::accepted);
  check(std::find(wire.begin(),wire.end(),casting::slot_event(229,1,{402,0}))!=wire.end());const auto size=wire.size();
  check(cast.consume_attack_reset(notices[0],1600,hooks)==R::duplicate&&wire.size()==size);
  check(world.acknowledge_attack_reset(notices[0].stamp));const auto after=*cast.cooldown_source_snapshot(x.source,1600,hooks);
  check(after.timers.empty()&&after.reset_order==notices[0].stamp.order&&world.publish_local_cooldowns(x.source,after));
  const auto permit=world.prepare_cooldown_checkpoint(x.source,after,1600);check(permit&&cast.apply_cooldown_checkpoint(*permit,1600,hooks));
  check(world.pending_attack_resets(x.source)->empty()&&world.cooldown_order_failures(x.source)==0);
 }
 std::cout<<"PASS Runtime cooldown provenance and staged ordered reset delivery,newer base/disable preservation,duplicate idempotence and missing-authority nonmutation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
