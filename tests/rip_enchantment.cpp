#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;using Kind=combat::conditions::Kind;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Rip line "+std::to_string(at.line()));}
struct Fixture{world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope a,b;
 Fixture(){for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,30,30},100);}a=*w.effect_scope(1);b=*w.effect_scope(2);}
 void enchant(){check(w.status_mutation(b,241,1,12,100,60100,true));check(w.status_mutation(b,242,2,12,100,60100,true));}
};
int main()try{
 check(enchantment_removal(955,0)->bleeding_ms==5000&&enchantment_removal(955,12)->bleeding_ms==21000&&enchantment_removal(955,20)->bleeding_ms==32000);
 check(casting::Runtime::effect_supported(955)&&!enchantment_removal(955,21));
 for(unsigned rank:{0u,12u,20u})for(bool enchanted:{false,true}){Fixture f;if(enchanted)f.enchant();f.box->drain();const auto plan=*enchantment_removal(955,rank);
  auto bad=plan;++bad.bleeding_ms;check(!f.w.enchantment_removal(f.a,f.b,bad,20,100));auto stale=f.b;++stale.generation;check(!f.w.enchantment_removal(f.a,stale,plan,20,100));
  check(f.w.enchantment_removal(f.a,f.b,plan,20,100));check(f.w.status_snapshot(f.b,100)->size()==unsigned(enchanted));
  check(f.w.has_condition(f.b,Kind::bleeding,100)==enchanted);if(enchanted){check(f.w.has_condition(f.b,Kind::bleeding,99+plan.bleeding_ms));check(!f.w.has_condition(f.b,Kind::bleeding,100+plan.bleeding_ms));}
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,1623))==1);
  check(f.w.resources(1)->health==500&&f.w.resources(1)->energy==30&&f.w.resources(2)->health==500);
 }
 // Condition reactions use the shared producer and propagate at this boundary.
 {Fixture f;f.enchant();f.w.join(3,"Nearby",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(3,{500,1000,30,30},100);const auto nearby=*f.w.effect_scope(3);
  check(f.w.degeneration_hex(f.a,f.b,*degeneration_hex(19,12),100));check(f.w.apply_fevered_dreams(f.a,f.b,12,false,100,FeveredDazedSpan::attempted_duration));
  check(f.w.enchantment_removal(f.a,f.b,*enchantment_removal(955,12),20,100));check(f.w.resources(2)->health==483);
  check(f.w.has_condition(nearby,Kind::bleeding,100)&&f.w.fevered_boundary_failures()==0);
 }
 for(unsigned mode=0;mode<5;++mode){Fixture f;f.enchant();casting::Runtime cast(1);casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return f.w.resources(id);};h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.a,at);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return f.w.projectile_target_valid(f.a,id,range,100);};h.enchantment_removal=[&](auto target,const auto& p,auto at){return f.w.enchantment_removal(f.a,target,p,20,at);};
  Policy p;p.primary=4;p.campaigns=255;p.learned[955]=1;Slots bar{};bar[0]={955,0};if(mode==4)h.enchantment_removal={};check(cast.start(100,{955,0,2},bar,p,{},true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(500,h);if(mode==3){f.w.leave(2);f.w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{500,1000,30,30},100);}
  cast.update(1099,h);check(!f.w.has_condition(f.b,Kind::bleeding,1099));cast.update(1100,h);
  if(mode<3){check(f.w.status_snapshot(f.b,1100)->size()==(mode?2u:1u));check(f.w.has_condition(f.b,Kind::bleeding,1100)==!mode);}
  check(f.w.resources(1)->energy==25);if(!mode)check(cast.start(13000,{955,0,2},bar,p,{},true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Rip conditional Bleeding/newest removal,rank duration,Fragility/Fevered,cast payment/control/lifetime and caster body\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
