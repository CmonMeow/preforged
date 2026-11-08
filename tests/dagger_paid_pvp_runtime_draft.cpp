#include "dagger_paid_pvp_fixture_draft.h"
int main()try{
 for(unsigned skill:{3061u,776u,777u,778u,779u,3251u,782u,783u})for(bool npc:{false,true}){
  Fixture f(skill,npc);f.enable_pvp();f.arm();const auto before=f.health();
  check(!skills::casting::Runtime::effect_supported(skill));check(f.start()==skills::casting::Reason::accepted);
  const auto release=f.last_permit->release_time();f.update(release);
  check(f.last_result.accepted&&!f.last_result.prerequisite_failed&&f.health()<before);
  check(f.payments==1&&f.paid==skills::find_skill(skill)->energy_cost());
  check(f.until()==release+1000ull*skills::find_skill(skill)->recharge);
  const auto packets=f.box->drain();
  check(std::find(packets.begin(),packets.end(),skills::casting::activation(1,f.target.actor,skill,true))!=packets.end());
  check(std::find(packets.begin(),packets.end(),skills::casting::slot_event(228,1,{static_cast<std::uint16_t>(skill),0},skills::find_skill(skill)->recharge))!=packets.end());
  if(combat::weapon::dagger_dual_role(skill)){
   auto phase=f.last_result.successful_second;check(bool(phase)&&f.cast.active());const auto hp=f.health();
   f.world.take_attack_outcomes(f.source);f.update(phase->due());const auto outcomes=f.world.take_attack_outcomes(f.source);
   check(!f.cast.active()&&!phase->open()&&outcomes.size()==1&&f.health()<hp&&f.payments==1);
  }
  check(!skills::casting::Runtime::effect_supported(skill));
 }
 for(unsigned skill:{3061u,3251u}){
  Fixture f(skill);f.enable_pvp();check(f.start()==skills::casting::Reason::accepted);
  const auto release=f.last_permit->release_time();f.update(release);
  check(f.last_result.accepted&&f.last_result.prerequisite_failed&&f.last_result.reset_recharge&&f.until()==0);
  check(f.payments==1&&f.paid==5&&f.refunds==0&&f.health()==1000);
  auto first=f.world.take_attack_outcomes(f.source);check(first.size()==1&&first[0].skill_failed&&!first[0].before_damage);
  if(skill==3061){f.update(release+500);auto second=f.world.take_attack_outcomes(f.source);
   check(second.size()==1&&second[0].skill_failed&&second[0].event!=first[0].event&&!second[0].before_damage);}
  check(f.health()==1000&&f.payments==1);
 }
 for(unsigned skill:{3061u,3251u}){
  Fixture f(skill);f.enable_pvp(1);f.arm();check(f.durable.learned.size()==1&&!f.durable.learned.contains(skill));
  check(f.start()==skills::casting::Reason::accepted);const auto release=f.last_permit->release_time();f.update(release);
  check(f.last_result.accepted&&!f.last_result.prerequisite_failed&&f.payments==1&&f.until()==release+6000);
  if(skill==3061){const auto phase=f.last_result.successful_second;check(bool(phase));const auto hp=f.health();
   f.update(phase->due());check(!f.cast.active()&&f.health()<hp&&f.payments==1);}
 }
 for(unsigned skill:{3061u,3251u}){
  Fixture f(skill);f.enable_pvp();f.copy=1;f.bar[0]={skill,f.copy}; // request matches forged slot; session grant is still absent
  check(f.start()==skills::casting::Reason::policy&&f.payments==0);
 }
 for(unsigned skill:{3061u,3251u}){Fixture f(skill);f.enable_pvp();f.policy.pvp_match=false;check(f.start()==skills::casting::Reason::unsupported&&f.payments==0);}
 for(unsigned skill:{775u,780u}){Fixture f(skill);f.policy.pvp_match=true;check(f.start()==skills::casting::Reason::unsupported&&f.payments==0);}
 std::cout<<"PASS candidate exact native PvP dagger aliases/mode/wire/recharge/paid nonhits; no binding promotion\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
