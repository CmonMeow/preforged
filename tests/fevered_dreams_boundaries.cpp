#include "gw_world_simulation.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;using Kind=combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Fevered boundary line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<9;++mode){world::Instance w;
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Actor",id==1?0:id==2?60:id==3?120:350,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);check(w.combat_armor(*w.effect_scope(id),60));}
  const auto c=*w.effect_scope(1),a=*w.effect_scope(2),b=*w.effect_scope(3),far=*w.effect_scope(4);
  if(mode==4)check(w.apply_condition(c,a,Kind::blind,100,10000));
  check(w.apply_fevered_dreams(c,a,12,false,100,FeveredDazedSpan::attempted_duration));
  if(mode==4){check(w.has_condition(far,Kind::dazed,100)&&!w.has_condition(far,Kind::blind,100));continue;}
  if(mode==0){check(w.apply_condition(c,a,Kind::blind,200,10000));check(w.has_condition(far,Kind::blind,200));}
  if(mode==1){check(w.apply_condition(c,c,Kind::blind,100,10000)&&w.apply_condition(c,c,Kind::poison,100,10000));check(w.condition_transfer(c,a,*condition_transfer(154,12),200));check(w.has_condition(far,Kind::poison,200)&&w.has_condition(far,Kind::dazed,200));}
  if(mode==2){check(w.blindness(c,a,*blindness(220,12),200));check(w.has_condition(far,Kind::blind,200));}
  if(mode==3){check(w.shockwave(c,*caster_earth_aoe(937,0),20,200));
   for(auto target:{a,b,far})for(auto kind:{Kind::weakness,Kind::cracked_armor,Kind::blind,Kind::dazed})check(w.has_condition(target,kind,200));
   check(w.resources(4)->health==500&&w.resources(3)->health==455); // primary damage committed once, before spread
  }
  if(mode==5){combat::weapon::Owned sword{10,1,{10,10,2},12,false,false,27};check(w.ordinary_weapon(c,sword,20)&&w.physical_armor(a,60));check(w.melee_attack(c,a,sword,*combat::weapon::attack_skill(382,12),200));check(w.has_condition(far,Kind::bleeding,200));}
  if(mode==6){combat::weapon::Owned bow{7,1,{10,10,1},12,false,false,5};check(w.ordinary_weapon(c,bow,20)&&w.physical_armor(a,60));check(w.melee_attack(c,a,bow,*combat::weapon::attack_skill(404,0),200));check(!w.has_condition(far,Kind::poison,200));w.tick(300);check(w.has_condition(a,Kind::poison,300)&&w.has_condition(far,Kind::poison,300));}
  if(mode==7){check(w.elemental_armor(a,0,60));check(w.elemental(c,a,*elemental(191,0),20,200));check(w.has_condition(far,Kind::burning,200));}
  if(mode==8){check(w.lightning_armor(a,60));check(w.lightning(c,a,*lightning(224,0),20,200));check(w.has_condition(far,Kind::weakness,200));}
  check(w.fevered_boundary_failures()==0);
 }
 std::cout<<"PASS automatic complete-batch Fevered boundaries for direct, transfer, blindness, Shockwave and initial application without manual flush; no later-primary overwrite\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
