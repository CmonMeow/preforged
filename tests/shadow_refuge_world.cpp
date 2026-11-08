#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw;
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static combat::weapon::Owned sword(){return {3,1,{20,20,2},12,false,false,27};}
static std::unique_ptr<world::Instance> fixture(){
 auto w=std::make_unique<world::Instance>();
 for(unsigned id:{1u,2u}){
  w->join(id,"Actor",float((id-1)*60),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
  w->set_vitals(id,{100,1000,30,30},100);
 }
 check(w->physical_armor(*w->effect_scope(2),60),"target armor");
 return w;
}
int main()try{
 const auto plan=*skills::regeneration::reviewed(814,12);
 for(unsigned mode=0;mode<4;++mode){
  auto w=fixture();auto self=*w->effect_scope(1);
  check(w->apply_health_regeneration(self,1,plan,1000,7000,0),"apply self refuge");
  if(mode==1||mode==3){
   check(w->ordinary_weapon(self,sword(),20),"weapon");
   check(w->ordinary_attack(self,2,6400),"attack start");
   if(mode==3)w->cancel_ordinary_attack(self);
  }
  if(mode==2){
   check(w->apply_health_regeneration(self,1,plan,2000,8000,0),"renewal");
   w->tick(7000);check(w->resources(1)->health==208,"old expiry after renewal");
   w->tick(8000);check(w->resources(1)->health==226,"renewed regeneration without idle end heal");
  }else{
   w->tick(7000);
   check(w->resources(1)->health==(mode==1?296:208),"conditional expiry heal");
   w->tick(8000);check(w->resources(1)->health==(mode==1?296:208),"expiry once");
  }
 }
 {
  auto w=fixture();auto self=*w->effect_scope(1);
  check(w->apply_health_regeneration(self,1,plan,1000,7000,0),"late-cancel refuge");
  check(w->ordinary_weapon(self,sword(),20),"late-cancel weapon");
  check(w->ordinary_attack(self,2,6400),"late-cancel attack");
  w->cancel_ordinary_attack(self,7500);
  check(w->resources(1)->health==296,"late skill start retains attacking expiry heal");
  w->tick(8000);
  check(w->resources(1)->health==296,"late expiry heal occurs once");
 }
 {
  auto w=fixture();auto self=*w->effect_scope(1);
  check(w->apply_health_regeneration(self,1,plan,1000,7000,0),"early-cancel refuge");
  check(w->ordinary_weapon(self,sword(),20),"early-cancel weapon");
  check(w->ordinary_attack(self,2,6400),"early-cancel attack");
  w->cancel_ordinary_attack(self,6500);
  w->tick(8000);
  check(w->resources(1)->health==208,"early skill start prevents attacking expiry heal");
 }
 for(bool already_attacking:{false,true}){
  auto w=fixture();auto self=*w->effect_scope(1);
  check(w->apply_health_regeneration(self,1,plan,1000,7000,0),"late attack refuge");
  check(w->ordinary_weapon(self,sword(),20),"late attack weapon");
  if(already_attacking)check(w->ordinary_attack(self,2,6400),"first attack before expiry");
  check(w->ordinary_attack(self,2,7500),"new attack after expiry");
  check(w->resources(1)->health==(already_attacking?296:208),"expiry resolves before new attack state");
  w->tick(8000);
  check(w->resources(1)->health==(already_attacking?296:208),"new attack cannot change past expiry");
 }
 std::cout<<"PASS Shadow Refuge owned regeneration and attacking end heal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
