#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Grenth World regression line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<8;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Foe",mode==3?2000.f:60.f,0,box,0,{}, {},100,mode==4?1:2);
  w.set_vitals(1,{mode==1?500u:100u,500,100,100},100);w.set_vitals(2,{mode==1?100u:500u,500,100,100},100);
  auto caster=*w.effect_scope(1),foe=*w.effect_scope(2);
  if(mode==2)++foe.generation;if(mode==5||mode==6)check(w.kill(caster,100));if(mode==7)check(w.kill(foe,100));
  box->drain();const bool accepted=w.grenths_balance(caster,foe,200,mode==6);
  check(accepted==(mode==0||mode==1||mode==6));
  if(mode==0||mode==1)check(w.resources(1)->health==300&&w.resources(2)->health==300);
  if(mode==6)check(w.resources(1)->health==0&&w.resources(2)->health==250);
  if(accepted){const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(2,1,86))==1);}
 }
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);
  w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{mode==1?300u:100u,500,100,100},100);w.set_vitals(2,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),foe=*w.effect_scope(2);
  if(mode==0)check(w.degeneration_hex(foe,caster,*skills::effects::degeneration_hex(129,12),100));
  else check(w.apply_condition(caster,mode==3?foe:caster,combat::conditions::Kind::deep_wound,100,60000));
  box->drain();check(w.grenths_balance(caster,foe,200));
  const unsigned caster_hp=mode==0?300:mode==1?350:250;
  const unsigned foe_hp=mode==0?300:mode==1?350:mode==2?251:250;
  check(w.resources(1)->health==caster_hp&&w.resources(2)->health==foe_hp);
  // Fractions publish precise reconciled Health, including the half point.
  const double precise_caster=mode==2?249.5:caster_hp,precise_foe=mode==2?250.5:foe_hp;
  const auto caster_fraction=combat::detail::fraction(1,{precise_caster,w.resources(1)->max_health});
  const auto foe_fraction=combat::detail::fraction(2,{precise_foe,w.resources(2)->max_health});
  const auto packets=box->drain();check(std::find(packets.begin(),packets.end(),caster_fraction)!=packets.end());
  check(std::find(packets.begin(),packets.end(),foe_fraction)!=packets.end());
 }
 std::cout<<"PASS Grenth World scope/eligibility, forward/reverse/reaction-death, body164, healing-modifier bypass and Deep Wound reconciled fractions; dispatcher/NPC pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
