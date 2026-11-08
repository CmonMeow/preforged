#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Hex Breaker World regression line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{28u,100u,76u,137u,101u,150u,31u,53u,129u,2054u,99u,44u,135u})for(unsigned mode=0;mode<5;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Defender",60,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{mode==3&&id==1?20u:500u,500,100,100},100);
  const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2);
  const auto apply=[&](std::uint64_t now){
   if(skill==28)return w.backfire(caster,owner,*skills::effects::backfire(28,12),20,now);
   if(skill==100)return w.soul_barbs(caster,owner,*skills::effects::soul_barbs(12),20,now);
   if(skill==76)return w.movement_hex(caster,owner,*skills::effects::movement_hex(76,12),now);
   if(skill==99)return w.apply_parasitic_bond(caster,owner,12,now);
   if(skill==44)return w.wound_spell(caster,owner,*skills::effects::wound_spell(44,12),20,now);
   if(skill==135)return w.faintheartedness(caster,owner,*skills::effects::faintheartedness(12),now);
   return w.degeneration_hex(caster,owner,*skills::effects::degeneration_hex(skill,12),now);
  };
  if(mode==1)check(apply(100)); // Preventing renewal must retain the old hex.
  check(w.hex_breaker(owner,*skills::effects::hex_breaker(12),20,100));
  if(mode==2){w.tick(65100);box->drain();check(apply(65200));}
  else {box->drain();check(apply(200));}
  const auto status=*w.status_snapshot(owner,mode==2?65200:200);
  check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==10;}));
  check(std::count_if(status.begin(),status.end(),[&](const auto& e){return e.skill==skill;})==(mode==1||mode==2?1:0));
  check(w.resources(1)->health==(mode==2?500u:mode==3?0u:461u));
  if(mode==1)check(std::find_if(status.begin(),status.end(),[&](const auto& e){return e.skill==skill;})->applied==100);
  if(mode==4){w.tick(20100);check(w.resources(2)->health==500);check(w.resources(2)->max_health==500);check(w.status_snapshot(owner,20100)->empty());}
  if(mode==0){check(apply(300));check(w.resources(1)->health==461);const auto after=*w.status_snapshot(owner,300);check(std::any_of(after.begin(),after.end(),[&](const auto& e){return e.skill==skill;}));}
 }
 for(unsigned skill:{108u,142u,136u}){
  world::Instance w;
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Selected",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.join(3,"Nearby",90,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u,3u})w.set_vitals(id,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2),nearby=*w.effect_scope(3);
  check(w.hex_breaker(owner,*skills::effects::hex_breaker(12),20,100));
  check(skill==136?w.faintheartedness(caster,owner,*skills::effects::shadow_of_fear(12),200):w.degeneration_hex(caster,owner,*skills::effects::degeneration_hex(skill,12),200));
  check(w.status_snapshot(owner,200)->empty());
  const auto effects=*w.status_snapshot(nearby,200);
  check(std::any_of(effects.begin(),effects.end(),[&](const auto& e){return e.skill==skill;}));
  check(w.resources(1)->health==461);
  w.tick(1200);check(w.resources(2)->health==500);check(skill==136?w.resources(3)->health==500:w.resources(3)->health<500);
 }
 {
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(2,"Owner",0,0,box);w.set_vitals(2,{500,500,100,100},100);
  const auto owner=*w.effect_scope(2);check(w.hex_breaker(owner,*skills::effects::hex_breaker(12),20,100));
  const auto stance=w.status_snapshot(owner,100)->front();box->drain();
  check(w.status_mutation(owner,349,50,0,200,10200,true));
  const auto removal=skills::casting::effect_remove(2,stance.instance);
  const auto replacement=box->drain();check(std::count(replacement.begin(),replacement.end(),removal)==1);
  w.tick(65100);const auto expired=box->drain();check(std::count(expired.begin(),expired.end(),removal)==0);
 }
 std::cout<<"PASS trusted World Hex Breaker: 13 skills x 5 application modes including delayed-effect absence; stance replacement cleanup; area/Runtime pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
