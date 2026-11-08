#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Conditional Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,target;unsigned original;
 Fixture(unsigned hp=500,bool conditioned=true):original(hp){
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Actor",id==1?0:id==2?60:id==3?100:400,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:id==2?hp:500u,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),{},60));}
  source=*w.effect_scope(1);target=*w.effect_scope(2);
  if(conditioned)for(unsigned id:{2u,3u})check(w.apply_condition(source,*w.effect_scope(id),combat::conditions::Kind::blind,100,10000));
 }
 void reversal(unsigned rank,unsigned actor=0,unsigned at=100){for(unsigned id:{2u,3u,4u})if(!actor||actor==id)check(w.status_mutation(*w.effect_scope(id),307,at==100?900:901,rank,at,at+8000,true));}
 bool has(unsigned actor,unsigned skill){const auto entries=*w.status_snapshot(*w.effect_scope(actor),101);return std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.skill==skill;});}
 double hp(unsigned actor=2){return w.resources(actor)->health;}
 bool cast(unsigned skill,unsigned rank=12){return w.condition_damage(source,target,*effects::condition_damage(skill,rank),20,101);}
};
double expected(unsigned hp,unsigned damage,int rank){const auto heal=rank<0?0u:std::min(damage,effects::reversal_fortune(rank)->maximum_healing);return std::max(0.,std::min(1000.,double(hp+heal))-double(damage-heal));}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{828u,864u})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u})for(bool conditioned:{false,true}){
  ++cases;Fixture f(hp,conditioned);if(rr>=0)f.reversal(rr);const auto plan=*effects::condition_damage(skill,rank);check(f.cast(skill,rank));
  const auto primary=expected(hp,plan.damage,rr);check(std::abs(f.hp()-primary)<1.01&&!f.has(2,307));
  check(f.has(2,828)==(skill==828&&conditioned&&primary>0));
  for(auto kind:{combat::conditions::Kind::poison,combat::conditions::Kind::weakness})check(f.w.has_condition(f.target,kind,101)==(skill==864&&conditioned&&primary>0));
  if(skill==864){const auto side=expected(500,plan.damage,rr);check(std::abs(f.hp(3)-side)<1.01&&!f.has(3,307));for(auto kind:{combat::conditions::Kind::poison,combat::conditions::Kind::weakness})check(f.w.has_condition(*f.w.effect_scope(3),kind,101)==(conditioned&&side>0));}
  else check(f.hp(3)==500&&f.has(3,307)==(rr>=0));
  check(f.hp(4)==500&&f.has(4,307)==(rr>=0));
 }
 // Conversion-healing death skips conditional hex/conditions.
 for(unsigned skill:{828u,864u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(10);check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));f.reversal(rr);check(f.cast(skill));
  check(f.hp()==0&&!f.has(2,307)&&!f.has(2,828));for(auto kind:{combat::conditions::Kind::poison,combat::conditions::Kind::weakness})check(!f.w.has_condition(f.target,kind,101));
 }
 for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f;const auto side=*f.w.effect_scope(3);check(f.w.apply_condition(f.source,side,combat::conditions::Kind::deep_wound,100,10000));f.w.set_vitals(3,{10,1000,100,100},100);f.reversal(rr);check(f.cast(864));check(f.hp(3)==0&&!f.has(3,307));
 }
 // Hex Breaker suppresses Vile's conditional hex, never its initial cold hit.
 for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(hp);f.reversal(rr);check(f.w.hex_breaker(f.target,*effects::hex_breaker(12),20,100));check(f.cast(828));check(std::abs(f.hp()-expected(hp,54,rr))<1.01&&!f.has(2,828)&&!f.has(2,307));
 }
 // Renewal copies only current authorities; it cannot restore consumed307.
 for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f;f.reversal(rr);check(f.cast(828)&&!f.has(2,307));const auto first=unsigned(f.hp());f.reversal(rr,2,101);check(f.cast(828));check(std::abs(f.hp()-expected(first,54,rr))<1.01&&!f.has(2,307));
  const auto entries=*f.w.status_snapshot(f.target,101);check(std::count_if(entries.begin(),entries.end(),[](const auto& e){return e.skill==828;})==1);
 }
 // Stable player-ID order: primary death's Bile consumes side307 first;
 // Oppressive then deals26 to its CURRENT525Health and adds conditions.
 {++cases;Fixture f(10);check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(2058,0),100));f.reversal(12,3);check(f.cast(864));check(f.hp()==0&&f.hp(3)==499&&!f.has(3,307));check(f.w.has_condition(*f.w.effect_scope(3),combat::conditions::Kind::poison,101));}
 check(cases==166);std::cout<<"PASS "<<cases<<" Vile Miasma/Oppressive Gaze player conversion/current effects/survival/area/renewal cases;NPC coverage in sibling suite,307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
