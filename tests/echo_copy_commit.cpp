#include "gw_echo_commit.h"
#include "gw_inspired_hex_commit.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Echo copy commit line "+std::to_string(at.line()));
}
void complete(world::Instance& world,Scope owner,Slot slot,std::uint64_t event,std::uint64_t now){
 check(world.prepare_diversion_skill_use(owner,slot,event,now));
 check(bool(world.finalize_diversion_skill_use(owner,event,now,0,0)));
}
int main()try{
 for(unsigned source_skill:{74u,75u})for(bool nonspell:{false,true})for(bool failed_first:{false,true}){
  world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1);const auto plan=*echo(source_skill);const Slot source{source_skill,0},used{nonspell?1u:76u,0};
  Slots base{};base[0]=source;base[1]=used;base[3]={source_skill,9};check(w.configure_adrenaline(owner,base,100));
  TemporarySkillSession session(owner,base);TemporarySkillIds ids(owner);
  check(!w.status_mutation(owner,source_skill,100,0,100,100+plan.armed_duration,true));
  check(!w.status_mutation(owner,source_skill,100,0,100,100+plan.armed_duration,true,Slot{source_skill,99}));
  check(w.status_mutation(owner,source_skill,100,0,100,100+plan.armed_duration,true,source));
  // This other copy is present in the authoritative bar, but did not arm it.
  check(!w.prepare_echo_use(owner,{source_skill,9},used,200));
  check(!w.prepare_echo_use(owner,source,{146,0},200));
  check(!commit_echo_copy(w,session,ids,owner,source,used,200,1));
  complete(w,owner,used,1,200);
  if(failed_first){
   auto edited=base;edited[2]={146,0};check(w.publish_skillbar_transition(owner,{base,edited},200));
   check(!commit_echo_copy(w,session,ids,owner,source,used,200,1));
   check(session.bar()==base&&w.status_snapshot(owner,200)->size()==1);
   check(w.publish_skillbar_transition(owner,{edited,base},200));
  }
  const auto committed=commit_echo_copy(w,session,ids,owner,source,used,200,1);check(bool(committed));
  const bool copy=source_skill==74||!nonspell;
  check(bool(committed->changed)==copy&&committed->packets.size()==unsigned(copy));
  check(w.status_snapshot(owner,200)->empty()&&session.base()==base);
  if(!copy){check(session.bar()==base&&w.cooldown_bar_snapshot(owner)->bar==base);continue;}
  const Slot replacement{used.skill,1};check(committed->changed->after==replacement&&session.bar()[0]==replacement);
  check(committed->packets[0]==slot_packet_2022(1,0,replacement));
  check(w.cooldown_bar_snapshot(owner)->bar==session.bar());
  check(!commit_echo_copy(w,session,ids,owner,source,used,201));
  check(!ids.allocate(owner,1,session.bar())); // Broad Echo grants do not widen the Hex-only API.
  const auto restored=expire_hex_copy(w,session,owner,200+plan.copy_duration);check(restored&&restored->size()==1&&session.bar()==base);
  const auto later=201+plan.copy_duration+plan.restore_cooldown;
  check(w.status_mutation(owner,source_skill,101,0,later,later+plan.armed_duration,true,source));
  check(!commit_echo_copy(w,session,ids,owner,source,used,later,1));
  complete(w,owner,used,2,later);
  const auto next=commit_echo_copy(w,session,ids,owner,source,used,later,2);check(next&&next->changed&&next->changed->after==Slot{used.skill,2});
 }
 {
  world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1);Slots base{};base[0]={74,0};base[1]={76,0};check(w.configure_adrenaline(owner,base,100));
  complete(w,owner,{76,0},1,100);
  check(w.status_mutation(owner,74,100,0,100,30100,true,Slot{74,0}));
  // Millisecond equality cannot make an earlier completed use trigger a new arm.
  check(!w.prepare_echo_use(owner,{74,0},{76,0},100,1));
  check(w.prepare_diversion_skill_use(owner,{76,0},2,100));
  check(!w.prepare_echo_use(owner,{74,0},{76,0},100,2)); // Prepared is not completed.
  check(w.abort_diversion_skill_use(owner,2));
  check(!w.prepare_echo_use(owner,{74,0},{76,0},100,2)); // Aborted is not completed.
  complete(w,owner,{76,0},3,100);
  check(!w.prepare_echo_use(owner,{74,0},{1,0},100,3));
  check(!w.prepare_echo_use(owner,{74,0},{76,0},101,3));
  const auto offer=w.prepare_echo_use(owner,{74,0},{76,0},100,3);check(bool(offer));
  auto forged=*offer;forged.completion_event=2;check(!w.commit_echo_use(owner,forged));
  check(w.prepare_echo_use(owner,{74,0},{76,0},100,3).has_value());
 }
 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1);Slots base{};base[0]={74,0};base[1]={75,0};base[2]={mode==1?1u:76u,0};check(w.configure_adrenaline(owner,base,100));
  TemporarySkillSession session(owner,base);TemporarySkillIds ids(owner);
  check(w.status_mutation(owner,74,100,0,100,30100,true,Slot{74,0}));
  check(w.status_mutation(owner,75,101,0,100,20100,true,Slot{75,0}));
  complete(w,owner,base[2],1,200);
  LocalCooldownSnapshot acknowledged{owner,1,200,{}};
  acknowledged.completion_order=w.cooldown_completion_stamp(owner,1)->stamp.order;
  check(w.publish_local_cooldowns(owner,acknowledged)&&w.local_cooldown_snapshot(owner).has_value());
  if(mode==2){
   const auto a=w.prepare_echo_use(owner,{74,0},base[2],200,1),b=w.prepare_echo_use(owner,{75,0},base[2],200,1);check(a&&b);
   auto after=base;after[0]={76,1};after[1]={76,2};
   check(w.status_mutation(owner,75,102,0,200,20200,true,Slot{75,0}));
   check(!w.commit_echo_uses(owner,{*a,*b},SkillbarTransition{base,after}));
   check(w.status_snapshot(owner,200)->size()==2&&w.cooldown_bar_snapshot(owner)->bar==base&&session.bar()==base);
   continue;
  }
  if(mode==3||mode==4){
   const std::vector<Slot> rejected=mode==3?std::vector<Slot>{{74,0},{75,9}}:std::vector<Slot>{{74,0},{74,0}};
   check(!commit_echo_copies(w,session,ids,owner,rejected,base[2],200,1));
   check(session.bar()==base&&w.cooldown_bar_snapshot(owner)->bar==base&&w.status_snapshot(owner,200)->size()==2);
  }
  auto completed=*w.cooldown_completion_stamp(owner,1);
  auto forged=completed;++forged.stamp.order;
  check(!observe_echo_completion(w,session,ids,owner,forged));
  const auto committed=observe_echo_completion(w,session,ids,owner,completed);check(bool(committed));
  check(!w.local_cooldown_snapshot(owner));
  check(committed->changes.size()==(mode==1?1u:2u)&&committed->packets.size()==committed->changes.size());
  check(session.bar()[0]==Slot{base[2].skill,1});
  check(session.bar()[1]==(mode==1?Slot{75,0}:Slot{base[2].skill,2}));
  check(w.cooldown_bar_snapshot(owner)->bar==session.bar()&&w.status_snapshot(owner,200)->empty());
  check(!commit_echo_copies(w,session,ids,owner,{{74,0},{75,0}},base[2],200,1));
 }
 {
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Mesmer",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,100,100},100);}
  const auto owner=*w.effect_scope(1),enemy=*w.effect_scope(2);Slots base{};base[0]={74,0};base[1]={22,0};check(w.configure_adrenaline(owner,base,100));
  TemporarySkillSession session(owner,base);TemporarySkillIds ids(owner);
  check(w.status_mutation(owner,74,100,0,100,30100,true,Slot{74,0}));
  check(w.backfire(enemy,owner,*backfire(28,0),20,100));
  check(w.prepare_diversion_skill_use(owner,{22,0},1,200));
  const auto inspired=commit_inspired_hex_copy(w,session,ids,owner,owner,*inspired_hex(22,0),{22,0},200);check(bool(inspired));
  check(session.bar()[1]==Slot{28,1});
  check(bool(w.finalize_diversion_skill_use(owner,1,200,0,0)));
  const auto observed=observe_echo_completion(w,session,ids,owner,*w.cooldown_completion_stamp(owner,1));check(bool(observed));
  check(observed->changes.size()==1&&session.bar()[0]==Slot{22,1}&&session.bar()[1]==Slot{28,1});
  check(w.cooldown_bar_snapshot(owner)->bar==session.bar());
 }
 std::cout<<"PASS Echo transactional copy allocation,native216 slot packets,Hex API isolation,failed-publication cursor rollback,nonspell end and no lifetime ID reuse\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
