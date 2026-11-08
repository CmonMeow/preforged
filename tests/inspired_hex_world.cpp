#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Inspired World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{22u,1049u})for(unsigned mode=0;mode<7;++mode){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Ally",mode==4?2000.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,mode==5?2:1);
  w.join(3,"Enemy",90,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{500,500,20,100},100);for(unsigned id:{2u,3u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),ally=*w.effect_scope(2),enemy=*w.effect_scope(3);auto selected=ally;
  if(mode!=1&&mode!=4&&mode!=5)check(w.wound_spell(enemy,ally,*skills::effects::wound_spell(44,12),20,100));
  if(mode==6)++selected.generation;
  const auto offer=w.prepare_inspired_hex(caster,selected,*skills::effects::inspired_hex(skill,12),200);
  if(mode>=4){check(!offer&&w.resources(1)->energy==20);continue;}
  check(bool(offer));
  if(mode==2){check(w.backfire(enemy,ally,*skills::effects::backfire(28,12),20,200));check(!w.commit_inspired_hex(*offer));check(w.resources(1)->energy==20);continue;}
  if(mode==3){check(w.kill(caster,200));check(!w.commit_inspired_hex(*offer));continue;}
  const auto result=w.commit_inspired_hex(*offer);check(bool(result));
  check(w.resources(1)->energy==(mode==1?20u:29u));
  check(result->energy_restored==(mode==1?0u:9u));
  if(mode==1)check(!result->removed);
  else{check(result->removed&&result->removed->skill==44&&result->copy.kind==skills::effects::HexCopyKind::player);
   check(w.has_condition(ally,combat::conditions::Kind::deep_wound,200));check(!w.commit_inspired_hex(*offer));
  }
 }
 for(unsigned skill:{22u,1049u})for(unsigned mode=0;mode<4;++mode){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(3,"Enemy",90,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  const unsigned initial=mode==0?60:mode==1?75:mode==2?98:100;
  w.set_vitals(1,{500,500,initial,100},100);for(unsigned id:{2u,3u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),ally=*w.effect_scope(2),enemy=*w.effect_scope(3);
  check(w.energy_capacity(1,5,0,100));check(w.resources(1)->max_energy==20);
  check(w.backfire(enemy,ally,*skills::effects::backfire(28,12),20,100));
  const auto offer=w.prepare_inspired_hex(caster,ally,*skills::effects::inspired_hex(skill,12),100);check(bool(offer));
  const auto result=w.commit_inspired_hex(*offer);check(result&&result->removed);
  check(result->energy_repaid==(mode==0?9u:mode==1?5u:0u));
  check(result->energy_restored==(mode==1?4u:mode==2?2u:0u));
  check(w.resources(1)->energy==(mode==0?0u:mode==1?4u:20u));
  check(w.energy_capacity(1,5,0,100,{80,0}));
  check(w.resources(1)->energy==(mode==0?69u:mode==1?84u:100u));
 }
 std::cout<<"PASS Inspired/Revealed World offer/commit, full/partial Energy-debt repayment and capacity clamp, no-hex and real end effects, identity/eligibility guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
