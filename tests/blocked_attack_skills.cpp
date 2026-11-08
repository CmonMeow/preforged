#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){
 if(!x)throw std::runtime_error("blocked attacks line "+std::to_string(at.line()));
}
int main()try{
 using namespace combat::weapon;
 for(unsigned id:{327u,341u,356u,386u,1134u})for(unsigned rank=0;rank<=20;++rank){
  const auto p=*attack_skill(id,rank,0);check(valid(p));
  check(p.bonus==unsigned(std::lround((id==327||id==356||id==1134)?5.+rank:1.+19.*rank/15.)));
  for(auto outcome:{AttackOutcome::hit,AttackOutcome::miss,AttackOutcome::dodge,AttackOutcome::obstruction,AttackOutcome::cancelled})check(!blocked_attack_effect(p,outcome));
  const auto blocked=blocked_attack_effect(p,AttackOutcome::block);check(bool(blocked));
  check(blocked->damage==((id==327||id==1134)?unsigned(std::lround(10.+24.*rank/15.)):p.bonus));
  auto forged=p;++forged.bonus;check(!blocked_attack_effect(forged,AttackOutcome::block));
 }
 for(unsigned id:{327u,341u,356u,386u,1134u})for(bool blocked:{false,true})for(bool miss:{false,true}){
  warrior_test::Fixture f(id,false);if(id==327||id==1134){f.weapon.item_type=27;check(f.world.ordinary_weapon(f.source,f.weapon,20));}
  if(blocked)check(f.world.apply_health_regeneration(f.target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  f.world.configure_attack_block_draws([]{return 0.;});
  if(miss)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,200,3000));
  check(f.world.melee_attack(f.source,f.target,f.weapon,*attack_skill(id,12,0),500));
  const auto event=f.world.take_attack_outcomes(f.source);check(event.size()==1);
  check(event[0].outcome==(miss?AttackOutcome::miss:blocked?AttackOutcome::block:AttackOutcome::hit));
  if(miss)check(f.health()==1000);else check(f.health()<1000);
  check(f.world.knocked_down(f.target,500)==(!miss&&blocked&&(id==327||id==356||id==1134)));
  check(f.world.has_condition(f.target,combat::conditions::Kind::deep_wound,500)==(!miss&&blocked&&id==341));
  check(f.world.has_condition(f.target,combat::conditions::Kind::bleeding,500)==(!miss&&blocked&&id==386));
  if(!miss&&blocked&&(id==327||id==356||id==1134))check(f.health()==1000-blocked_attack_effect(*attack_skill(id,12,0),AttackOutcome::block)->damage);
 }
 for(unsigned id:{327u,341u,356u,386u,1134u})for(unsigned mode=0;mode<6;++mode){
  using namespace skills;
  warrior_test::Fixture f(id,false,mode==4?1:1000);
  if(id==327||id==1134){f.weapon.item_type=27;check(f.world.ordinary_weapon(f.source,f.weapon,20));}
  check(f.world.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  f.world.configure_attack_block_draws([]{return 0.;});
  Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[id]=1;
  attributes::Ranks ranks{};ranks[find_skill(id)->attribute]=12;
  std::uint64_t clock=1000;unsigned commits=0;std::vector<world::Packet> packets;
  casting::Hooks h;
  h.resources=[&](unsigned actor){return f.world.resources(actor);};
  h.events=[&](const auto& values){packets.insert(packets.end(),values.begin(),values.end());};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.world.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned actor){return f.world.effect_scope(actor);};
  h.weapon_snapshot=[&](){return std::optional{f.weapon};};
  h.melee_target=[&](unsigned actor,auto at){return f.world.melee_target(f.source,actor,at,id);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return f.world.melee_attack(f.source,target,weapon,plan,at);};
  h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,clock);};
  h.attack_interrupt_epoch=[&](){return f.world.attack_interrupt_epoch(f.source);};
  h.weapon_attack_timing=[&](auto at){return f.world.weapon_attack_timing(f.source,at);};
  h.attack_duration=[&](auto base,auto at){return f.world.attack_duration(f.source,base,at);};
  casting::Runtime cast(1);
  check(cast.start(clock,{static_cast<std::uint16_t>(id),0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  check(f.world.resources(1)->energy==25&&!commits);
  check(std::count(packets.begin(),packets.end(),casting::activation(1,f.target.actor,id,true))==1);
  clock=1200;
  if(mode==1)cast.cancel(h);
  if(mode==2)cast.interrupt(clock,h);
  if(mode==3)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,clock,5000));
  if(mode==5){f.world.leave(2);f.world.join(2,"New lifetime",60,0,std::make_shared<world::Outbox>(),0,{}, {},clock,2);f.world.set_vitals(2,{1000,1000,30,30},100);}
  clock=1499;cast.update(clock,h);check(!commits);
  clock=1500;cast.update(clock,h);check(!cast.active());
  const auto outcomes=f.world.take_attack_outcomes(f.source);
  const bool attempted=mode==0||mode==3||mode==4;
  check(commits==unsigned(attempted)&&outcomes.size()==unsigned(attempted));
  if(attempted){check(outcomes[0].outcome==(mode==3?AttackOutcome::miss:AttackOutcome::block));check(!outcomes[0].before_damage);}
  const bool secondary=mode==0||mode==4;
  if(secondary)check(f.health()<(mode==4?1:1000));else check(f.world.resources(2)->health==1000);
  check(f.world.has_condition(f.target,combat::conditions::Kind::deep_wound,clock)==(mode==0&&id==341));
  check(f.world.has_condition(f.target,combat::conditions::Kind::bleeding,clock)==(mode==0&&id==386));
 }
 // Secondary damage uses general damage protection, but is neither a weapon
 // hit nor physical weapon damage. Armor/absorption must not reduce it.
 for(unsigned id:{327u,356u,1134u})for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(id,false);
  if(id==327||id==1134){f.weapon.item_type=27;check(f.world.ordinary_weapon(f.source,f.weapon,20));}
  check(f.world.physical_armor(f.target,140));check(f.world.physical_absorption(f.target,3));
  check(f.world.apply_health_regeneration(f.target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  f.world.configure_attack_block_draws([]{return 0.;});
  if(mode&1)check(f.world.status_mutation(f.target,346,10,0,200,3000,true));
  if(mode&2)check(f.world.protective_enchantment(f.target,f.target,*skills::regeneration::reviewed(299,12),0,200));
  const auto before=f.health();const auto plan=*attack_skill(id,12,0);
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,500));
  const unsigned damage=blocked_attack_effect(plan,AttackOutcome::block)->damage;
  // Shielding Hands at Protection12 reduces damage by15 after doubling.
  const auto expected=std::max(0,int(damage*((mode&1)?2:1))-((mode&2)?15:0));
  check(f.health()==before-expected);
  const auto outcomes=f.world.take_attack_outcomes(f.source);
  check(outcomes.size()==1&&outcomes[0].outcome==AttackOutcome::block&&!outcomes[0].before_damage);
 }
 std::cout<<"PASS five blocked-attack policies,casting lifecycle and secondary armor/Frenzy/Shielding interaction matrix\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
