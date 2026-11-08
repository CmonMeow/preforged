// NEW unregistered follow-on. Requires complete detached source package.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <fstream>
#include <filesystem>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Cyclone targetless actual World line "+std::to_string(at.line()));}
// Same known-good synthetic nav container shape as bundle_world.cpp (kind0).
void nav_fixture(const std::filesystem::path& path){
 std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);check(bool(out));
 const auto put=[&](std::uint32_t value){for(unsigned i=0;i<4;++i)out.put(char(value>>(8*i)));};
 const auto number=[&](float value){put(std::bit_cast<std::uint32_t>(value));};
 out.write("FWNAV001",8);for(auto value:{1u,7u,0xeefe704cu,12u,1u,1u})put(value);
 number(-2000);number(-2000);number(2000);number(2000);put(2);
 const auto cell=[&]{for(unsigned i=0;i<5;++i)put(0xffffffff);number(-2000);number(2000);number(-2000);number(2000);number(-2000);number(2000);};
 put(1);cell();put(1);cell();out.close();check(bool(out));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope actor,hexer;
 combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,2,0.};combat::weapon::AttackPlan plan=*combat::weapon::attack_skill(330,0);
 Fixture(unsigned foes,unsigned hp=500,std::shared_ptr<const navigation::Mesh> mesh={}){
  if(mesh)w.configure_navigation(mesh);
  w.join(1,"Cyclone owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{hp,1000,100,100},100);actor=*w.effect_scope(1);
  w.join(3,"Hex caster",-1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);hexer=*w.effect_scope(3);
  for(unsigned i=0;i<foes;++i){const unsigned id=i?4:2;w.join(id,"Adjacent foe",60+40.f*i,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{1000,1000,100,100},100);const auto target=*w.effect_scope(id);check(w.physical_armor(target,60));check(w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));}
  check(w.ordinary_weapon(actor,weapon,20,100,combat::weapon::CriticalProfile{1,0,20}));w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .999999;});box->drain();
 }
 void hex(){check(w.empathy(hexer,actor,26,0,20,100));}
 std::pair<combat::weapon::AttackPermit,std::uint64_t> prepare(){const auto duration=w.attack_duration(actor,plan.windup_ms,1000,true);check(duration&&*duration);const auto at=1000+*duration;auto permit=w.prepare_cyclone_cast_followon(actor,weapon,plan,1000,at);check(bool(permit));return {permit,at};}
};
int main()try{
 for(bool refusal:{false,true}){
  Fixture baseline(0),f(0);f.hex();
  for(auto* fixture:{&baseline,&f}){
   check(fixture->w.elemental_armor(fixture->actor,60,60));
   check(fixture->w.fire_storm(fixture->hexer,fixture->actor,*fire_storm(0,197),20,100));
  }
  auto [permit,at]=f.prepare();check(at>=1100&&at<2100);
  if(refusal)permit->cancel();
  // Same real queued field, independently materialized through ordinary tick.
  baseline.w.tick(at);const auto field_health=baseline.w.resources(1)->health;
  check(field_health<500);f.box->drain();
  auto result=f.w.complete_cyclone_cast_followon(f.actor,f.weapon,f.plan,permit,at);
  check(result.accepted==!refusal&&result.contacts.empty());
  check(f.w.resources(1)->health==field_health-(refusal?0u:10u));
  if(refusal){check(!result.publication);check(!f.box->drain().empty());}
  else{
   check(bool(result.publication));check(f.box->drain().empty());
   result.publication();check(!f.box->drain().empty());
   result.publication();check(f.box->drain().empty());
  }
 }
 std::cout<<"PASS Cyclone completion owns causal FireStorm settlement; refusal flushes; closure exact once\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
