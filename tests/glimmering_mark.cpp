#include "gw_glimmering_mark.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Glimmering Mark line "+std::to_string(at.line()));
}
int main()try{
 const Scope caster{1,1,1},target{1,2,1},other{1,3,1};unsigned cases=0;
 for(unsigned rank=0;rank<=20;++rank)for(bool dense:{false,true}){
  const auto plan=*glimmering_mark(rank);check(plan.damage==unsigned(std::lround(5.+20.*rank/15.)));
  check(plan.duration==10000&&plan.pulses==10&&plan.blind_ms==3000);
  GlimmeringMarkSchedule schedule;check(schedule.add(caster,target,7,20,plan,100));
  unsigned count=0;const auto receive=[&](const GlimmeringMarkPulse& p){
   check(p.caster==caster&&p.target==target&&p.effect==7&&p.level==20&&p.plan==plan);
   check(p.ordinal==++count&&p.at==100+count*1000&&p.applied==100);return true;};
  if(dense)for(unsigned at=200;at<12100;at+=100)check(schedule.advance(at,receive));
  check(schedule.advance(12100,receive));check(count==10&&schedule.size()==0&&!schedule.next_deadline());
  check(schedule.advance(12100,receive)&&count==10);++cases;
 }
 const auto plan=*glimmering_mark(12);
 {GlimmeringMarkSchedule s;check(s.add(caster,target,7,20,plan,100));check(s.add(target,other,8,20,plan,100));
  check(!s.end_target(target,99)&&s.size()==2);check(s.end_target(target,7)&&s.size()==1);
  check(s.targeted_use_completed(target,other,194,200).value()&&s.size()==0);}
 { // Fixed227 window retains a pulse while the next targeted spell casts;
   // target hex feedback ends at that next spell's completion, not its start.
  GlimmeringMarkSchedule s;check(s.add(caster,target,7,20,plan,100));unsigned hits=0;
  const auto pulse=[&](const auto&){++hits;return true;};
  check(s.advance(850,pulse)&&hits==0); // another spell starts; no end commit
  check(s.advance(1100,pulse)&&hits==1);
  check(s.targeted_use_completed(caster,target,1085,1865).value());
  check(s.advance(10100,pulse)&&hits==1&&s.size()==0);
 }
 {GlimmeringMarkSchedule s;check(s.add(caster,target,7,20,plan,100));
  check(!s.targeted_use_completed(other,target,194,200).value());check(!s.targeted_use_completed(caster,other,194,200).value());
  check(s.targeted_use_completed(caster,target,194,200).value()&&s.size()==0);}
 {GlimmeringMarkSchedule s;check(s.add(caster,target,7,20,plan,100));
  check(!s.targeted_use_completed(caster,target,194,1100));unsigned hits=0;
  check(s.advance(1100,[&](const auto&){++hits;return true;}));
  check(s.add(other,target,8,20,plan,1100));
  check(!s.targeted_use_completed(caster,target,194,1200).value());check(s.targeted_use_completed(other,target,194,1200).value());check(hits==1);}
 {GlimmeringMarkSchedule s;check(s.add(caster,target,7,20,plan,100));unsigned hits=0;
  check(s.advance(2100,[&](const auto& p){++hits;if(p.effect==7){check(!s.advance(9999,[](const auto&){return true;}));
   check(s.add(other,target,8,20,plan,p.at));return false;}check(p.effect==8&&p.ordinal==1);return true;}));
  check(hits==2&&s.size()==1);auto stale=caster;++stale.generation;s.retire(stale);check(s.size()==1);
  s.retire(other);check(s.size()==0);}
 {GlimmeringMarkSchedule s;check(s.add(caster,target,7,20,plan,100));unsigned hits=0;
  check(s.advance(10100,[&](const auto&){++hits;return false;}));check(hits==1&&s.size()==0);}
 {GlimmeringMarkSchedule s;check(s.add(caster,target,7,20,plan,100));
  auto stale=target;++stale.generation;s.retire(stale);check(s.size()==1);
  s.retire(target);check(s.size()==0);}
 {GlimmeringMarkSchedule s;auto forged=plan;++forged.damage;
  check(!s.add(caster,target,7,20,forged,100));check(!s.add(caster,target,0,20,plan,100));
  check(!s.add(caster,caster,7,20,plan,100));check(!s.add(caster,target,7,0,plan,100));
  check(!s.add(caster,target,7,20,plan,UINT64_MAX-12999));
  check(s.add(caster,target,7,20,plan,UINT64_MAX-13000));}
 { // A large delayed update with renewing callbacks has resumable work,
   // rather than an unbounded loop or duplicate pulse publication.
  GlimmeringMarkSchedule s;check(s.add(caster,target,7,20,plan,0));unsigned count=0;
  const auto renew=[&](const GlimmeringMarkPulse& p){
   check(p.at==std::uint64_t(++count)*1000);check(s.add(caster,target,7,20,plan,p.at));return false;};
  check(!s.advance(65537000,renew)&&count==65536);
  check(s.advance(65537000,renew)&&count==65537&&s.next_deadline()==65538000);
 }
 using namespace combat::weapon;
 check(glimmering_mark_blinds({AttackTargetAction::idle,{}})==false);
 check(glimmering_mark_blinds({AttackTargetAction::attack,{}})==false);
 check(glimmering_mark_blinds({AttackTargetAction::attack,skills::Slot{390,0}})==true);
 check(glimmering_mark_blinds({AttackTargetAction::skill,skills::Slot{390,0}})==true);
 check(glimmering_mark_blinds({AttackTargetAction::skill,skills::Slot{186,0}})==false);
 check(!glimmering_mark_blinds({AttackTargetAction::idle,skills::Slot{390,0}}));
 check(!glimmering_mark_blinds({AttackTargetAction::attack,skills::Slot{186,0}}));
 check(!glimmering_mark(21));
 std::cout<<cases<<" all-rank dense/sparse pulse cases; renewal, targeted end, reentry, retirement and attack-skill selection passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
