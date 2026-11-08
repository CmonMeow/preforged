#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mark of Pain world regression line "+std::to_string(at.line()));}
int main(int argc,char** argv)try{
 check(argc==2);std::ofstream fixture(argv[1],std::ios::binary);
 auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)fixture.put(char(v>>(8*i)));};
 auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};
 fixture.write("FWNAV001",8);for(auto n:{1u,42u,0xeefe704cu,12u,1u,1u})u32(n);
 f32(-500);f32(-500);f32(500);f32(500);u32(2);
 for(unsigned plane=0;plane<2;++plane){u32(1);for(unsigned i=0;i<5;++i)u32(0xffffffff);f32(-500);f32(500);f32(-500);f32(500);f32(-500);f32(500);}
 fixture.close();const auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(argv[1]));
 for(unsigned mode=0;mode<11;++mode){
  world::Instance w;w.configure_navigation(mesh);w.configure_attack_miss_draws([]{return .5;});
  const auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=7;++id){const unsigned team=id==1||id==5||id==6?1:2;
   const float x=id==2||id==5||id==7?60.f:id==3?226.f:id==4?227.f:0.f;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{},id==7?1:0,100,team);w.set_vitals(id,{500,500,100,100},100);
  }
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2),attacker=*w.effect_scope(6);
  check(w.plane(7)==1&&w.plane(2)==0);
  check(w.physical_armor(target,60));
  const auto plan=skills::effects::degeneration_hex(150,12);check(bool(plan)&&plan->pips==0&&plan->radius==0);
  box->drain();check(w.degeneration_hex(caster,target,*plan,100));
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(2,1,150))==1);
  if(mode==1){const auto e=w.status_snapshot(target,100)->front();check(w.status_mutation(target,150,e.instance,12,200,{},false));}
  if(mode==2){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);}
  if(mode==3)check(w.kill(caster,200));
  if(mode==4)check(w.degeneration_hex(caster,target,*skills::effects::degeneration_hex(150,0),200));
  if(mode==5)check(w.apply_condition(target,attacker,combat::conditions::Kind::blind,200,5000));
  if(mode==6)check(w.physical_absorption(target,3));
  if(mode==7)w.set_vitals(2,{10,500,100,100},100);
  if(mode==10)check(w.status_mutation(*w.effect_scope(3),346,900,0,200,8100,true));
  combat::weapon::Owned sword{3,1,{20,20,mode==8?11u:2u},12,false,false,27};
  // The nonphysical trigger case is an earth-damage sword. Physical armor
  // alone is not authority for admitting that attack.
  if(mode==8)check(w.combat_armor(target,60));
  if(mode==6)sword.damage.minimum=sword.damage.maximum=3;
  const auto at=mode==9?30300u:300u;
  check(w.ordinary_weapon(attacker,sword,20)&&w.ordinary_attack(attacker,2,at));w.tick(at+500);w.cancel_ordinary_attack(attacker);
  const unsigned splash=mode==1||mode==2||mode==5||mode==6||mode==8||mode==9?0:mode==4?10:mode==10?68:34;
  if(w.resources(3)->health!=500-splash)
   std::cerr<<"mode="<<mode<<" adjacent="<<w.resources(3)->health<<" expected="<<500-splash<<'\n';
  check(w.resources(3)->health==500-splash);
  check(w.resources(4)->health==500&&w.resources(5)->health==500&&w.resources(7)->health==500);
  if(w.resources(2)->health!=(mode==5||mode==6?500u:mode==7?0u:480u))std::cerr<<"mode="<<mode<<" primary="<<w.resources(2)->health<<" splash="<<w.resources(3)->health<<'\n';
  check(w.resources(2)->health==(mode==5||mode==6?500u:mode==7?0u:480u));
  w.tick(at+600);check(w.resources(3)->health==500-splash);
 }
 std::cout<<"PASS Mark of Pain player AoE boundary/team/layer exclusion,physical-only damage,removal,renewal,caster lifetime and lethal primary hit\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
