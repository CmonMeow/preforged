#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Frigid Armor damage line "+std::to_string(at.line()));}
int main()try{
 for(bool cracked:{false,true})for(unsigned kind=0;kind<14;++kind){world::Instance w;w.join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Earth",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);for(unsigned id:{1u,2u})w.set_vitals(id,{10000,10000,100,100},100);const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  check(w.physical_armor(b,60)&&w.combat_armor(b,60)&&w.elemental_armor(b,60,60)&&w.lightning_armor(b,60));check(w.status_mutation(b,1261,69,12,100,22100,true));
  if(cracked&&(kind==4||kind==9))check(w.apply_condition(a,b,combat::conditions::Kind::cracked_armor,100,10000));
  const int armor=(kind==4||kind==9)?(cracked?74:94):60;double expected=0;
  if(kind==0){auto plan=*caster_earth_aoe(174,0);check(w.shockwave(a,plan,20,200));expected=26*std::exp2((60.-armor)/40.);}
  if(kind==1){auto plan=*lightning(224,0);check(w.lightning(a,b,plan,20,200));expected=25*std::exp2((60.-armor*.75)/40.);}
  if(kind==2||kind==3){auto plan=*elemental(kind==2?191:846,0);check(w.elemental(a,b,plan,20,200));expected=20*std::exp2((60.-armor)/40.);}
  if(kind==4){combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(a,sword,20));const auto plan=*combat::weapon::attack_skill(322,0);check(w.melee_attack(a,b,sword,plan,200));combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;strike.location_armor=armor;strike.base_roll=.5;strike.miss_roll=.5;expected=combat::weapon::resolve_attack_skill(strike,plan,false)->damage;}
  if(kind==5){const auto plan=*earth_projectile(1374,0,0);check(w.launch_projectile(a,2,plan,20,200));w.tick(500);expected=plan.earth_damage*std::exp2((60.-armor)/40.);}
  if(kind==6){const auto plan=*lightning_javelin(0);check(w.lightning_javelin(a,b,plan,20,166,200));w.tick(500);expected=plan.damage*std::exp2((60.-armor*.75)/40.);}
  if(kind==7||kind==8){const auto plan=*condition_damage(kind==7?828:864,0);check(w.condition_damage(a,b,plan,20,200));expected=plan.damage*(kind==8?1.:std::exp2((60.-armor)/40.));}
  if(kind==9){check(w.ear_bite(a,b,*ear_bite(10),20,200));expected=70*std::exp2((60.-armor)/40.);}
  if(kind>=10){combat::weapon::Owned weapon{1,1,{20,20,kind-4},12,false,false,27};check(w.ordinary_weapon(a,weapon,20));const auto plan=*combat::weapon::attack_skill(322,0);check(w.melee_attack(a,b,weapon,plan,200));combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;strike.location_armor=60;strike.base_roll=.5;strike.miss_roll=.5;expected=combat::weapon::resolve_attack_skill(strike,plan,false)->damage;}
  const auto actual=w.resources(2)->health;const auto wanted=unsigned(std::ceil(10000.-expected));if(actual!=wanted)throw std::runtime_error("kind="+std::to_string(kind)+" cracked="+std::to_string(cracked)+" hp="+std::to_string(actual)+" expected="+std::to_string(wanted));
 }
 {
 world::Instance w;w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto b=*w.effect_scope(1);
 check(w.apply_condition(b,b,combat::conditions::Kind::burning,100,1000));
 check(!w.status_mutation(b,1261,69,21,100,30100,true));
 check(!w.status_mutation(b,1261,69,0,100,30100,true));
 check(w.status_mutation(b,1261,69,0,100,10100,true));
 check(w.has_condition(b,combat::conditions::Kind::burning,100));
 check(w.apply_condition(b,b,combat::conditions::Kind::burning,200,20000));
 check(w.apply_condition(b,b,combat::conditions::Kind::blind,200,20000));
 w.tick(1100);check(!w.has_condition(b,combat::conditions::Kind::burning,1100));check(w.has_condition(b,combat::conditions::Kind::blind,1100));
 check(w.apply_condition(b,b,combat::conditions::Kind::burning,1200,10000));check(!w.has_condition(b,combat::conditions::Kind::burning,1200));
 check(w.status_mutation(b,1261,69,0,1300,{},false));
 check(w.apply_condition(b,b,combat::conditions::Kind::burning,1400,1000));check(w.has_condition(b,combat::conditions::Kind::burning,1400));
 }
 {
 world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
 check(w.apply_condition(a,a,combat::conditions::Kind::burning,100,10000));check(w.status_mutation(b,1261,69,0,100,10100,true));
 check(!w.condition_transfer(a,b,*condition_transfer(154,0),200));check(w.has_condition(a,combat::conditions::Kind::burning,200));check(!w.has_condition(b,combat::conditions::Kind::burning,200));
 }
 std::cout<<"PASS Frigid Armor player physical-only armor across damage paths; Burning admission preserves existing Burning and unrelated conditions\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
