#include "gw_weapon_attack_skills.h"
#include "gw_character_skills.h"
#include "gw_hex_copy_session.h"
#include <stdexcept>
using namespace fw::gw;
static void check(bool v){if(!v)throw std::runtime_error("PvP dagger typed alias");}
int main(){
 using namespace combat::weapon;
 for(unsigned rank=0;rank<=20;++rank)for(unsigned id:{775u,776u,777u,778u,779u,780u,782u,783u,3061u,3251u}){
  const auto identity=dagger_identity(id);check(bool(identity));const auto plan=attack_skill(id,rank,0,0,true);
  check(plan&&plan->skill==id&&plan->required_type==32&&valid(*plan,true));check(!attack_skill(id,rank)&&!valid(*plan));
  if(id==3061)check(plan->bonus==skills::effects::interpolate(20,45,rank));
  if(id==3251)check(plan->bonus==skills::effects::interpolate(10,25,rank)&&plan->windup_ms==250);
  if(id==780)check(plan->bonus==skills::effects::interpolate(10,35,rank));
  auto forged=*plan;++forged.bonus;check(!valid(forged,true));
 }
 for(unsigned id:{0u,781u,3060u,3252u,3433u})check(!dagger_identity(id));
 check(dagger_role(3061)==775&&dagger_role(3251)==780&&dagger_dual_role(3061)&&!dagger_dual_role(3251));
 check(dagger_mode_allowed(3061,true)&&dagger_mode_allowed(3251,true));
 check(!dagger_mode_allowed(3061,false)&&!dagger_mode_allowed(3251,false));
 check(!dagger_mode_allowed(775,true)&&!dagger_mode_allowed(780,true));
 for(unsigned id:{776u,777u,778u,779u,782u,783u})check(dagger_mode_allowed(id,true)&&dagger_mode_allowed(id,false));
 check(skills::find_skill(3061)->recharge==6&&skills::find_skill(3251)->recharge==6);
 skills::Profile saved;saved.primary=7;saved.learned={{775,1},{780,1},{782,1}};saved.bar[0]={775,0};saved.bar[1]={780,0};saved.bar[2]={782,0};
 const auto match=skills::match_profile(saved,true);check(match.bar[0].skill==3061&&match.bar[1].skill==3251&&match.bar[2].skill==782);
 check(match.learned.at(3061)==1&&match.learned.at(3251)==1&&saved.learned.count(3061)==0&&saved.bar[0].skill==775);
 const auto policy=skills::policy(match,255,false,true);const skills::effects::Scope owner{1,25,1};
 skills::effects::HexCopySession session(owner,match.bar);
 const auto death=session.admit(owner,policy,0,{3061,0},1000),fox=session.admit(owner,policy,1,{3251,0},1000);
 check(death&&fox&&!death->cast.copied&&!fox->cast.copied&&session.valid(*death,policy,1000)&&session.valid(*fox,policy,1000));
 check(!session.admit(owner,policy,0,{775,0},1000)&&!session.admit(owner,policy,1,{780,0},1000));
 check(!session.admit(owner,policy,0,{3061,1},1000));
 auto forged=match.bar;forged[0]={3061,1};check(!session.replace_base(owner,forged,policy,1000));
}
