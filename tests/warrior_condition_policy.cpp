#include "gw_weapon_attack_skills.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw::combat;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("warrior condition policy line "+std::to_string(at.line()));}
int main()try{
 using namespace weapon;using enum conditions::Kind;
 for(unsigned rank=0;rank<=20;++rank)for(unsigned id:{320u,334u,342u,352u,849u,850u,1416u}){
  const auto p=attack_skill(id,rank,0);check(p&&valid(*p));
  check(p->required_type==((id==320||id==1416)?27u:(id==352||id==850)?15u:2u));
  const auto expected_bonus=id==320?0u:id==334?unsigned(std::lround(1.+9.*rank/15.)):(id==342||id==352)?unsigned(std::lround(1.+19.*rank/15.)):id==1416?unsigned(std::lround(5.+25.*rank/15.)):5+rank;
  check(p->bonus==expected_bonus);
  const auto expected_seconds=id==320?unsigned(std::lround(3.+12.*rank/15.)):id==334?15u:id==342?20u:id==850?unsigned(std::lround(1.+7.*rank/15.)):id==1416?unsigned(std::lround(5.+10.*rank/15.)):5+rank;
  check(condition_duration(*p)==expected_seconds*1000);
  for(bool enabled:{false,true}){
   ConditionContact contact{enabled,enabled,enabled,!enabled};
   const auto kind=attack_condition(*p,false,contact);
   check(bool(kind)==(id==320||enabled));
   if(kind)check(*kind==((id==320||id==334)?crippled:id==342?weakness:(id==849||id==1416)?bleeding:deep_wound));
   Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;
   auto hit=resolve_attack_skill(strike,*p,false,false,false,false,contact);
   check(hit&&!hit->missed&&hit->damage==20+((id!=334&&id!=342)||enabled?expected_bonus:0));
   strike.weakness=true;hit=resolve_attack_skill(strike,*p,false,false,false,false,contact);
   check(hit&&std::abs(hit->damage-(6.8+((id!=334&&id!=342)||enabled?expected_bonus:0)))<1e-8);
   strike.blind=true;check(resolve_attack_skill(strike,*p,false,false,false,false,contact)->missed);
  }
  auto forged=*p;++forged.bonus;check(!valid(forged)&&!attack_condition(forged,false,{true,true,true,false}));
 }
 check(!attack_skill(320,21));
 // Retain existing Sever Artery and Gash behavior through the shared selector.
 check(attack_condition(*attack_skill(382,12),false)==bleeding);
 check(!attack_condition(*attack_skill(384,12),false));check(attack_condition(*attack_skill(384,12),true)==deep_wound);
 std::cout<<"PASS seven warrior condition attacks ranks0..20,weapon requirements,base/bonus separation,contact predicates,durations,Blind and forged plans\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
