#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Defy stance line "+std::to_string(at.line()));}
struct Fixture{world::Instance w;Scope owner,foe;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Fixture(){for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*30),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}owner=*w.effect_scope(1);foe=*w.effect_scope(2);check(w.lightning_armor(owner,60));}};
int main()try{
 for(unsigned rank=0;rank<=20;++rank){Fixture f;check(f.w.stage_defy_pain_health(f.owner,*defy_pain(rank,true),100));const auto before=f.w.resources(1)->health;
  check(f.w.lightning(f.foe,f.owner,*lightning(224,12),20,100));check(before-f.w.resources(1)->health==45); // Armor applies;PvE flat reduction does not.
 }
 for(unsigned mode=0;mode<8;++mode){Fixture f;check(f.w.stage_defy_pain_health(f.owner,*defy_pain(12,mode!=7),100));const auto entry=f.w.status_snapshot(f.owner,100)->front();f.box->drain();
  if(mode==0)check(f.w.status_mutation(f.owner,346,1001,0,200,8200,true));
  if(mode==1)check(f.w.hex_breaker(f.owner,*hex_breaker(12),20,200));
  if(mode==2)check(f.w.end_stance(f.owner,entry.instance,200));
  if(mode==3)check(f.w.settle_resources(f.owner,20100));
  if(mode==4)check(f.w.kill(f.owner,200));
  if(mode==5)f.w.set_vitals(1,{300,600,100,100},100);
  if(mode==6)check(f.w.stage_defy_pain_health(f.owner,*defy_pain(0,false),200));
  if(mode==7){check(f.w.status_mutation(f.owner,346,1001,0,100,8100,true));check(f.w.stage_defy_pain_health(f.owner,*defy_pain(0,true),200));}
  const auto expected=mode==5?600u:mode>=6?1090u:1000u;check(f.w.resources(1)->max_health==expected);
  world::Packet end{67,0};combat::detail::put(end,1);combat::detail::put(end,entry.instance);auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),end)==1);
  check(f.w.settle_resources(f.owner,mode==3?20200:300));
 }
 {Fixture f;check(f.w.hex_breaker(f.owner,*hex_breaker(12),20,100));check(f.w.stage_defy_pain_health(f.owner,*defy_pain(12,true),200));
  check(f.w.degeneration_hex(f.foe,f.owner,*degeneration_hex(31,12),200));const auto entries=*f.w.status_snapshot(f.owner,200);check(std::any_of(entries.begin(),entries.end(),[](auto e){return e.skill==31;}));
 }
 {Fixture f;Slots bar{};bar[0]={345,0};bar[1]={340,0};check(f.w.configure_adrenaline(f.owner,bar,100));const auto plan=*defensive_stance(12);
  check(f.w.status_mutation(f.owner,345,1001,12,100,100+plan.duration,true));check(f.w.stage_defy_pain_health(f.owner,*defy_pain(12,true),200));
  check(f.w.adrenaline(f.owner)->front().units==25);check(f.w.settle_resources(f.owner,300)&&f.w.adrenaline(f.owner)->front().units==25);
 }
 {Fixture f;f.w.set_vitals(1,{5,1000,100,100},100);check(f.w.stage_defy_pain_health(f.owner,*defy_pain(0,true),100));
  check(f.w.life_steal(f.foe,f.owner,*life_steal(153,0),100));check(f.w.status_mutation(f.owner,346,1001,0,200,8200,true));
  check(f.w.resources(1)->max_health==1000&&f.w.resources(1)->health==1);check(f.w.heal_target(1,1,1,1248,true,200)&&f.w.resources(1)->health==0);
 }
 {Fixture f;f.w.set_vitals(1,{400,1000,100,100},100);check(f.w.stage_defy_pain_health(f.owner,*defy_pain(12,true),100));
  check(f.w.status_mutation(f.owner,456,1001,0,200,15200,true));check(f.w.resources(1)->health==400&&f.w.resources(1)->max_health==1000);
  const auto entries=*f.w.status_snapshot(f.owner,200);check(std::none_of(entries.begin(),entries.end(),[](auto e){return e.skill==456;}));
 }
 std::cout<<"PASS21PvP armor/no-flat ranks,stance replacement both directions,alias refresh,expiry/death/reset/end-once,HexBreaker credit and Defensive reward\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
