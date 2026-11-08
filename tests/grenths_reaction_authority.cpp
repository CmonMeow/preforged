#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Grenth reaction authority line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Enemy",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{mode==1?500u:20u,500,100,100},100);w.set_vitals(2,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),enemy=*w.effect_scope(2);
  if(mode==3){check(w.kill(caster,150));check(!w.before_spell_effect(caster,86,7,200));}
  else{check(w.backfire(enemy,caster,*skills::effects::backfire(28,12),20,100));check(w.before_spell_effect(caster,mode==2?28:86,7,200)==(mode==1));}
  if(mode==4){check(w.resurrect(caster,300));check(w.kill(caster,400));}
  if(mode){check(!w.take_grenths_reaction_death(caster,7,200));continue;}
  auto stale=caster;++stale.generation;
  check(!w.take_grenths_reaction_death(stale,7,200));
  check(!w.take_grenths_reaction_death(caster,8,200)&&!w.take_grenths_reaction_death(caster,7,201));
  check(bool(w.take_grenths_reaction_death(caster,7,200)));
  check(!w.take_grenths_reaction_death(caster,7,200));
 }
 std::cout<<"PASS Grenth reaction-death authorization is scoped/event/time matched and one-use; manual death, other spell, survival and stale requests rejected\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
