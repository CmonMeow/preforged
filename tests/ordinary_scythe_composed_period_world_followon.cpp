// NEW unregistered declared-policy fixture. Numeric geometry is authored TEST
// policy, never claimed retail Scythe reach/arc/tie selection or normal enable.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("composed ordinary Scythe timing World line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;Scope actor,hexer,primary;std::vector<Scope> foes;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,35,0.};
 static world::Instance::OrdinaryScythePolicyFollowon policy(){using P=world::Instance::OrdinaryScythePolicyFollowon;return {200,1.5707963267948966,P::Origin::wielder,P::SecondaryOrder::actor_id,P::ReactionOrder::secondary_first_provisional,"authored fixture ONLY; empirical geometry unresolved"};}
 Fixture(unsigned count=3,unsigned hp=500,bool mixed=false,bool enroll=true,bool npc_armor_known=true){
  if(mixed){world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=90;n.y=-30;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,20};d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={1000,1000};d.damageable=d.conditions_allowed=true;if(npc_armor_known){d.physical_armor=60;d.physical_armor_evidence="authored target armor";}d.fire_armor=60;d.fire_armor_evidence="authored target fire armor";d.healing_allowed=d.fleshy=true;d.healing_evidence=d.fleshy_evidence="authored living target";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Scythe owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{hp,1000,100,100},100);actor=*w.effect_scope(1);
  w.join(3,"Hex caster",-1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);hexer=*w.effect_scope(3);
  for(unsigned index=0;index<count;++index){const unsigned id=index==0?2:index==1?4:5;
   if(mixed&&index==2){foes.push_back(*w.effect_scope(100));continue;}
   const float x=index==0?80:index==1?70:90,y=index==0?0:index==1?30:-30;
   w.join(id,"Contact foe",x,y,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{1000,1000,100,100},100);const auto scope=*w.effect_scope(id);foes.push_back(scope);
   check(w.physical_armor(scope,60)&&w.elemental_armor(scope,60,60)&&w.lightning_armor(scope,60));check(w.ordinary_weapon(scope,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));
  }
  primary=foes.front();check(w.ordinary_weapon(actor,weapon,20,100,combat::weapon::CriticalProfile{1,0,20}));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .999999;});
  if(enroll)check(w.configure_ordinary_scythe_policy_followon(actor,policy(),100));box->drain();
 }
 void hex(unsigned skill=26){check(w.empathy(hexer,actor,skill,0,20,100));}
 void vigor(){check(w.complete_vigorous_spirit(actor,actor,*vigorous_spirit(12),0,100));}
 void preparation(unsigned skill){const auto p=combat::weapon::preparation_contact_definition(skill,12);check(bool(p));check(w.status_mutation(actor,skill,123,12,100,100+p->duration,true));}
 void swing(){check(w.ordinary_attack(actor,primary.actor,1000));w.tick(1800);w.cancel_ordinary_attack(actor);}
 double health(Scope scope){return scope.actor==100?w.npc_combat_snapshot(scope)->health->current:double(w.resources(scope.actor)->health);}
 std::vector<combat::weapon::ResolvedAttack> outcomes(){return w.take_attack_outcomes(actor);}
};
int main()try{
 // Both packages operate through ONE existing ordinary state machine: authored
 // geometry max3, shared1500repeat/500windup, one canonical action per release.
 {Fixture f;f.hex(3151);f.vigor();const auto timing=f.w.weapon_attack_timing(f.actor,1000);check(timing&&timing->period_ms==1500&&timing->windup_ms==500);
  check(f.w.ordinary_attack(f.actor,f.primary.actor,1000));f.box->drain();f.w.tick(1499);check(f.outcomes().empty()&&f.w.resources(1)->health==500);for(const auto target:f.foes)check(f.health(target)==1000);
  f.w.tick(1500);const auto first=f.outcomes();check(first.size()==3);for(const auto& e:first)check(e.at==1500&&e.outcome==combat::weapon::AttackOutcome::hit&&!e.skill_failed);
  check(f.w.resources(1)->health==500-45+vigorous_spirit(12)->healing);std::vector<double> first_hp;for(const auto target:f.foes){first_hp.push_back(f.health(target));check(f.health(target)<1000);}f.box->drain();
  // Repeat animation starts500ms before its1500ms repeat deadline. A visit
  // one millisecond earlier must not postpone either owned timer.
  f.w.tick(2499);check(f.outcomes().empty());f.box->drain();f.w.tick(2500);unsigned timing_packets=0;
  for(const auto& packet:f.box->drain())if(packet.size()==14&&packet[0]==53&&packet[1]==0){const auto word=[&](unsigned off){return std::uint32_t(packet[off])|(std::uint32_t(packet[off+1])<<8)|(std::uint32_t(packet[off+2])<<16)|(std::uint32_t(packet[off+3])<<24);};check(std::bit_cast<float>(word(6))==1.5f&&std::bit_cast<float>(word(10))==1.f);++timing_packets;}check(timing_packets==1);
  f.w.tick(2999);check(f.outcomes().empty()&&f.w.resources(1)->health==500-45+vigorous_spirit(12)->healing);for(unsigned i=0;i<3;++i)check(f.health(f.foes[i])==first_hp[i]);
  f.w.tick(3000);const auto second=f.outcomes();check(second.size()==3);for(const auto& e:second)check(e.at==3000&&e.outcome==combat::weapon::AttackOutcome::hit&&!e.skill_failed);
  check(f.w.resources(1)->health==500-90+2*vigorous_spirit(12)->healing);for(unsigned i=0;i<3;++i)check(f.health(f.foes[i])<first_hp[i]);
  const auto hp=f.w.resources(1)->health;f.w.tick(3000);check(f.outcomes().empty()&&f.w.resources(1)->health==hp);f.w.cancel_ordinary_attack(f.actor);
 }
 // Period correction does not implicitly enroll missing/unknown geometry.
 {Fixture f(3,500,false,false);check(!f.w.configure_ordinary_scythe_policy_followon(f.actor,{},100));f.hex(3151);
  const auto timing=f.w.weapon_attack_timing(f.actor,1000);check(timing&&timing->period_ms==1500);check(f.w.ordinary_attack(f.actor,f.primary.actor,1000));f.w.tick(1499);check(f.outcomes().empty());f.w.tick(1500);f.w.cancel_ordinary_attack(f.actor);
  const auto out=f.outcomes();check(out.size()==1&&out[0].target==f.primary&&f.w.resources(1)->health==485);check(f.health(f.foes[1])==1000&&f.health(f.foes[2])==1000);
 }
 std::cout<<"PASS composed declared3contacts + shared1500period/500windup + exacttimers + perfoeEmpathy/onceVigor + geometrydefaultOFF; retailgeometry/hiddenreactionorderunproved\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
