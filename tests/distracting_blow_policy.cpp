#include "gw_distracting_blow.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fw::gw;using namespace combat::weapon;
void check(bool b){if(!b)throw std::runtime_error("Distracting Blow policy");}
int main()try{
 const auto p=distracting_blow();check(p&&p->energy==5&&p->recharge_ms==10000&&p->windup==500&&p->zero_weapon_damage);
 const skills::effects::Scope source{1,1,1},primary{1,2,1};
 const AttackActionFacts spell{AttackTargetAction::skill,skills::Slot{281,0}};
 std::vector<BlowRecipient> c{{primary,100,100,true,spell},{{1,3,1},266,100,true,{AttackTargetAction::attack,{}}},
 {{1,4,1},266.001,100,true,spell},{{1,5,1},100,100,false,spell},{{1,6,1},100,100,true,{}},
 {source,100,100,true,spell}};
 auto selected=distracting_blow_recipients(*p,source,primary,c);check(selected&&selected->size()==3);
 check((*selected)[0].scope==primary&&(*selected)[0].attempt&&(*selected)[1].scope.actor==3&&(*selected)[1].attempt&&(*selected)[2].scope.actor==6&&!(*selected)[2].attempt);
 auto bad=c;bad[1].action={AttackTargetAction::skill,{}};check(!distracting_blow_recipients(*p,source,primary,bad));
 bad=c;bad[0].eligible=false;check(!distracting_blow_recipients(*p,source,primary,bad));
 bad=c;bad[1].scope=primary;check(!distracting_blow_recipients(*p,source,primary,bad));
 bad=c;bad[1].scope.actor=2;bad[1].scope.generation=2;check(!distracting_blow_recipients(*p,source,primary,bad));
 bad=c;bad[1].x=std::numeric_limits<double>::quiet_NaN();check(!distracting_blow_recipients(*p,source,primary,bad));
 auto forged=*p;forged.zero_weapon_damage=false;check(!distracting_blow_recipients(forged,source,primary,c));
 check(!distracting_blow_recipients(*p,source,source,c));check(!distracting_blow_recipients(*p,source,{1,2,2},c));
 const std::vector<skills::effects::Scope> single{primary},two{primary,{1,3,1}},three{primary,{1,3,1},{1,6,1}};
 for(unsigned type:{2u,15u,27u,32u,35u})check(distracting_blow_contacts(*p,source,primary,type,c,single));
 for(unsigned type:{2u,15u,27u,32u})check(!distracting_blow_contacts(*p,source,primary,type,c,two));
 check(distracting_blow_contacts(*p,source,primary,35,c,two)&&distracting_blow_contacts(*p,source,primary,35,c,three));
 for(unsigned type:{0u,5u,22u,26u,36u,99u})check(!distracting_blow_contacts(*p,source,primary,type,c,single));
 check(!distracting_blow_contacts(*p,source,primary,35,c,{}));
 for(const auto& invalid:std::vector<std::vector<skills::effects::Scope>>{
     {{1,3,1},primary},{primary,primary},{primary,{1,3,2}},{primary,{1,4,1}},
     {primary,{1,5,1}},{primary,source},{primary,{1,3,1},{1,6,1},{1,4,1}}})
  check(!distracting_blow_contacts(*p,source,primary,35,c,invalid));
 std::cout<<"PASS staged Distracting Blow pinned5Energy/500ms/10s/zero damage,primary-centered166 area,eligibility,scope/generation/duplicate/finite checks and all-action preflight;World/Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
