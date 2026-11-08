#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Retribution protection line "+std::to_string(at.line()));
}
std::uint32_t word(const world::Packet& p,unsigned n){return std::uint32_t(p[n])|std::uint32_t(p[n+1])<<8|std::uint32_t(p[n+2])<<16|std::uint32_t(p[n+3])<<24;}
int main()try{unsigned cases=0;
 for(unsigned skill:{320u,394u,330u,396u})for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<8;++mode)for(bool weaker:{false,true}){
  ++cases;warrior_test::Fixture f(skill,false);auto& w=f.world;
  w.configure_attack_critical_draws([]{return .999999;});
  if(f.weapon.item_type==5){f.weapon.damage.raw_damage_type=1;check(w.ordinary_weapon(f.source,f.weapon,20));}
  check(w.player_holy_equipment(f.source,0));w.set_vitals(1,{500,1000,100,100},100);
  w.join(3,"Retribution caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.join(4,"Seed caster",50,0,std::make_shared<world::Outbox>(),0,{world::Packet{165,0,4,0,0,0,3,0}}, {},100,1);
  w.set_vitals(3,{1000,1000,100,100},100);w.set_vitals(4,{500,1000,100,100},100);
  const auto caster=*w.effect_scope(3),seed_caster=*w.effect_scope(4);
  check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
  check(w.party_action(1,158,4,100)&&w.party_action(4,154,1,100));
  check(w.ordinary_weapon(caster,std::nullopt,20));
  check(w.stage_retribution(caster,f.target,*skills::effects::retribution(rank),100));
  if(weaker){check(w.stage_retribution(caster,f.target,*skills::effects::retribution(0),100));
   const auto status=*w.status_snapshot(f.target,100);check(status.size()==1&&status[0].rank==rank);}
  const bool reversal=mode&1,seed=mode&2,shield=mode&4;
  if(reversal)check(w.status_mutation(f.source,307,81,0,100,8100,true));
  if(shield)check(w.status_mutation(f.source,299,82,12,100,8100,true));
  const auto seed_plan=*skills::effects::healing_seed(12);
  if(seed)check(w.complete_healing_seed(seed_caster,f.source,seed_plan,0,100));
  f.box->drain();check(w.melee_attack(f.source,skill==330?f.source:f.target,f.weapon,*combat::weapon::attack_skill(skill,12),500));
  w.tick(900);const auto packets=f.box->drain();double expected=500;unsigned heals=0,hits=0;bool active=reversal;
  // Use actual authoritative base-hit notices as inputs, not the unrelated
  // before/after Health delta: conversion and other reactions may heal first.
  for(const auto& p:packets){if(p.size()!=18||p[0]!=162||p[1]||word(p,6)!=2||word(p,10)!=1)continue;
   const auto base=-double(std::bit_cast<float>(word(p,14)))*1000;if(base<=0)continue;++hits;
   double damage=std::min<double>(5+rank,std::max(1.,std::floor((base+1e-4)*.33)));
   if(shield)damage=std::max(0.,damage-15);
   if(active&&damage>0){const auto converted=std::min(15.,damage);expected+=converted;damage-=converted;active=false;}
   expected-=damage;if(seed&&damage>0){expected+=seed_plan.healing;++heals;}
  }
  if(!hits||w.resources(1)->health!=unsigned(expected)||w.resources(4)->health!=500+heals*seed_plan.healing||w.retribution_damage_failures()||w.retribution_pending_damage_contacts()){
   std::cerr<<"skill="<<skill<<" rank="<<rank<<" mode="<<mode<<" source="<<w.resources(1)->health<<" expected="<<expected<<" ally="<<w.resources(4)->health<<" heals="<<heals<<" failures="<<w.retribution_damage_failures()<<'\n';check(false);
  }
  const auto statuses=*w.status_snapshot(f.source,900);const auto has_reversal=std::any_of(statuses.begin(),statuses.end(),[](const auto& e){return e.skill==307;});
  check(has_reversal==active);
 }
 std::cout<<"PASS "<<cases<<" Retribution return-damage protection/conversion/Healing Seed cases; no paid binding or retail compound-order claim\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
