#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Diversion prepared World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Victim",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2);
  check(w.diversion(caster,owner,*skills::effects::diversion(12),100));
  const auto old=w.status_snapshot(owner,100)->front();
  check(w.prepare_diversion_skill_use(owner,{67,0},7,200));
  check(!w.prepare_diversion_skill_use(owner,{67,0},8,200));
  if(mode==2){check(!w.abort_diversion_skill_use(owner,8));check(w.abort_diversion_skill_use(owner,7));check(!w.finalize_diversion_skill_use(owner,7,200,10000,0));check(w.status_snapshot(owner,200)->size()==1);continue;}
  if(mode==3){check(w.kill(owner,200));check(w.resurrect(owner,300));check(!w.finalize_diversion_skill_use(owner,7,200,10000,0));check(w.prepare_diversion_skill_use(owner,{67,0},8,300));check(w.abort_diversion_skill_use(owner,8));continue;}
  if(mode<2){check(w.status_mutation(owner,30,old.instance,12,200,{},false));
   if(mode==1)check(w.diversion(caster,owner,*skills::effects::diversion(0),200));
  }
  if(mode==4){check(!w.finalize_diversion_skill_use(owner,8,200,10000,0));check(!w.finalize_diversion_skill_use(owner,7,201,10000,0));}
  check(w.finalize_diversion_skill_use(owner,7,200,10000,0,mode==5)==(mode==5?0:57200));
  check(w.diversion_recharge_until(owner,{67,0},200)==(mode==5?0:57200));
  const auto completed=w.cooldown_completion_stamp(owner,7);check(completed&&completed->instant_recharge==(mode==5));
  check(skills::effects::completion_deadline(*completed)==(mode==5?200:10200));
  const auto effects=*w.status_snapshot(owner,200);
  check(effects.size()==(mode==1?1:0));if(mode==1)check(effects.front().rank==0);
  check(!w.finalize_diversion_skill_use(owner,7,200,10000,0));check(!w.prepare_diversion_skill_use(owner,{67,0},7,200));
 }
 std::cout<<"PASS World-owned Diversion preparation/finalization survives removal, preserves replacement, aborts safely, rejects wrong event/time/replay and clears on death\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
