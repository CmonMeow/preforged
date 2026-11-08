#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ward938 completion line "+std::to_string(at.line())+" "+context);
}
int main()try{unsigned cases=0;
 for(unsigned renewal:{0u,181u,2860u})for(unsigned mode=0;mode<10;++mode){
  context="renewal="+std::to_string(renewal)+" mode="+std::to_string(mode);
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Ward caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,50,100},100);
  const auto owner=*w.effect_scope(1);const auto plan=*defensive_ground_ward_plan(938,0);
  if(renewal){const auto r=*ether_renewal(renewal,0);check(w.status_mutation(owner,renewal,80,0,100,100+r.duration,true));}
  const auto admitted=mode==4&&renewal?100+ether_renewal(renewal,0)->duration:200;
  w.tick(admitted);check(w.before_spell_effect(owner,938,1,admitted));box->drain();
  const auto at=mode==7?admitted+100:admitted;
  const auto before_completion=*w.resources(1);
  std::vector<world::Packet> visuals;
  if(mode==1)check(!w.complete_defensive_ward(owner,plan,2,at,&visuals)&&visuals.empty());
  if(mode==2){auto stale=owner;++stale.generation;check(!w.complete_defensive_ward(stale,plan,1,at,&visuals)&&visuals.empty());}
  if(mode==3){auto forged=plan;++forged.radius;check(!w.complete_defensive_ward(owner,forged,1,at,&visuals)&&visuals.empty());}
  if(mode==8){w.tick(at+100);check(!w.complete_defensive_ward(owner,plan,1,at,&visuals)&&visuals.empty());check(!w.ward_knockdown_protection(owner,at+100));++cases;continue;}
  if(mode==9)check(!w.complete_defensive_ward(owner,plan,0,at,&visuals)&&visuals.empty());
  if(mode==6){check(w.kill(owner,at));check(!w.complete_defensive_ward(owner,plan,1,at,&visuals)&&visuals.empty());check(!w.ward_knockdown_protection(owner,at));++cases;continue;}
  check(w.complete_defensive_ward(owner,plan,1,at,&visuals));
  check(visuals.size()==2&&visuals[0]==skills::particles::ward_stability_ground_2022(0,0,0));
  const bool rewarded=renewal&&mode!=4;
  check(w.resources(1)->health==before_completion.health+(rewarded?5:0)&&w.resources(1)->energy==before_completion.energy+(rewarded?1:0));
  check(w.ward_knockdown_protection(owner,at));
  const auto statuses=*w.status_snapshot(owner,at);
  check(std::none_of(statuses.begin(),statuses.end(),[](const auto& e){return e.skill==938;})); // Field is not an enchantment.
  check(!w.complete_defensive_ward(owner,plan,1,at,&visuals)&&visuals.empty());
  if(mode==5){check(w.before_spell_effect(owner,938,2,at+100));const auto before=*w.resources(1);
   check(w.complete_defensive_ward(owner,plan,2,at+100,&visuals));
   check(w.resources(1)->health==before.health+(renewal?5:0)&&w.resources(1)->energy==before.energy+(renewal?1:0));
  }
  ++cases;
 }
 for(unsigned renewal:{181u,2860u}){context="reward death";world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Ward caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{50,1000,50,100},100);const auto owner=*w.effect_scope(1);
  const auto r=*ether_renewal(renewal,0);check(w.status_mutation(owner,renewal,80,0,100,100+r.duration,true));
  check(w.apply_condition(owner,owner,combat::conditions::Kind::deep_wound,100,60000));
  check(w.before_spell_effect(owner,938,1,200));std::vector<world::Packet> visuals;
  check(w.complete_defensive_ward(owner,*defensive_ground_ward_plan(938,0),1,200,&visuals));
  check(w.resources(1)->health==0&&visuals.empty()&&!w.ward_knockdown_protection(owner,200));
  check(!w.complete_defensive_ward(owner,*defensive_ground_ward_plan(938,0),1,200,&visuals));++cases;
 }
 std::cout<<cases<<" Ward938 admitted completion, Renewal, event/lifetime/rank/timeline rejection, recast and reward-death cases passed; Runtime remains unbound\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
