#include "gw_world_simulation.h"
#include "gw_temporary_skill_session.h"
#include <iostream>
#include <source_location>
#include <type_traits>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Echo session/World line "+std::to_string(at.line()));
}
void complete(world::Instance& world,Scope owner,Slot slot,std::uint64_t event,std::uint64_t now){
 check(world.prepare_diversion_skill_use(owner,slot,event,now));
 check(bool(world.finalize_diversion_skill_use(owner,event,now,0,0)));
}
int main()try{
 static_assert(!std::is_constructible_v<EchoConsumption,Scope,const EchoUseProposal&>);
 for(unsigned mode=0;mode<9;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,50,100},100);
  const auto owner=*w.effect_scope(1);Slots base{};base[0]={75,0};base[1]={76,0};base[2]={1,0};
  check(w.configure_adrenaline(owner,base,100));TemporarySkillSession session(owner,base);
  Policy policy;policy.primary=5;policy.secondary=1;policy.campaigns=255;for(unsigned id:{75u,76u,1u})policy.learned[id]=1;
  check(w.status_mutation(owner,75,200,0,100,20100,true,Slot{75,0}));
  const Slot used{mode==6?1u:76u,0};complete(w,owner,used,1,200);const auto offer=w.prepare_echo_use(owner,{75,0},used,200,1);check(bool(offer));
  auto prepared=session.prepare_echo(*offer,0,mode==6?Slot{75,0}:Slot{76,1});check(prepared&&session.ready(*prepared)&&session.bar()==base);
  if(mode==1){check(w.status_mutation(owner,75,201,0,200,20200,true,Slot{75,0}));}
  if(mode==3){check(bool(session.expire(owner,201)));check(!session.ready(*prepared));continue;}
  if(mode==4){auto edited=base;edited[2]={146,0};check(w.publish_skillbar_transition(owner,{base,edited},200));}
  if(mode==5){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},200,1);w.set_vitals(1,{500,500,50,100},100);check(!w.commit_echo_use(owner,*offer,SkillbarTransition{base,prepared->bar()}));continue;}
  if(mode==8)check(w.settle_resources(owner,20100));
  std::optional<SkillbarTransition> transition;
  if(mode!=6)transition=SkillbarTransition{base,prepared->bar()};
  if(mode==2)transition->after[2]={146,0};
  if(mode==0||mode==7)check(w.status_mutation(owner,11,301,0,200,1200,true)); // New unrelated state after prepare.
  const auto receipt=w.commit_echo_use(owner,*offer,transition);
  const bool accepted=mode==0||mode==6||mode==7;
  check(bool(receipt)==accepted);
  if(!accepted){check(session.bar()==base);if(mode!=8){const auto status=*w.status_snapshot(owner,200);check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==75;}));}continue;}
  check(w.cooldown_bar_snapshot(owner)->bar==prepared->bar());
  const auto status=*w.status_snapshot(owner,200);check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==75;}));
  if(mode!=6)check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==11;}));
  check(session.commit_echo(std::move(*prepared),*receipt));check(!session.commit_echo(std::move(*prepared),*receipt));
  check(!w.commit_echo_use(owner,*offer,transition));
  if(mode==6){check(session.bar()==base);continue;}
  check(session.bar()[0]==Slot{76,1}&&session.base()==base);
  const auto admission=session.admit(owner,policy,0,{76,1},200);check(bool(admission));
  check(session.valid(*admission,policy,20199)&&!session.valid(*admission,policy,20200));
  const auto before=session.bar();const auto restored=session.expire(owner,20200);
  check(restored&&restored->size()==1&&restored->front().grant.restore_cooldown==20000&&session.bar()==base);
  check(w.publish_skillbar_transition(owner,{before,session.bar()},20200));
  check(w.cooldown_bar_snapshot(owner)->bar==base);
 }
 std::cout<<"PASS Echo World/session atomic bar and enchant consumption,renewal/edit/lifetime/time rejection,unrelated-state preservation,nonspell end and cast-capability expiry\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
