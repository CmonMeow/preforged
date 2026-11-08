#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Gaze line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope a,b;
 Fixture(unsigned health){for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?1000u:health,1000,30,30},100);}a=*w.effect_scope(1);b=*w.effect_scope(2);}
 void enchant(){check(w.status_mutation(b,180,1,12,100,60100,true));check(w.status_mutation(b,38,2,12,100,46100,true));}
};
int main()try{
 check(enchantment_removal(766)->minimum_health_percent==50&&enchantment_removal(766)->count==UINT32_MAX);
 check(!enchantment_removal(766,21));check(casting::Runtime::effect_supported(766));
 for(unsigned health:{499u,500u,501u,1000u}){
  Fixture f(health);f.enchant();auto p=*enchantment_removal(766);auto bad=p;bad.minimum_health_percent=49;
  check(!f.w.enchantment_removal(f.a,f.b,bad,20,100));auto stale=f.b;++stale.generation;
  check(!f.w.enchantment_removal(f.a,stale,p,20,100));check(!f.w.enchantment_removal(f.a,f.a,p,20,100));
  check(f.w.enchantment_removal(f.a,f.b,p,20,100));check(f.w.status_snapshot(f.b,100)->size()==(health>500?0u:2u));
  check(f.w.resources(2)->health==health&&f.w.resources(1)->energy==30);
  check(f.w.enchantment_removal(f.a,f.b,p,20,100)); // no reward/replay damage
 }
 // Removal evaluates Health before any removed-enchantment end heal.
 {Fixture f(500);check(f.w.protective_enchantment(f.b,f.b,*regeneration::reviewed(299,12),0,100));
  check(f.w.enchantment_removal(f.a,f.b,*enchantment_removal(766),20,100));check(f.w.resources(2)->health==500&&f.w.status_snapshot(f.b,100)->size()==1);}
 for(unsigned mode=0;mode<6;++mode){Fixture f(mode==0?500:501);f.enchant();casting::Runtime cast(1);casting::Hooks h;
  h.events=[](const auto&){};h.resources=[&](auto id){return f.w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.a,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return f.w.projectile_target_valid(f.a,id,range,100);};
  h.enchantment_removal=[&](auto target,const auto& plan,auto at){return f.w.enchantment_removal(f.a,target,plan,20,at);};
  Policy p;p.primary=4;p.campaigns=255;p.learned[766]=1;Slots bar{};bar[0]={766,0};
  if(mode==5)h.enchantment_removal={};
  check(cast.start(100,{766,0,2},bar,p,{},true,h)==(mode==5?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==5)continue;
  if(mode==2)cast.cancel(h);if(mode==3)cast.interrupt(500,h);
  if(mode==0)f.w.set_vitals(2,{501,1000,30,30},100);
  if(mode==1)f.w.set_vitals(2,{500,1000,30,30},100);
  if(mode==4){f.w.leave(2);f.w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{1000,1000,30,30},100);}
  cast.update(2099,h);if(mode<4)check(f.w.status_snapshot(f.b,2099)->size()==2);
  cast.update(2100,h);if(mode<4)check(f.w.status_snapshot(f.b,2100)->size()==(mode==0?0u:2u));
  check(f.w.resources(1)->energy==20);
  if(mode<2)check(cast.start(13000,{766,0,2},bar,p,{},true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Gaze strict completion threshold,all enchantments,no reward,stale target,cancel/interrupt and admission hooks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
