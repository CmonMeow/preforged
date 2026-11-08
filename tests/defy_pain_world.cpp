#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Defy World line "+std::to_string(at.line()));}
struct Fixture{world::Instance w;Scope owner,foe;Fixture(){for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}owner=*w.effect_scope(1);foe=*w.effect_scope(2);check(w.lightning_armor(owner,60)&&w.physical_armor(owner,60));}};
int main()try{
 for(unsigned rank=0;rank<=20;++rank){Fixture f;const auto plan=*defy_pain(rank);check(f.w.stage_defy_pain_health(f.owner,plan,100));
  check(f.w.resources(1)->max_health==1000+plan.health_bonus);auto before=f.w.resources(1)->health;
  check(f.w.lightning(f.foe,f.owner,*lightning(224,12),20,100));const auto loss=before-f.w.resources(1)->health;
  check(std::abs(double(loss)-(45-plan.reduction))<1.1); //80AR with25%penetration gives60effective AR.
  before=f.w.resources(1)->health;check(f.w.life_steal(f.foe,f.owner,*life_steal(153,0),100));check(before-f.w.resources(1)->health==18);
  check(f.w.stage_to_limit_health(f.owner,*to_limit(0),100)&&f.w.stage_endure_pain_health(f.owner,*endure_pain(0),100));
  check(f.w.resources(1)->max_health==1100+plan.health_bonus);check(f.w.stage_defy_pain_health(f.owner,*defy_pain(0),200));check(f.w.resources(1)->max_health==1190);
  check(f.w.settle_resources(f.owner,20300)&&f.w.resources(1)->max_health==1000);
 }
 for(unsigned mode=0;mode<3;++mode){Fixture f;check(f.w.stage_defy_pain_health(f.owner,*defy_pain(12),100));
  if(mode==0){check(f.w.kill(f.owner,200));check(f.w.resources(1)->health==0&&f.w.resources(1)->max_health==1000);}
  if(mode==1){f.w.set_vitals(1,{300,600,100,100},100);check(f.w.resources(1)->max_health==600);}
  if(mode==2){auto stale=f.owner;++stale.generation;check(!f.w.stage_defy_pain_health(stale,*defy_pain(0),100));check(f.w.stage_defy_pain_health(f.owner,*defy_pain(0,true),100));}
 }
 {Fixture f;check(!f.w.status_mutation(f.owner,318,9,0,100,20100,true));check(!f.w.status_mutation(f.owner,3204,9,0,100,20100,true));
  combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,27};check(f.w.ordinary_weapon(f.foe,weapon,20));
  check(f.w.stage_defy_pain_health(f.owner,*defy_pain(12),100));const auto before=f.w.resources(1)->health;
  check(f.w.ordinary_attack(f.foe,1,100));f.w.tick(600);f.w.tick(1000);check(std::abs(double(before-f.w.resources(1)->health)-(20*std::exp2(-20./40.)-8))<1.1);
 }
 {Fixture f;check(f.w.stage_defy_pain_health(f.owner,*defy_pain(12),100));
  check(f.w.stage_watch_yourself_recipient(f.owner,f.owner,*watch_yourself(0,true),100));
  for(unsigned n=0;n<2;++n){const auto before=f.w.resources(1)->health;check(f.w.lightning(f.foe,f.owner,*lightning(224,12),20,100));
   const auto expected=45*std::exp2((60-(n?80.:85.)*.75)/40)-8;check(std::abs(double(before-f.w.resources(1)->health)-expected)<1.1);}
 }
 {Fixture f;check(f.w.stage_defy_pain_health(f.owner,*defy_pain(12),100));auto before=f.w.resources(1)->health;
  check(f.w.shadow_damage(f.foe,f.owner,*shadow_damage(219,0),20,100));check(before-f.w.resources(1)->health==14);
  check(f.w.status_mutation(f.owner,299,1001,12,100,8100,true));before=f.w.resources(1)->health;
  check(f.w.shadow_damage(f.foe,f.owner,*shadow_damage(219,0),20,100));check(before==f.w.resources(1)->health);
 }
 {Fixture f;check(f.w.stage_defy_pain_health(f.owner,*defy_pain(12),100));const auto before=f.w.resources(1)->health;
  check(f.w.degeneration_hex(f.foe,f.owner,*degeneration_hex(31,12),100));f.w.tick(1100);check(before-f.w.resources(1)->health==10);
 }
 std::cout<<"PASS21DefyPain PvE World Health/Lightning mitigation,life-steal exclusion,weapon mitigation,coexistence,refresh,expiry,death/reset/scope guards;PvP and normal casting remain unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
