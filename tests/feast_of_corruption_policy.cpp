#include "gw_feast_of_corruption.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Feast of Corruption policy regression");}
int main()try{
 check(!feast_of_corruption(21));
 const auto low=*feast_of_corruption(0),high=*feast_of_corruption(20),plan=*feast_of_corruption(12);
 check(low.damage==16&&low.steal==8&&high.damage==101&&high.steal==51&&plan.damage==67&&plan.steal==34&&plan.radius==166&&plan.range==1248&&plan.retain_dead_selected_anchor);
 for(bool hexed:{false,true})for(int armor:{0,200}){
  combat::Actor target{2,3,combat::Health{500,500},armor,0,true};
  const auto result=feast_of_corruption_target(plan,{100,500},target,hexed,20);check(bool(result));
  check(result->shadow.damage==67&&result->target.health->current==(hexed?399:433)&&result->source.current==(hexed?134:100)&&bool(result->steal)==hexed);
 }
 combat::Actor target{2,3,combat::Health{500,500},0,0,true};target.incoming_reduction=5;
 auto result=feast_of_corruption_target(plan,{490,500},target,true,20);check(result&&result->shadow.damage==62&&result->target.health->current==409&&result->steal->stolen==29&&result->source.current==500&&result->steal->gained==10);
 target.incoming_reduction=0;target.health=combat::Health{70,500};result=feast_of_corruption_target(plan,{100,500},target,true,20);
 check(result&&result->steal&&result->steal->stolen==4&&result->target.health->current==0&&result->source.current==104&&(result->target.effects&0x10));
 target.health=combat::Health{60,500};result=feast_of_corruption_target(plan,{100,500},target,true,20);check(result&&result->shadow.killed&&!result->steal&&result->source.current==100);
 target.health=combat::Health{0,500};check(!feast_of_corruption_target(plan,{100,500},target,true,20));
 target.health=combat::Health{500,500};auto forged=plan;forged.radius=10000;check(!feast_of_corruption_target(forged,{100,500},target,true,20));
 check(!feast_of_corruption_target(plan,{0,500},target,true,20)&&!feast_of_corruption_target(plan,{100,500},target,true,0));
 std::cout<<"PASS Feast canonical plan and candidate shadow-then-steal transaction,hex qualification,reduction,overheal/overkill;world/ordering/rendered acceptance pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
