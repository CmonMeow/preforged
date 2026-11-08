#include "gw_world_simulation.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC weapon reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned hp,double fire=1,std::optional<bool> healing=true){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=10;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;
  d.physical_armor=d.fire_armor=d.cold_armor=d.lightning_armor=60;d.physical_armor_evidence="synthetic weapon recipient";
  d.fire_multiplier=fire;d.fire_multiplier_evidence="synthetic vulnerability";d.healing_allowed=healing;d.healing_evidence="synthetic healing admission";
  n.combat_definition=d;w.configure_npcs({n});w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  source=*w.effect_scope(1);target=*w.effect_scope(100);w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .99;});
 }
 double hp(){return w.npc_combat_snapshot(target)->health->current;}
 bool rr(unsigned at){const auto entries=*w.status_snapshot(target,at);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
 void attack(unsigned route,unsigned raw,bool admit=true){
  const unsigned type=route==0?26:route==3?5:route==4?2:27;
  combat::weapon::Owned weapon{12,1,{20,20,raw},12,false,route==0,type};
  check(w.ordinary_weapon(source,weapon,20)&&w.configure_projectiles({100,2}));
  if(route<2){check(w.ordinary_attack(source,100,100));w.tick(600);if(!route)w.tick(700);w.cancel_ordinary_attack(source);}
  else {const auto plan=*combat::weapon::attack_skill(route==2?322:route==3?394:330,0);if(route!=4)check(w.melee_target(source,100,100,plan.skill));check(w.melee_attack(source,route==4?source:target,weapon,plan,100)==admit);if(route==3)w.tick(200);}
 }
};
static void fixture(std::ofstream& out,const char* scope,unsigned route,unsigned initial,double final,bool critical,const std::vector<world::Packet>& packets){
 if(!out)return;
 out<<"{\"scope\":\""<<scope<<"\",\"route\":"<<route<<",\"initial_health\":"<<initial<<",\"final_health\":"<<final<<",\"critical\":"<<(critical?"true":"false")<<",\"packets\":[";
 bool comma=false;for(const auto& packet:packets){if(comma)out<<',';comma=true;out<<'\"';for(auto b:packet)out<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(b);out<<std::dec<<'\"';}out<<"]}\n";
}
int main(int argc,char** argv)try{
 check(argc<=2);std::ofstream output;if(argc==2){output.open(argv[1]);check(output.good());}
 unsigned world_cases=0,unit_cases=0;
 for(unsigned skill:{182u,207u,221u})for(unsigned route=0;route<5;++route)for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,500u,1000u})for(unsigned factor:{1u,2u}){
  Fixture f(hp,factor);check(f.w.conjure_enchantment(f.source,*effects::conjure(skill,12),100));
  if(rr>=0)check(f.w.stage_npc_reversal(f.target,rr,100));f.box->drain();f.attack(route,skill==182?5u:skill==207?3u:4u);
  const double multiplier=skill==182?factor:1,base=(route<2?20:route==3?25:route==4?24:30)*multiplier;
  const auto converted=rr<0?0.:std::min(base,double(effects::reversal_fortune(rr)->maximum_healing));
  double expected=std::max(0.,std::min(1000.,hp+converted)-(base-converted));const bool bonus=expected>0;
  if(bonus)expected=std::max(0.,expected-17*multiplier);
  if(std::abs(f.hp()-expected)>.001)std::cerr<<"skill="<<skill<<" route="<<route<<" rr="<<rr<<" hp="<<hp<<" factor="<<factor<<" got="<<f.hp()<<" expected="<<expected<<'\n';
  check(std::abs(f.hp()-expected)<.001&&!f.rr(route==0?700:route==1?600:route==3?200:100));
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,skill))==unsigned(bonus));
  if(skill==182&&(route==0||route==2)&&rr==0&&(hp==10||hp==1000)&&factor==1)fixture(output,"World",route,hp,f.hp(),false,packets);
  check(f.w.take_npc_deaths(f.source).size()==unsigned(expected<=0));f.w.tick(800);check(f.w.take_npc_deaths(f.source).empty());++world_cases;
 }
 check(world_cases==480);
 for(unsigned skill:{182u,207u,221u})for(unsigned route=0;route<5;++route)for(unsigned rr:{0u,12u,20u}){
  Fixture f(10);check(f.w.conjure_enchantment(f.source,*effects::conjure(skill,12),100)&&f.w.stage_npc_reversal(f.target,rr,100));
  check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));
  f.box->drain();f.attack(route,skill==182?5u:skill==207?3u:4u);check(f.hp()==0);
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,skill))==0);
  check(f.w.take_npc_deaths(f.source).size()==1);f.w.tick(800);check(f.w.take_npc_deaths(f.source).empty());++world_cases;
 }
 check(world_cases==525);
 // Missing/refused NPC healing metadata rejects Reversal admission, while
 // the ordinary damage/Conjure path remains usable without healing authority.
 for(auto healing:{std::optional<bool>{},std::optional<bool>{false}})for(unsigned route=0;route<5;++route)for(unsigned rr:{0u,12u,20u}){
  Fixture f(500,1,healing);check(!f.w.stage_npc_reversal(f.target,rr,100)&&f.w.conjure_enchantment(f.source,*effects::conjure(182,12),100));
  f.box->drain();f.attack(route,5);const double base=route<2?20:route==3?25:route==4?24:30;
  check(f.hp()==500-base-17&&!f.rr(route==0?700:route==1?600:route==3?200:100));
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,182))==1&&f.w.take_npc_deaths(f.source).empty());++world_cases;
 }
 // Physical contacts use the same reduced amount without enabling elemental
 // rewards or fabricating a skill for ordinary melee.
 for(unsigned route:{1u,2u,4u})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  Fixture f(hp);check(f.w.stage_npc_reversal(f.target,rr,100));f.attack(route,2);
  const double base=route==1?20:route==4?24:30,converted=std::min(base,double(effects::reversal_fortune(rr)->maximum_healing));
  check(f.hp()==std::max(0.,std::min(1000.,hp+converted)-(base-converted)));++world_cases;
 }
 check(world_cases==582);
 // Generic spell admission must not accept skill0; the weapon API and wire
 // publisher must retain it without fabricating a skill attribution.
 for(unsigned rank=0;rank<=20;++rank)for(unsigned hp:{1u,500u,1000u})for(double damage:{10.,67.,200.})for(bool critical:{false,true}){
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={double(hp),1000};d.damageable=true;d.healing_allowed=true;d.healing_evidence="synthetic";
  combat::npcs::LiveActor a(d,{1,1,1,146,100});const auto life=a.lifetime();check(a.apply_reversal_fortune(life,rank,100,1,100));
  auto raw=a;const auto incoming=raw.apply_weapon_damage(life,damage,200);check(bool(incoming)&&incoming->skill==0);
  check(!a.apply_reversal_hit(life,*incoming,200));
  const auto receipt=a.apply_reversal_weapon_hit(life,*incoming,200);check(receipt&&receipt->damage&&receipt->damage->skill==0);
  const auto converted=std::min(damage,double(effects::reversal_fortune(rank)->maximum_healing));
  check(a.actor(life)->health->current==std::max(0.,std::min(1000.,hp+converted)-(damage-converted)));
  const auto packets=combat::npcs::reversal_weapon_packets_2022(100,1,*receipt,critical);
  if(rank==12&&hp==500&&damage==200)fixture(output,"LiveActor",0,hp,a.actor(life)->health->current,critical,packets);
  check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,0))==unsigned(damage>converted));++unit_cases;
 }
 check(unit_cases==378);std::cout<<"PASS "<<world_cases<<" real NPC weapon/Conjure contacts and "<<unit_cases<<" typed zero-attribution conversion/wire cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
