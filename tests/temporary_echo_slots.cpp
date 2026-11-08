#include "gw_temporary_skill_slots.h"
#include <iostream>
#include <source_location>
#include <string>
#include <stdexcept>
using namespace fw::gw::skills;using namespace effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Nested temporary copy line "+std::to_string(at.line()));
}
EchoUseProposal proposal(Scope owner,unsigned echo_skill,unsigned copied,Time applied,Time now,unsigned copy=0){
 Registry registry(owner);const auto plan=*echo(echo_skill);
 check(registry.apply(owner,{1,owner.actor,echo_skill,0,Category::enchantment,applied,applied+plan.armed_duration},Replace::same_skill).accepted);
 const auto prepared=prepare_echo_completed_use(registry,owner,{echo_skill,copy},{copied,0},now);check(bool(prepared));return *prepared;
}
int main()try{
 const Scope owner{1,2,3};TemporarySkillSlots copies(owner);Slots bar{};bar[0]={75,0};bar[1]={74,0};bar[2]={76,0};
 Policy policy;policy.primary=5;policy.campaigns=255;for(unsigned id:{74u,75u,76u})policy.learned[id]=1;
 const auto outer=proposal(owner,75,74,100,1000);
 check(!copies.install_echo(owner,bar,0,{75,0},{74,0},1000,outer));
 check(!copies.install_echo(owner,bar,0,{75,0},{74,65536},1000,outer));
 check(!copies.install_echo(owner,bar,0,{75,0},{74,1},1001,outer));
 check(bool(copies.install_echo(owner,bar,0,{75,0},{74,1},1000,outer)));
 const auto old=copies.authorize(owner,bar,0,{74,1},1000);check(bool(old));
 const auto inner=proposal(owner,74,76,1000,2000,1);
 check(bool(copies.install_echo(owner,bar,0,{74,1},{76,1},2000,inner)));
 check(!copies.valid(*old,bar,2000));
 const auto live=copies.authorize(owner,bar,0,{76,1},2000);check(bool(live));
 check(copies.eligibility_bar(owner,bar,2000)->at(0)==Slot{75,0});
 check(validate(*copies.eligibility_bar(owner,bar,2000),policy)==Reason::accepted);
 check(copies.expire(owner,bar,21000)->empty()&&bar[0]==Slot{76,1});
 check(copies.valid(*live,bar,31999)&&!copies.valid(*live,bar,32000));
 const auto restored=copies.expire(owner,bar,32000);
 check(restored&&restored->size()==1&&restored->front().restored&&bar[0]==Slot{75,0});
 check(restored->front().grant.restore_cooldown==20000&&restored->front().grant.restoration==EchoRestorationKind::independent_disable);
 check(!copies.valid(*old,bar,32000)&&!copies.valid(*live,bar,32000));
 TemporarySkillSlots nested(owner);Slots second{};second[0]={74,0};second[1]={28,0};
 const auto hex=proposal(owner,74,22,100,1000);
 check(bool(nested.install_echo(owner,second,0,{74,0},{22,1},1000,hex)));
 check(bool(nested.install(owner,second,0,{22,1},{28,1},2000)));
 const auto done=nested.expire(owner,second,22000);
 check(done&&done->size()==1&&second[0]==Slot{74,0}&&done->front().grant.restore_cooldown==10000);
 check(!nested.install(owner,second,0,{74,0},{28,1},23000));
 std::cout<<"PASS shared nested Echo/Inspired Hex leaves preserve root eligibility,replace expiry,invalidate old grants and retain original restoration cooldown\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
