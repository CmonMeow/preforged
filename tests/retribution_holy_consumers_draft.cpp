// DETACHED: unregistered acceptance proposal, no production binding.
#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Retribution holy consumers draft line "+std::to_string(at.line()));
}
static unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|unsigned(p[at+1])<<8|unsigned(p[at+2])<<16|unsigned(p[at+3])<<24;}
int main()try{unsigned cases=0;
 for(bool target_protection:{false,true})for(bool return_protection:{false,true})for(bool reversal:{false,true})for(bool seed:{false,true})for(unsigned holy_bonus:{0u,6u}){
  warrior_test::Fixture f(320,false);auto& w=f.world;
  w.configure_attack_critical_draws([]{return .999999;});
  // Large base hit makes recipient Protective Spirit observably precede
  // Retribution's percentage/cap. Small attacker ceiling tests return reduction.
  f.weapon.damage.minimum=f.weapon.damage.maximum=300;
  check(w.ordinary_weapon(f.source,f.weapon,20));
  w.set_vitals(1,{50,100,100,100},100);w.set_vitals(2,{100,100,100,100},100);check(w.player_holy_equipment(f.source,holy_bonus));
  w.join(3,"Return caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.join(4,"Seed caster",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(3,{1000,1000,100,100},100);w.set_vitals(4,{500,1000,100,100},100);
  const auto caster=*w.effect_scope(3),monk=*w.effect_scope(4);
  check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
  check(w.party_action(1,158,4,100)&&w.party_action(4,154,1,100));
  check(w.ordinary_weapon(caster,std::nullopt,20));
  check(w.stage_retribution(caster,f.target,*skills::effects::retribution(20),100));
  if(target_protection)check(w.complete_protective_spirit(f.target,f.target,*skills::effects::protective_spirit(12),0,100));
  if(return_protection)check(w.complete_protective_spirit(f.source,f.source,*skills::effects::protective_spirit(12),0,100));
  if(reversal)check(w.status_mutation(f.source,307,81,0,100,8100,true));
  const auto healing=*skills::effects::healing_seed(12);
  if(seed)check(w.complete_healing_seed(monk,f.source,healing,0,100));
  f.box->drain();check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(320,12),500));w.tick(900);
  const auto packets=f.box->drain();double base=0;unsigned contacts=0;
  for(const auto& packet:packets)if(packet.size()==18&&packet[0]==162&&word(packet,6)==2&&word(packet,10)==1){
   const auto damage=-double(std::bit_cast<float>(word(packet,14)))*1000;
   if(damage>0){base+=damage;++contacts;}
  }
  check(contacts==1&&base>0);
  if(target_protection)check(base<=10.001);else check(base>10.001);
  auto damage=std::min(25.,std::max(1.,std::floor((base+1e-4)*.33)))+holy_bonus;
  if(return_protection)damage=std::min(10.,damage);
  const auto converted=reversal?std::min(15.,damage):0.;
  double hp=std::min(100.,50+converted)-(damage-converted);
  const bool seed_trigger=damage>converted;
  if(seed&&seed_trigger&&hp>0)hp=std::min(100.,hp+healing.healing);
  check(w.resources(1)->health==unsigned(std::ceil(hp)));
  check(w.resources(4)->health==500+(seed&&seed_trigger?healing.healing:0));
  check(!w.retribution_damage_failures()&&!w.retribution_pending_damage_contacts());++cases;
 }
 std::cout<<"PASS "<<cases<<" detached holy bonus/Protective Spirit/Reversal/Seed return consumers\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
