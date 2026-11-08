#include "gw_world_simulation.h"
#include <array>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Insidious Cyclone regression line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;w.configure_attack_miss_draws([]{return .5;});
  for(unsigned id=1;id<=4;++id){const float x=id==1?60.f:id==2?0.f:id==3?-60.f:100.f;
   w.join(id,"Actor",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{id==1?100u:500u,500,100,100},100);
   if(mode!=4||id!=4)check(w.physical_armor(*w.effect_scope(id),60));
  }
  const auto caster=*w.effect_scope(1),attacker=*w.effect_scope(2);
  check(w.apply_insidious_parasite(caster,attacker,12,100));
  if(mode==1||mode==2)check(w.apply_condition(caster,attacker,combat::conditions::Kind::blind,100,5000));
  if(mode==2)w.configure_attack_miss_draws([i=0u]()mutable{const std::array draws{.1,.95,.1};if(i>=draws.size())throw std::runtime_error("unexpected extra miss roll");return draws[i++];});
  if(mode==3)for(unsigned id:{1u,3u,4u})check(w.status_mutation(*w.effect_scope(id),299,900+id,20,100,8100,true));
  combat::weapon::Owned axe{1,1,{mode==3?1u:20u,mode==3?1u:20u,2},12,false,false,2};check(w.ordinary_weapon(attacker,axe,20));
  const auto plan=*combat::weapon::attack_skill(330,mode==3?0:12);
  if(mode==4){check(!w.melee_attack(attacker,attacker,axe,plan,200));check(w.resources(1)->health==100&&w.resources(2)->health==500&&w.take_attack_outcomes(attacker).empty());check(w.physical_armor(*w.effect_scope(4),60));}
  check(w.melee_attack(attacker,attacker,axe,plan,200));
  const unsigned hits=mode==1?0:mode==2?1:3;
  const unsigned first_damage=mode==1||mode==2||mode==3?0:30;
  std::cout<<"mode="<<mode<<" afflicted="<<w.resources(2)->health<<" caster="<<w.resources(1)->health<<" qualified_hits_expected="<<hits<<'\n';
  check(w.resources(2)->health==500-39*hits&&w.resources(1)->health==100+39*hits-first_damage);
  check(w.resources(3)->health==(mode==1||mode==3?500u:470u)&&w.resources(4)->health==(mode==1||mode==2||mode==3?500u:470u));
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==3);
  for(const auto& event:events)check(event.delivery==combat::weapon::AttackDelivery::melee);
  for(std::size_t i=0;i<events.size();++i){const bool hit=mode!=1&&(mode!=2||i==1);check(events[i].event==i+1&&events[i].before_damage==(hit?std::optional<std::uint64_t>{200}:std::nullopt));}
  w.tick(300);check(w.resources(2)->health==500-39*hits&&w.take_attack_outcomes(attacker).empty());
 }
 std::cout<<"PASS Insidious Cyclone per-hit transfers,Health rebasing,mixed misses,zero-damage hits,atomic admission and once-only outcomes;retail capped/fatal ordering unverified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
