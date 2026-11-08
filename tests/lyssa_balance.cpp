#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Lyssa line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope a,b;
 Fixture(){for(unsigned id:{1u,2u}){w.join(id,"Player",id==1?0:60,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,1000,30,30},100);}a=*w.effect_scope(1);b=*w.effect_scope(2);}
 void enchant(Scope owner,unsigned count,std::uint64_t end=60100){unsigned i=0;for(unsigned skill:{241u,242u,180u}){if(i==count)break;check(w.status_mutation(owner,skill,++i,12,100,end,true));}}
};
int main()try{
 const auto plan=*enchantment_removal(877);check(plan.compare_enchantments&&plan.count==1&&!plan.damage&&!plan.energy);
 check(casting::Runtime::effect_supported(877)&&!enchantment_removal(877,21));
 // Shared allocation-free count excludes other categories,future applications,
 // expired entries and wrong generations; expiry is exclusive.
 {Scope owner{1,2,3};Registry registry(owner);
  check(registry.apply(owner,{1,2,241,0,Category::enchantment,100,200},Replace::independent).accepted);
  check(registry.apply(owner,{2,2,180,0,Category::enchantment,300,400},Replace::independent).accepted);
  check(registry.apply(owner,{3,2,28,0,Category::hex,100,500},Replace::independent).accepted);
  check(registry.count(owner,Category::enchantment,99)==0&&registry.count(owner,Category::enchantment,100)==1);
  check(registry.count(owner,Category::enchantment,200)==0&&registry.count(owner,Category::enchantment,300)==1);
  check(registry.count({1,2,4},Category::enchantment,100)==0);}
 for(unsigned own=0;own<=3;++own)for(unsigned foe=0;foe<=3;++foe){Fixture f;f.enchant(f.a,own);f.enchant(f.b,foe);
  auto forged=plan;forged.compare_enchantments=false;check(!f.w.enchantment_removal(f.a,f.b,forged,20,100));
  auto stale=f.b;++stale.generation;check(!f.w.enchantment_removal(f.a,stale,plan,20,100));
  check(!f.w.enchantment_removal(f.a,f.a,plan,20,100));f.box->drain();
  check(f.w.enchantment_removal(f.a,f.b,plan,20,100));
  const auto entries=*f.w.status_snapshot(f.b,100);const bool removed=foe&&own<=foe;
  check(entries.size()==foe-unsigned(removed)&&f.w.status_snapshot(f.a,100)->size()==own);
  if(removed&&foe>1)check(std::none_of(entries.begin(),entries.end(),[&](const auto& e){return e.instance==foe;}));
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,877))==unsigned(removed));
  check(f.w.resources(1)->health==500&&f.w.resources(1)->energy==30&&f.w.resources(2)->health==500);
 }
 for(unsigned mode=0;mode<7;++mode){Fixture f;f.enchant(f.b,1,mode==2?1100:60100);if(mode==1)f.enchant(f.a,2,1100);
  casting::Runtime cast(1);casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return f.w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.a,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return f.w.projectile_target_valid(f.a,id,range,100);};
  h.enchantment_removal=[&](auto target,const auto& p,auto at){return f.w.enchantment_removal(f.a,target,p,20,at);};
  Policy p;p.primary=5;p.campaigns=255;p.learned[877]=1;Slots bar{};bar[0]={877,0};if(mode==6)h.enchantment_removal={};
  check(cast.start(100,{877,0,2},bar,p,{},true,h)==(mode==6?casting::Reason::unsupported:casting::Reason::accepted));if(mode==6)continue;
  if(mode==0)f.enchant(f.a,2);if(mode==3)cast.cancel(h);if(mode==4)cast.interrupt(500,h);
  if(mode==5){f.w.leave(2);f.w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{500,1000,30,30},100);}
  cast.update(1100,h);if(mode<5)check(f.w.status_snapshot(f.b,1100)->size()==((mode==1||mode==2)?0u:1u));
  check(f.w.resources(1)->energy==25);if(mode<3)check(cast.start(13000,{877,0,2},bar,p,{},true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Lyssa active counts,equality,newest single removal,expiry,completion changes,cancel/control/lifetime and target body\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
