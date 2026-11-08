#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Diversion NPC line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<7;++mode){
  const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
  auto population=world::load_npc_catalog(appearance);
  const std::string relation=mode==5?"neutral":"hostile-to-players";
  const std::string hp=mode==6?"- -":"500 500",flags=mode==6?"0 0":"1 1",evidence=mode==6?"-":"test";
  std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/diversion_npc.cpp\nnpc 146 100 1 1234 0 0 8 1 "+relation+" - "+hp+" 60 "+flags+" test "+evidence+" test - "+evidence+" 60 60 test test\n");
  world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
  world::Instance w;w.configure_npcs(population.for_map(146));auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",mode==4?2000.f:0.f,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),npc=*w.effect_scope(100);const auto plan=*skills::effects::diversion(12);
  if(mode==3)check(w.diversion_skill_used(npc,{28,0},7,100,20000,0)==0);
  box->drain();check(w.diversion(caster,npc,plan,100)==(mode<4));if(mode>=4)continue;
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(100,1,30))==1);
  check(!w.resources(100)&&(w.npc_combat_snapshot(npc)->effects&0x800));
  if(mode==1){check(w.diversion(caster,npc,plan,200));w.tick(6100);check(w.npc_combat_snapshot(npc)->effects&0x800);w.tick(6200);check(!(w.npc_combat_snapshot(npc)->effects&0x800));continue;}
  const skills::Slot slot{mode==2?22u:28u,0};const auto until=w.diversion_skill_used(npc,slot,7,200,20000,0);
  if(mode==3){check(!until&&(w.npc_combat_snapshot(npc)->effects&0x800));continue;}
  check(until&&*until==(mode==2?0u:67200u));check(!(w.npc_combat_snapshot(npc)->effects&0x800));
  check(w.diversion_recharge_until(npc,slot,200)==*until);
  check(!w.diversion_skill_used(npc,slot,7,201,20000,0));
  if(mode==0){
   check(w.diversion(caster,npc,plan,300));check(w.prepare_diversion_skill_use(npc,{67,0},8,400));
   check(w.shatter_delusions(caster,npc,*skills::effects::shatter_delusions(12),20,400));
   check(!(w.npc_combat_snapshot(npc)->effects&0x800));
   check(w.finalize_diversion_skill_use(npc,8,400,10000,0)==57400);
   check(w.diversion_recharge_until(npc,{67,0},400)==57400);
  }
 }
 std::cout<<"PASS Diversion NPC apply/renew/expiry, body52, one-use recharge/instant-reset/replay guards and range/neutral/unknown-Health rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
