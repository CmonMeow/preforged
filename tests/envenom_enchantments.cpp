#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;using Kind=combat::conditions::Kind;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Envenom line "+std::to_string(at.line()));}
struct Fixture{world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope a,b;
 Fixture(){for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,30,30},100);}a=*w.effect_scope(1);b=*w.effect_scope(2);}
 void enchant(unsigned count){unsigned i=0;for(unsigned skill:{241u,242u,180u,243u}){if(i==count)break;check(w.status_mutation(b,skill,++i,12,100,60100,true));}}
};
int main()try{
 check(enchantment_removal(936,0)->poison_per_remaining_ms==3000&&enchantment_removal(936,12)->poison_per_remaining_ms==9000&&enchantment_removal(936,20)->poison_per_remaining_ms==12000);
 check(casting::Runtime::effect_supported(936)&&!enchantment_removal(936,21));
 for(unsigned rank:{0u,12u,20u})for(unsigned count=0;count<=4;++count){Fixture f;f.enchant(count);f.box->drain();const auto plan=*enchantment_removal(936,rank);auto bad=plan;++bad.poison_per_remaining_ms;check(!f.w.enchantment_removal(f.a,f.b,bad,20,100));
  check(f.w.enchantment_removal(f.a,f.b,plan,20,100));const auto remaining=count?count-1:0;check(f.w.status_snapshot(f.b,100)->size()==remaining);
  check(f.w.has_condition(f.b,Kind::poison,100)==(remaining>0));if(remaining){const auto duration=remaining*plan.poison_per_remaining_ms;check(f.w.has_condition(f.b,Kind::poison,99+duration));check(!f.w.has_condition(f.b,Kind::poison,100+duration));}
  check(!f.w.has_condition(f.b,Kind::bleeding,100));const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,1622))==1);
  check(f.w.resources(1)->health==500&&f.w.resources(1)->energy==30&&f.w.resources(2)->health==500);
 }
 {Fixture f;f.enchant(2);check(f.w.apply_condition(f.a,f.b,Kind::poison,100,60000));check(f.w.enchantment_removal(f.a,f.b,*enchantment_removal(936),20,100));check(f.w.has_condition(f.b,Kind::poison,60099)&&!f.w.has_condition(f.b,Kind::poison,60100));}
 {Fixture f;f.enchant(3);f.w.join(3,"Nearby",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(3,{500,1000,30,30},100);const auto nearby=*f.w.effect_scope(3);
  check(f.w.degeneration_hex(f.a,f.b,*degeneration_hex(19,12),100));check(f.w.apply_fevered_dreams(f.a,f.b,12,false,100,FeveredDazedSpan::attempted_duration));
  check(f.w.enchantment_removal(f.a,f.b,*enchantment_removal(936,12),20,100));check(f.w.resources(2)->health==483);check(f.w.has_condition(nearby,Kind::poison,18099)&&!f.w.has_condition(nearby,Kind::poison,18100)&&f.w.fevered_boundary_failures()==0);
 }
 for(unsigned mode=0;mode<6;++mode){Fixture f;f.enchant(3);casting::Runtime cast(1);casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return f.w.resources(id);};h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.a,at);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return f.w.projectile_target_valid(f.a,id,range,100);};h.enchantment_removal=[&](auto target,const auto& p,auto at){return f.w.enchantment_removal(f.a,target,p,20,at);};
  Policy p;p.primary=4;p.campaigns=255;p.learned[936]=1;Slots bar{};bar[0]={936,0};attributes::Ranks ranks{};ranks[7]=12;if(mode==5)h.enchantment_removal={};check(cast.start(100,{936,0,2},bar,p,ranks,true,h)==(mode==5?casting::Reason::unsupported:casting::Reason::accepted));if(mode==5)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(500,h);
  if(mode==3){f.w.leave(2);f.w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{500,1000,30,30},100);}
  if(mode==4)check(f.w.status_mutation(f.b,243,4,12,500,60500,true));
  cast.update(1099,h);check(!f.w.has_condition(f.b,Kind::poison,1099));cast.update(1100,h);
  if(!mode||mode==4){const auto end=1100+(mode==4?27000:18000);check(f.w.has_condition(f.b,Kind::poison,end-1)&&!f.w.has_condition(f.b,Kind::poison,end));check(cast.start(17000,{936,0,2},bar,p,ranks,true,h)==casting::Reason::recharge);}else check(!f.w.has_condition(f.b,Kind::poison,1100));
  check(f.w.resources(1)->energy==25);
 }
 std::cout<<"PASS Envenom remaining-enchantment duration,newest removal,renewal,Fragility/Fevered,completion count,control/lifetime and caster1622\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
