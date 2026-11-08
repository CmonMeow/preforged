#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Armor Earth damage line "+std::to_string(at.line()));}
int main()try{
 for(bool cracked:{false,true})for(unsigned kind=0;kind<9;++kind){world::Instance w;w.join(1,"Attacker",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Earth",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);for(unsigned id:{1u,2u})w.set_vitals(id,{10000,10000,100,100},100);const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  check(w.physical_armor(b,60)&&w.combat_armor(b,60)&&w.elemental_armor(b,60,60)&&w.lightning_armor(b,60));check(w.armor_earth(b,12,100));
  if(cracked)check(w.apply_condition(a,b,combat::conditions::Kind::cracked_armor,100,10000));
  const int armor=cracked?93:113;double expected=0;
  if(kind==0){auto plan=*caster_earth_aoe(174,0);check(w.shockwave(a,plan,20,200));expected=26*std::exp2((60.-armor)/40.);}
  if(kind==1){auto plan=*lightning(224,0);check(w.lightning(a,b,plan,20,200));expected=25*std::exp2((60.-armor*.75)/40.);}
  if(kind==2||kind==3){auto plan=*elemental(kind==2?191:846,0);check(w.elemental(a,b,plan,20,200));expected=20*std::exp2((60.-armor)/40.);}
  if(kind==4){combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(a,sword,20));const auto plan=*combat::weapon::attack_skill(322,0);check(w.melee_attack(a,b,sword,plan,200));combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;strike.location_armor=armor;strike.base_roll=.5;strike.miss_roll=.5;expected=combat::weapon::resolve_attack_skill(strike,plan,false)->damage;}
  if(kind==5){const auto plan=*earth_projectile(1374,0,0);check(w.launch_projectile(a,2,plan,20,200));w.tick(500);expected=plan.earth_damage*std::exp2((60.-armor)/40.);}
  if(kind==6){const auto plan=*lightning_javelin(0);check(w.lightning_javelin(a,b,plan,20,166,200));w.tick(500);expected=plan.damage*std::exp2((60.-armor*.75)/40.);}
  if(kind==7||kind==8){const auto plan=*condition_damage(kind==7?828:864,0);check(w.condition_damage(a,b,plan,20,200));expected=plan.damage*(kind==8?1.:std::exp2((60.-armor)/40.));}
  const auto actual=w.resources(2)->health;const auto wanted=unsigned(std::ceil(10000.-expected));if(actual!=wanted)throw std::runtime_error("kind="+std::to_string(kind)+" cracked="+std::to_string(cracked)+" hp="+std::to_string(actual)+" expected="+std::to_string(wanted));
 }
 {
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Wounded",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{1000,1000,100,100},100);w.set_vitals(2,{50,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.elemental_armor(target,400,400));
  check(w.apply_condition(source,target,combat::conditions::Kind::deep_wound,100,5000));
  check(w.resources(2)->health==1&&w.resources(2)->max_health==900);
  box->drain();check(w.elemental(source,target,*elemental(191,0),20,100));
  check(w.resources(2)->health==0);
  const auto packets=box->drain();const double tiny=20*std::exp2((60.-400)/40.);
  check(tiny>0&&tiny<1);
  // Pending wound debt is settled by this event. The damage packet must
  // report that actual death, not the raw fractional subtraction from1HP.
  const auto stale=combat::detail::fraction(2,{1-tiny,900});
  check(std::count(packets.begin(),packets.end(),stale)==0);
  check(std::count(packets.begin(),packets.end(),combat::detail::fraction(2,{0,900}))>=1);
 }
 std::cout<<"PASS Armor Earth actual earth/lightning/fire/cold/physical/projectile/conditional/armor-ignoring paths, single bonus and Cracked Armor after bonus\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
