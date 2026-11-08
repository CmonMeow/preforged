#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Insidious world regression line "+std::to_string(at.line()));}
void setup(world::Instance& w){
 w.configure_attack_miss_draws([]{return .5;});
 for(unsigned id=1;id<=3;++id){w.join(id,"Actor",id==2?0.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{id==1?100u:500u,500,100,100},100);check(w.physical_armor(*w.effect_scope(id),60));}
}
bool has_hex(world::Instance& w,skills::effects::Scope scope,std::uint64_t at){const auto entries=w.status_snapshot(scope,at);return entries&&std::any_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==123;});}
int main()try{
 for(bool skill:{false,true})for(bool missed:{false,true}){
  world::Instance w;setup(w);const auto caster=*w.effect_scope(1),attacker=*w.effect_scope(2);
  check(w.apply_insidious_parasite(caster,attacker,12,100));
  if(missed)check(w.apply_condition(caster,attacker,combat::conditions::Kind::blind,100,5000));
  combat::weapon::Owned sword{1,1,{120,120,2},12,false,false,27};check(w.ordinary_weapon(attacker,sword,20));
  if(skill)check(w.melee_attack(attacker,caster,sword,*combat::weapon::attack_skill(322,0),600));
  else{check(w.ordinary_attack(attacker,1,100));w.tick(600);w.cancel_ordinary_attack(attacker);}
  check(w.resources(1)->health==(missed?100u:skill?9u:19u)&&w.resources(2)->health==(missed?500u:461u));
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==1&&events[0].before_damage==(missed?std::nullopt:std::optional<std::uint64_t>{600}));
  check(events[0].delivery==combat::weapon::AttackDelivery::melee);
  w.tick(700);check(w.resources(2)->health==(missed?500u:461u)&&w.take_attack_outcomes(attacker).empty());
 }
 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;setup(w);const auto caster=*w.effect_scope(1),attacker=*w.effect_scope(2),other=*w.effect_scope(3);
  check(w.apply_insidious_parasite(caster,attacker,12,100));
  if(mode==0)check(w.kill(caster,200));
  if(mode==1){w.leave(1);w.join(1,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{100,500,100,100},100);}
  if(mode==2){const auto entries=w.status_snapshot(attacker,100);check(entries&&entries->size()==1);check(w.status_mutation(attacker,123,entries->front().instance,12,200,{},false));}
  if(mode==3){check(w.apply_insidious_parasite(other,attacker,0,200));check(w.kill(caster,300));}
  if(mode<3)check(!has_hex(w,attacker,300));
  const auto at=mode==4?13300u:400u;
  combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(attacker,sword,20)&&w.ordinary_attack(attacker,3,at));w.tick(at+500);w.cancel_ordinary_attack(attacker);
  check(w.resources(2)->health==(mode==3?485u:500u)&&w.resources(3)->health==480);
  if(mode==4)check(!has_hex(w,attacker,at+500));
 }
 check(skills::effects::degeneration_hex(123,12).has_value());
 std::cout<<"PASS player Insidious pre-damage survival,ordinary/single melee,miss exclusion,expiry/removal,renewal and caster death/leave cleanup\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
