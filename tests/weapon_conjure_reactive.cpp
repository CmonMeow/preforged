#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Weapon Conjure reactions line "+std::to_string(at.line()));
}
int main()try{
 unsigned cases=0;
 // Exercise the real release/impact paths. Base and Conjure are distinct
 // damage events; Reversal belongs to the base, Seed to each positive loss.
 for(unsigned skill:{182u,207u,221u})for(unsigned route=0;route<5;++route)
 for(int rr:{-1,0,12,20})for(unsigned initial:{1u,10u,500u,1000u})for(unsigned rank:{0u,12u,20u}){
  world::Instance w;w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .99;});
  auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0.f:id==2?10.f:170.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:id==2?initial:500u,1000,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2),monk=*w.effect_scope(3);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(rank),0,100));
  check(w.conjure_enchantment(owner,*effects::conjure(skill,12),100));
  if(rr>=0)check(w.status_mutation(target,307,900,rr,100,10000,true));
  check(w.elemental_armor(target,60,60)&&w.lightning_armor(target,60));
  const unsigned type=route==0?26:route==3?5:route==4?2:27;
  combat::weapon::Owned weapon{12,1,{20,20,skill==182?5u:skill==207?3u:4u},12,false,route==0,type};
  check(w.ordinary_weapon(owner,weapon,20)&&w.configure_projectiles({100,2}));
  box->drain();const auto epoch=*w.control_epoch(target);
  if(route<2){check(w.ordinary_attack(owner,2,100));w.tick(600);if(route==0)w.tick(700);w.cancel_ordinary_attack(owner);}
  else {const auto plan=*combat::weapon::attack_skill(route==2?322:route==3?394:330,0);if(route!=4)check(w.melee_target(owner,2,100,plan.skill));const auto accepted=w.melee_attack(owner,route==4?owner:target,weapon,plan,100);if(!accepted)std::cerr<<"rejected route="<<route<<" rr="<<rr<<" hp="<<initial<<" skill="<<skill<<" rank="<<rank<<'\n';check(accepted);if(route==3)w.tick(200);}
  const double base=route<2?20:route==3?25:route==4?24:30;
  const auto converted=rr<0?0.:std::min(base,double(effects::reversal_fortune(rr)->maximum_healing));
  double hp=std::min(1000.,initial+converted);unsigned triggers=0;
  for(double damage:{base-converted,17.}){if(hp<=0)break;const auto loss=std::min(hp,damage);hp-=loss;if(loss>0){++triggers;if(hp>0)hp=std::min(1000.,hp+effects::healing_seed(rank)->healing);}}
  if(std::abs(double(w.resources(2)->health)-hp)>=1.01)std::cerr<<"skill="<<skill<<" route="<<route<<" rr="<<rr<<" hp="<<initial<<" rank="<<rank<<" got="<<w.resources(2)->health<<" wanted="<<hp<<'\n';
  check(std::abs(double(w.resources(2)->health)-hp)<1.01);
  check(w.resources(3)->health==std::min(1000u,500+triggers*effects::healing_seed(rank)->healing));
  check(*w.control_epoch(target)==epoch+unsigned(hp<=0));
  const auto status=*w.status_snapshot(target,route==0?700:route==1?600:route==3?200:100);
  check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;}));
  const auto packets=box->drain();
  const bool conjure=std::min(1000.,initial+converted)-(base-converted)>0;
  check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,skill))==unsigned(conjure));
  ++cases;
 }
 check(cases==720);
 // Hostile NPC ordinary hits use the same conversion/Seed transaction but
 // keep zero attribution and the NPC publication gate.
 for(unsigned damage:{10u,40u,200u})for(int rr:{-1,0,12,20})for(unsigned initial:{1u,500u,1000u})for(unsigned rank:{0u,12u,20u}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={1000,1000};d.damageable=true;d.spawn_energy=combat::npcs::Energy{0,30};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Ally",id==1?100.f:240.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{id==1?initial:500u,1000,100,100},100);}
  const auto source=*w.effect_scope(100),target=*w.effect_scope(1),monk=*w.effect_scope(2);
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  check(w.complete_healing_seed(monk,target,*effects::healing_seed(rank),0,100));check(w.elemental_armor(target,60,60));
  if(rr>=0)check(w.status_mutation(target,307,900,rr,100,10000,true));
  combat::ai::FlarePolicy policy;policy.source=source;policy.level=20;policy.fire_magic=12;policy.speed=300;policy.acquire=2000;policy.follow=2500;policy.leash=2500;policy.flight={10000,32};policy.evidence="synthetic weapon conversion fixture";
  check(w.configure_npc_flare_ai(policy,1000));
  combat::ai::ordinary::Reviewed profile{damage,damage,0,5,true,combat::ai::ordinary::Delivery::ranged,500,1500,1000,{100,32},"synthetic fixed fire wand"};
  check(w.configure_npc_ordinary_ai(source,profile,1000));
  w.tick(1000);w.tick(1500);w.tick(2500);
  const auto converted=rr<0?0.:std::min(double(damage),double(effects::reversal_fortune(rr)->maximum_healing));
  double hp=std::max(0.,std::min(1000.,initial+converted)-(damage-converted));
  const bool triggers=damage>converted;
  if(hp>0&&triggers)hp=std::min(1000.,hp+effects::healing_seed(rank)->healing);
  if(std::abs(double(w.resources(1)->health)-hp)>=1.01)std::cerr<<"NPC damage="<<damage<<" rr="<<rr<<" hp="<<initial<<" rank="<<rank<<" got="<<w.resources(1)->health<<" wanted="<<hp<<'\n';
  check(std::abs(double(w.resources(1)->health)-hp)<1.01&&w.resources(2)->health==500+(triggers?effects::healing_seed(rank)->healing:0));++cases;
 }
 check(cases==828);
 // Conversion healing can settle deferred Deep Wound debt and kill. No
 // Conjure or Seed may resurrect the recipient or duplicate death cleanup.
 for(unsigned skill:{182u,207u,221u})for(unsigned route=0;route<5;++route)for(unsigned rr:{0u,12u,20u}){
  world::Instance w;w.configure_attack_critical_draws([]{return .99;});auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0.f:10.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?1000u:10u,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.elemental_armor(target,60,60)&&w.lightning_armor(target,60));
  check(w.conjure_enchantment(source,*effects::conjure(skill,12),100));check(w.status_mutation(target,307,900,rr,100,10000,true));
  check(w.apply_condition(source,target,combat::conditions::Kind::deep_wound,100,10000));
  const unsigned type=route==0?26:route==3?5:route==4?2:27;combat::weapon::Owned weapon{12,1,{20,20,skill==182?5u:skill==207?3u:4u},12,false,route==0,type};
  check(w.ordinary_weapon(source,weapon,20)&&w.configure_projectiles({100,2}));box->drain();const auto epoch=*w.control_epoch(target);
  if(route<2){check(w.ordinary_attack(source,2,100));w.tick(600);if(!route)w.tick(700);w.cancel_ordinary_attack(source);}
  else {const auto plan=*combat::weapon::attack_skill(route==2?322:route==3?394:330,0);if(route!=4)check(w.melee_target(source,2,100,plan.skill));check(w.melee_attack(source,route==4?source:target,weapon,plan,100));if(route==3)w.tick(200);}
  check(w.resources(2)->health==0&&*w.control_epoch(target)==epoch+1);
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,skill))==0);
  w.tick(800);check(*w.control_epoch(target)==epoch+1);++cases;
 }
 check(cases==873);std::cout<<"PASS "<<cases<<" real ordinary/melee/projectile/Cyclone contacts: base-before-Conjure conversion, actual-loss Seed, survival and single death\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
