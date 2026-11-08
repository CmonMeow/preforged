#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Projectile miss-state regression line "+std::to_string(at.line()));}
int main()try{
 for(bool npc_target:{false,true})for(bool applied_after_launch:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.configure_attack_miss_draws([]{return .5;});
  if(npc_target){
   const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
   auto population=world::load_npc_catalog(appearance);
   std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/projectile_miss_state.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");
   const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);w.configure_npcs(population.for_map(146));
  }
  w.join(1,"Wand",0,0,box,0,{}, {},100,1);w.join(2,"Foe",100,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,20,20},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc_target?100:2),condition_source=*w.effect_scope(2);
  combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(w.ordinary_weapon(source,wand,20)&&(npc_target||w.combat_armor(target,60))&&w.configure_projectiles({100,32}));
  if(!applied_after_launch)check(w.apply_condition(condition_source,source,combat::conditions::Kind::blind,100,700));
  check(w.ordinary_attack(source,target.actor,100));w.tick(600);
  if(applied_after_launch)check(w.apply_condition(condition_source,source,combat::conditions::Kind::blind,800,5000));
  w.tick(1600);w.cancel_ordinary_attack(source);
  const auto expected=applied_after_launch?500u:480u;
  const double health=npc_target?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;
  std::cout<<"npc="<<npc_target<<" applied_after_launch="<<applied_after_launch<<" Health="<<health<<" expected="<<expected<<'\n';
  check(health==expected);
 }
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
