#include "gw_world_simulation.h"
#include "gw_hex_copy_session.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Hex copy session line "+std::to_string(at.line()));}
int main()try{using namespace skills;using namespace effects;
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(3,"Enemy",90,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{500,500,20,100},100);for(unsigned id:{2u,3u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),ally=*w.effect_scope(2),enemy=*w.effect_scope(3);
  Slots saved{};saved[0]={22,0};const auto untouched=saved;HexCopySession session(caster,saved);
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[22]=1;
  check(w.backfire(enemy,ally,*backfire(28,12),20,100));
  const auto offer=w.prepare_inspired_hex(caster,ally,*inspired_hex(22,12),200);check(bool(offer));
  auto prepared=session.prepare(*offer,0,{22,0},{28,0});check(prepared&&session.ready(*prepared)&&session.bar()==saved);
  if(mode==1){check(w.wound_spell(enemy,ally,*wound_spell(44,12),20,200));check(!w.commit_inspired_hex(prepared->offer()));check(session.bar()==saved&&w.resources(1)->energy==20);continue;}
  if(mode==2){check(bool(session.expire(caster,201)));check(!session.ready(*prepared));check(session.bar()==saved);continue;}
  const auto result=w.commit_inspired_hex(prepared->offer());check(bool(result));
  if(mode==3){auto foreign=*result;foreign.ally.actor=9;check(!session.commit(std::move(*prepared),foreign)&&session.bar()==saved);}
  check(session.commit(std::move(*prepared),*result));check(session.bar()[0]==Slot{28,0}&&session.base()==saved&&saved==untouched&&!policy.learned.contains(28));
  const auto cast=session.admit(caster,policy,0,{28,0},200);check(bool(cast));
  check(session.valid(*cast,policy,20199)&&!session.valid(*cast,policy,20200));
  if(mode==3){
   check(bool(session.replace_base(caster,saved,policy,200)));check(session.bar()==saved);
   check(w.backfire(enemy,ally,*backfire(28,12),20,200));
   const auto next=w.prepare_inspired_hex(caster,ally,*inspired_hex(22,12),200);check(bool(next));
   auto p=session.prepare(*next,0,{22,0},{28,0});check(bool(p));const auto r=w.commit_inspired_hex(p->offer());check(r&&session.commit(std::move(*p),*r));
   check(!session.valid(*cast,policy,200)); // Same pair/time/generation still cannot cross base replacement.
  }
  const auto expired=session.expire(caster,20200);check(expired&&expired->size()==1&&session.bar()==saved);
 }
 std::cout<<"PASS World-to-session prepared copy commit, matched result identities, no saved profile/learning mutation, stale plan rejection,20second restoration and base-epoch isolation\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
