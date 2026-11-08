#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mind Wrack World line "+std::to_string(at.line()));}
int main()try{
 for(bool pvp:{false,true})for(unsigned mode=0;mode<9;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},mode==8?300:100,2);w.set_vitals(1,{500,500,100,100},100);w.set_vitals(2,{500,500,mode==1?2u:1u,100},100);const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);check(w.energy_capacity(2,5,0,mode==8?300:100,{80,-2}));
  if(mode==7||mode==8)check(w.hex_breaker(target,*hex_breaker(12),20,mode==8?300:100));
  box->drain();check(w.apply_mind_wrack(caster,target,12,pvp,mode==8?200:100)==(mode!=8));
  if(mode==8){check(w.resources(1)->health==500&&w.status_snapshot(target,300)->size()==1);continue;}
  if(mode==7){check(w.resources(1)->health==461&&w.status_snapshot(target,100)->empty());continue;}
  auto status=*w.status_snapshot(target,100);check(status.size()==1&&status[0].skill==(pvp?2734u:49u)&&status[0].expires==33100);const auto effect=status[0].instance;
  auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(2,1,pvp?2734:49))==1);
  check(!w.status_mutation(target,49,999,12,100,33100,true));
  if(mode==2)check(w.status_mutation(target,pvp?2734:49,effect,12,200,{},false));
  if(mode==3)check(w.kill(caster,200));
  if(mode==4){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},200,1);w.set_vitals(1,{500,500,100,100},100);check(!w.apply_mind_wrack(caster,target,12,pvp,200));}
  if(mode==5){w.tick(33100);check(w.status_snapshot(target,33100)->empty());check(w.consume_energy(2,1,target,33100)&&w.resources(2)->health==500);continue;}
  if(mode==6){check(w.apply_mind_wrack(caster,target,0,!pvp,200));status=*w.status_snapshot(target,200);check(status.size()==1&&status[0].skill==(pvp?49u:2734u)&&status[0].expires==5200);}
  check(w.consume_energy(2,1,target,200));
  if(mode==1){check(w.resources(2)->energy==1&&w.resources(2)->health==500&&w.status_snapshot(target,200)->size()==1);check(w.consume_energy(2,1,target,300));}
  check(w.resources(2)->energy==0&&w.resources(2)->health==((mode>=2&&mode<=4)?500u:mode==6?485u:417u));
  check(w.status_snapshot(target,mode==1?300:200)->empty());check(w.consume_energy(2,0,target,400)&&w.resources(2)->health==((mode>=2&&mode<=4)?500u:mode==6?485u:417u));
 }
 // A replacement caster owns the new alias; the old caster's death cannot
 // remove or take credit for that new effect instance.
 {world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==2?60:0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{500,500,id==2?1u:100u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2),c=*w.effect_scope(3);check(w.energy_capacity(2,5,0,100,{80,-2}));check(w.apply_mind_wrack(a,b,12,false,100)&&w.apply_mind_wrack(c,b,0,true,200));check(w.kill(a,200));const auto hex=*w.status_snapshot(b,200);check(hex.size()==1&&hex[0].cause==3&&hex[0].skill==2734);check(w.consume_energy(2,1,b,300)&&w.resources(2)->health==485);}
 // A future original-caster cursor rejects the cost before Energy changes;
 // delayed lethal degeneration settles the caster and ends the hex first.
 for(bool dying:{false,true}){world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1&&dying?1u:500u,500,id==1?100u:1u,100},100);}const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);check(w.energy_capacity(2,5,0,100,{80,-2})&&w.apply_mind_wrack(caster,target,12,false,100));
  if(dying){check(w.degeneration_hex(target,caster,*degeneration_hex(31,12),100));check(w.consume_energy(2,1,target,1100));check(w.resources(1)->health==0&&w.resources(2)->health==500&&w.status_snapshot(target,1100)->empty());}
  else{check(w.settle_resources(caster,300));check(!w.consume_energy(2,1,target,200)&&w.resources(2)->energy==1&&w.resources(2)->health==500);check(w.consume_energy(2,1,target,300)&&w.resources(2)->health==417);}
 }
 const std::string hash(64,'a');std::istringstream a("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\n");std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/mind_wrack_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");auto pop=world::load_npc_catalog(a);world::bind_combat_catalog(pop,combat::npcs::load_catalog(m,hash),hash);
 for(bool leave:{false,true}){world::Instance w;w.configure_npcs(pop.for_map(146));w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);const auto caster=*w.effect_scope(1),npc=*w.effect_scope(100);check(w.apply_mind_wrack(caster,npc,12,false,100));check(w.npc_combat_snapshot(npc)->effects&0x800);check(w.apply_mind_wrack(caster,npc,0,true,200));if(leave)w.leave(1);else check(w.kill(caster,300));check(!(w.npc_combat_snapshot(npc)->effects&0x800));}
 std::cout<<"PASS Mind Wrack World player/NPC lifetime/body83, PvE/PvP alias renewal, actual timestamped cost zero burst once, ordinary removal/expiry/caster death/departure, protection and invalid future recipient\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
