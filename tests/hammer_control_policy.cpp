#include "gw_weapon_attack_skills.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw::combat;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("hammer control policy line "+std::to_string(at.line()));}
int main()try{using namespace weapon;
 for(unsigned id:{355u,357u,358u,359u,360u})for(unsigned rank=0;rank<=20;++rank)for(unsigned strength:{0u,7u,8u,20u}){
  const auto plan=*attack_skill(id,rank,strength);check(valid(plan)&&plan.required_type==15);
  check(plan.windup_ms==(id==360?1000u:500u));
  const unsigned bonus=id==358?unsigned(std::lround(1.+19.*rank/15.)):id==359?unsigned(std::lround(1.+29.*rank/15.)):0u;
  check(plan.bonus==bonus&&condition_duration(plan)==((id==355||id==360)?(5+rank)*1000:0));
  for(bool start:{false,true})for(bool contact:{false,true})for(bool attacking:{false,true}){
   ConditionContact facts{false,contact,false,false,start};
   check(attack_knockdown(plan,attacking,facts)==(id==360||(id==357&&!attacking)||(id==359&&!contact)?0u:id==358&&strength>=8?4000u:2000u));
   const auto kind=attack_condition(plan,false,facts);check(bool(kind)==(id==355||id==360));if(kind)check(*kind==conditions::Kind::weakness);
   Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;
   const auto hit=resolve_attack_skill(strike,plan,attacking,false,false,false,facts);
   check(hit&&!hit->missed&&std::abs(hit->damage-(20*std::exp2(60.*strength*.01/40.)+(id!=359||start?bonus:0)))<1e-8);
   strike.blind=true;check(resolve_attack_skill(strike,plan,attacking,false,false,false,facts)->missed);
  }
  auto forged=plan;forged.knockdown_on_hit=!forged.knockdown_on_hit;check(!valid(forged)&&!attack_knockdown(forged,true,{false,true,false,false,true}));
 }
 Strike strike;check(!resolve_attack_skill(strike,*attack_skill(359,12),false)); // no invented start fact
 check(!attack_skill(355,21));
 std::cout<<"PASS five hammer controls,ranks0..20,Strength threshold,start/contact Weakness separation,conditional knockdown and fixed Staggering activation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
