#include "gw_echo_commit.h"
#include "gw_skill_casting.h"
#include "gw_temporary_skill_expiry.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Copy restoration World line "+std::to_string(at.line()));
}
void complete(world::Instance& world,Scope owner,Slot slot,std::uint64_t event,std::uint64_t now){
 check(world.prepare_diversion_skill_use(owner,slot,event,now));
 check(bool(world.finalize_diversion_skill_use(owner,event,now,0,0)));
}
void runtime_echo(){
 for(unsigned echo_skill:{74u,75u})for(unsigned mode=0;mode<6;++mode){
  world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1);
  if(mode==5){
   w.join(2,"Enemy",30,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);
   check(w.diversion(*w.effect_scope(2),owner,*diversion(12),100));
  }
  Slots base{};base[0]={echo_skill,0};base[1]={281,0};check(w.configure_adrenaline(owner,base,100));
  TemporarySkillSession session(owner,base);TemporarySkillIds ids(owner);casting::Runtime runtime(1);casting::Hooks h;
  unsigned observed=0;std::vector<Packet> wire;
  h.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};
  h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.heal=[](auto,auto,float,bool){return true;};
  h.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
  h.consume_energy_at=[&](unsigned cost,auto now){return w.consume_energy(1,cost,owner,now);};
  h.current_skillbar=[&](){return std::optional{session.bar()};};
  h.skill_disabled_slot=[&](auto slot,auto now){return w.skill_disabled_slot(owner,slot,now);};
  h.effect_snapshot=[&](auto now){return w.status_snapshot(owner,now);};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return w.status_mutation(owner,skill,effect,rank,now,until,active);};
  h.arm_echo=[&](auto slot,auto effect,auto now,auto until){return mode!=3&&w.status_mutation(owner,slot.skill,effect,0,now,until,true,slot);};
  h.before_spell_effect=[&](auto skill,auto event,auto now){return w.before_spell_effect(owner,skill,event,now);};
  h.prepare_skill_use=[&](auto slot,auto event,auto now){return w.prepare_diversion_skill_use(owner,slot,event,now);};
  h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(owner,event);};
  h.finalize_skill_use=[&](auto event,auto now,auto base_ms,auto existing,bool instant){return w.finalize_diversion_skill_use(owner,event,now,base_ms,existing,instant);};
  h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(owner,event);};
  h.completed_skill_use=[&](const CooldownCompletion& use){
   ++observed;
   const auto copied=observe_echo_completion(w,session,ids,owner,use);
   if(!copied)return false;wire.insert(wire.end(),copied->packets.begin(),copied->packets.end());return true;
  };
  if(mode==4)h.arm_echo={};
  Policy policy;policy.primary=5;policy.secondary=3;policy.campaigns=255;policy.learned[echo_skill]=1;policy.learned[281]=1;attributes::Ranks ranks{};
  check(runtime.start(100,{echo_skill,0,1},session.bar(),policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==4){check(w.resources(1)->energy==100&&observed==0);continue;}
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(101,h);
  const auto finished=100+std::uint64_t(find_skill(echo_skill)->activation*1000);
  runtime.update(finished,h);check(!runtime.active());
  if(mode&&mode!=5){check(observed==0&&session.bar()==base);check(w.status_snapshot(owner,finished)->empty());continue;}
  check(observed==1);
  const auto start=finished+1000;
  check(runtime.start(start,{281,0,1},session.bar(),policy,ranks,true,h)==casting::Reason::accepted);
  runtime.update(start+1000,h);check(observed==2&&session.bar()[0]==Slot{281,1});
  const auto snapshot=runtime.cooldown_source_snapshot(owner,start+1000,h);check(snapshot&&w.publish_local_cooldowns(owner,*snapshot));
  const auto replacement=slot_packet_2022(1,0,{281,1});const auto replaced=std::find(wire.begin(),wire.end(),replacement);
  check(replaced!=wire.end()&&std::find(replaced,wire.end(),casting::terminal(58,1))!=wire.end());
  check(std::any_of(wire.begin(),wire.end(),[&](const auto& packet){return packet==casting::generic(21,1,echo_skill==74?128:129);}));
  if(mode==5&&echo_skill==75){
   const auto expired_at=start+1000+echo(echo_skill)->copy_duration;
   const auto restored=expire_temporary_copy(w,session,owner,expired_at);check(bool(restored));
   wire.clear();check(runtime.consume_copy_restoration(restored->receipt,expired_at,h,false)&&wire.empty());
   check(runtime.restored_slot_feedback({{75,0}},expired_at,h));
   const auto original_deadline=finished+std::uint64_t(find_skill(75)->recharge)*1000+diversion(12)->additional_recharge;
   check(original_deadline>restored->receipt.cooldowns().front().until);
   check(wire.size()==1&&wire.front()==casting::slot_event(228,1,{75,0},unsigned((original_deadline-expired_at+999)/1000)));
   // Recall's UI must retain the real combined Diversion/restoration deadline,
   // not replace it with ten seconds. Preparation emits nothing.
   wire.clear();auto recall_hooks=h;
   recall_hooks.skill_disabled_slot=[&](auto slot,auto at)->std::optional<std::uint64_t>{
    const auto prior=w.skill_disabled_slot(owner,slot,at);if(!prior)return {};return std::max(*prior,expired_at+10000);
   };
   const RecallFeedbackNotice notice{owner,42,expired_at,session.bar()};
   const auto feedback=runtime.prepare_recall_feedback(notice,expired_at,recall_hooks);
   check(feedback&&wire.empty()&&feedback->packets==std::vector<Packet>{casting::slot_event(228,1,{281,0},10)});
   auto missing=recall_hooks;missing.skill_disabled_slot=[](auto,auto)->std::optional<std::uint64_t>{return {};};
   check(!runtime.prepare_recall_feedback(notice,expired_at,missing)&&wire.empty());
   auto replaced=session.bar();replaced[0]={75,9};recall_hooks.current_skillbar=[&](){return std::optional{replaced};};
   const auto changed=runtime.prepare_recall_feedback(notice,expired_at,recall_hooks);
   check(changed&&changed->packets.size()==1&&changed->packets.front()==casting::slot_event(228,1,{281,0},10));
  }

 }
}
int main()try{
 runtime_echo();
 for(unsigned source_skill:{74u,75u})for(unsigned delay:{0u,5000u,25000u}){
  world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1);const auto rule=*echo(source_skill);Slots base{};base[0]={source_skill,0};base[1]={76,0};
  check(w.configure_adrenaline(owner,base,100));TemporarySkillSession session(owner,base);TemporarySkillIds ids(owner);
  check(w.status_mutation(owner,source_skill,100,0,100,100+rule.armed_duration,true,Slot{source_skill,0}));
  complete(w,owner,{76,0},1,200);
  check(bool(commit_echo_copy(w,session,ids,owner,{source_skill,0},{76,0},200,1)));
  const auto expiry=200+rule.copy_duration;const auto at=expiry+delay;const auto until=expiry+rule.restore_cooldown;
  const auto restored=expire_temporary_copy(w,session,owner,at);check(restored&&session.bar()==base&&w.cooldown_bar_snapshot(owner)->bar==base);
  casting::Runtime runtime(1);casting::Hooks hooks;std::vector<Packet> wire;
  hooks.target_scope=[&](auto id){return w.effect_scope(id);};hooks.current_skillbar=[&](){return std::optional{session.bar()};};
  hooks.skill_disabled_slot=[&](auto slot,auto time){return w.skill_disabled_slot(owner,slot,time);};
  hooks.events=[&](const auto& batch){wire.insert(wire.end(),batch.begin(),batch.end());};
  if(at<until){
   // Failed imports must leave neither a replay watermark nor a local timer.
   casting::Runtime retry(1);auto bad=hooks;
   bad.current_skillbar=[&](){Slots missing{};return std::optional{missing};};
   check(!retry.consume_copy_restoration(restored->receipt,at,bad)&&wire.empty());
   check(retry.cooldown_source_snapshot(owner,at,hooks)->timers.empty());
   check(!retry.consume_copy_restoration(restored->receipt,at-1,hooks)&&wire.empty());
   bad=hooks;bad.target_scope=[&](auto){return std::optional{Scope{owner.instance,owner.actor,owner.generation+1}};};
   check(!retry.consume_copy_restoration(restored->receipt,at,bad)&&wire.empty());
   if(source_skill==75){
    bad=hooks;bad.skill_disabled_slot=[](auto,auto)->std::optional<std::uint64_t>{return {};};
    check(!retry.consume_copy_restoration(restored->receipt,at,bad)&&wire.empty());
    bad.skill_disabled_slot=[](auto,auto)->std::optional<std::uint64_t>{return 0;};
    check(!retry.consume_copy_restoration(restored->receipt,at,bad)&&wire.empty());
   }
   check(retry.consume_copy_restoration(restored->receipt,at,hooks)&&wire.size()==1);wire.clear();
  }
  check(runtime.consume_copy_restoration(restored->receipt,at,hooks));
  check(wire.size()==unsigned(at<until));
  if(at<until)check(wire.front()==casting::slot_event(228,1,{source_skill,0},unsigned((until-at+999)/1000)));
  const auto imported=runtime.cooldown_source_snapshot(owner,at,hooks);check(bool(imported));
  check(imported->timers.size()==unsigned(source_skill==74&&at<until));
  // This isolated Runtime did not process the preceding completion notice.
  if(!imported->timers.empty())check(!w.publish_local_cooldowns(owner,*imported));
  const auto count=wire.size();check(runtime.consume_copy_restoration(restored->receipt,at,hooks)&&wire.size()==count);
  // Replacing native identity clears its old timer, so refresh even when
  // importing the receipt did not increase a deadline (or all events expired).
  wire.clear();check(runtime.restored_slot_feedback({{source_skill,0}},at,hooks));
  check(wire.size()==1&&wire.front()==(at<until?casting::slot_event(228,1,{source_skill,0},unsigned((until-at+999)/1000)):casting::slot_event(229,1,{source_skill,0})));
  auto unavailable=hooks;unavailable.skill_disabled_slot=[](auto,auto)->std::optional<std::uint64_t>{return {};};
  wire.clear();check(!runtime.restored_slot_feedback({{source_skill,0}},at,unavailable)&&wire.empty());
  auto longer=hooks;longer.skill_disabled_slot=[&](auto,auto)->std::optional<std::uint64_t>{return at+30000;};
  check(runtime.restored_slot_feedback({{source_skill,0}},at,longer));
  check(wire.size()==1&&wire.front()==casting::slot_event(228,1,{source_skill,0},30));

  casting::Runtime late(1);wire.clear();check(late.consume_copy_restoration(restored->receipt,std::max(at,until),hooks)&&wire.empty());
  const auto& events=restored->receipt.cooldowns();check(events.size()==unsigned(at<until));
  if(!events.empty()){
   const auto& event=events[0];check(event.until==until&&event.stamp.owner==owner&&event.stamp.at==at&&event.slot==Slot{source_skill,0}&&event.kind==rule.restoration);
   check(!w.skill_disabled_slot(owner,{source_skill,0},at-1));
   if(source_skill==74){
    LocalCooldownSnapshot snapshot{owner,imported->revision+1,at,{{event.slot,event.until,CooldownOrigin{event.stamp,CooldownOriginKind::restoration,event.generation}}},*w.control_epoch(owner),0,0};
    snapshot.completion_order=w.cooldown_completion_stamp(owner,1)->stamp.order;
    check(w.publish_local_cooldowns(owner,snapshot));
    check(w.local_cooldown_snapshot(owner)->verified==std::vector<Slot>{{source_skill,0}});
    ++snapshot.revision;++snapshot.timers.front().until;
    check(!w.publish_local_cooldowns(owner,snapshot)&&!w.local_cooldown_snapshot(owner));
   }
  }
  const auto blocked=w.skill_disabled_slot(owner,{source_skill,0},at);check(blocked&&*blocked==(source_skill==75&&at<until?until:0));
  check(w.skill_disabled_until(owner,source_skill,at)==blocked);
  if(source_skill==75&&at<until){
   const auto revision=w.disable_reset_revision(owner);check(bool(revision));
   check(w.reset_skill_disables(owner,{{source_skill,0}},*revision,at));
   check(w.skill_disabled_slot(owner,{source_skill,0},at)==until);
  }
  check(w.skill_disabled_slot(owner,{source_skill,1},at)==0);
  check(w.skill_disabled_slot(owner,{source_skill,0},std::max(at,until))==0);
  const auto again=expire_temporary_copy(w,session,owner,at);check(again&&again->changes.empty()&&again->receipt.cooldowns().empty());
 }
 {const Scope owner{1,1,1};CooldownBarAuthority authority(owner);Slots before{},after{};before[0]={76,1};after[0]={75,0};check(bool(authority.publish(owner,before,100)));
  TemporarySkillRestoration change{{0,{75,0},{76,1},100,20100,1,20000,EchoRestorationKind::independent_disable},true};
  check(!prepare_copy_restoration(authority,owner,before,after,{change},20099));
  auto corrupt=change;corrupt.grant.restore_cooldown=19999;check(!prepare_copy_restoration(authority,owner,before,after,{corrupt},20100));
  auto unrelated=after;unrelated[1]={74,0};check(!prepare_copy_restoration(authority,owner,before,unrelated,{change},20100));
  change.grant.applied=UINT64_MAX-30000;change.grant.expires=UINT64_MAX-1;
  check(!prepare_copy_restoration(authority,owner,before,after,{change},UINT64_MAX-1));
 }
 std::cout<<"PASS atomic copy/root restoration,scope-minted cooldown receipts,Arcane independent disable,expiry-anchored late delivery,copy isolation and malformed/overflow rejection\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

