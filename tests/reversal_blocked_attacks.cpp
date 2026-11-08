#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Blocked Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;effects::Scope source,target,bonded;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool is_npc,unsigned hp=500,bool conditions=true,bool friend_near=false,bool knockdown=true):npc(is_npc){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={double(npc?hp:500),1000};d.damageable=true;d.conditions_allowed=conditions;d.healing_allowed=true;d.healing_evidence="synthetic";
  d.physical_armor=60;d.physical_armor_evidence="synthetic";d.earth_armor=d.fire_armor=d.cold_armor=60;d.fire_armor_evidence=d.cold_armor_evidence="synthetic";
  d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="synthetic";d.knockdown_allowed=knockdown;d.knockdown_evidence="synthetic";
  d.spawn_energy=combat::npcs::Energy{20,100};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:friend_near?100:700,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:id==2&&!npc?hp:500u,1000,id==1?100u:20u,100},100);check(w.physical_armor(*w.effect_scope(id),60));check(w.combat_armor(*w.effect_scope(id),60));check(w.elemental_armor(*w.effect_scope(id),60,60));check(w.player_holy_equipment(*w.effect_scope(id),0));check(w.energy_capacity(id,6,0,100,{80,-2}));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);bonded=*w.effect_scope(3);box->drain();
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
 void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
 bool rr(unsigned at){const auto entries=*w.status_snapshot(target,at);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
 unsigned cast(unsigned skill,unsigned rank=12){
  bool ok;if(skill==195||skill==247||skill==1113)ok=w.damage_field(source,*effects::damage_field(skill,rank),20,100);
  else if(skill==830)ok=w.ray_of_judgment(source,target,*effects::ray_of_judgment(rank),20,100);
  else ok=w.fire_storm(source,target,*effects::fire_storm(rank,skill),20,100);
  check(ok);return 100+effects::damage_field(skill,rank)->first_ms;
 }
 void wound(){if(npc){check(w.apply_condition(source,source,combat::conditions::Kind::deep_wound,100,10000));check(w.condition_transfer(source,target,*effects::condition_transfer(132,12),100));}else check(w.apply_condition(source,target,combat::conditions::Kind::deep_wound,100,10000));}
};
double expected(double hp,unsigned amount,int rank){const auto heal=rank<0?0u:std::min(amount,effects::reversal_fortune(rank)->maximum_healing);return std::max(0.,std::min(1000.,hp+heal)-(amount-heal));}
unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
combat::weapon::Owned weapon(unsigned skill){const auto plan=*combat::weapon::attack_skill(skill,12);return {12,1,{20,20,2},12,false,false,plan.required_type?plan.required_type:27u};}
void block(Fixture& f){check(f.npc?f.w.stage_npc_shield_deflection(f.target,*effects::shield_deflection(12),100):f.w.apply_health_regeneration(f.target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,f.target));f.w.configure_attack_block_draws([]{return 0.;});}
void attack(Fixture& f,unsigned skill,unsigned rank=12,bool missed=false){auto arm=weapon(skill);check(f.w.ordinary_weapon(f.source,arm,20));if(missed)check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,10000));f.w.configure_attack_miss_draws([]{return 0.;});check(f.w.melee_attack(f.source,f.target,arm,*combat::weapon::attack_skill(skill,rank),500));const auto events=f.w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].outcome==(missed?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::block)&&!events[0].before_damage);}
double wounded(double hp){if(hp<=0)return 0;combat::deep_wound::State state;state.reset(1000);state.capacity(hp,1000,true);return hp;}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{327u,341u,356u,386u,1134u})for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(npc,hp);block(f);if(rr>=0)f.reversal(rr);attack(f,skill,rank);const auto reaction=*combat::weapon::blocked_attack_effect(*combat::weapon::attack_skill(skill,rank),combat::weapon::AttackOutcome::block);const auto after=expected(hp,reaction.damage,rr);const auto wanted=skill==341?wounded(after):after;
  check(std::abs(f.hp()-wanted)<1.01&&!f.rr(500));if(wanted>0){if(reaction.knockdown_ms)check(f.w.knocked_down(f.target,500));if(reaction.condition)check(f.w.has_condition(f.target,*reaction.condition,500));}if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));
 }
 for(unsigned skill:{327u,341u,356u,386u,1134u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,10);f.wound();block(f);f.reversal(rr);attack(f,skill,0);check(f.hp()==0&&!f.rr(500));if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 for(unsigned skill:{341u,386u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(true,500,false);block(f);f.reversal(rr);attack(f,skill);const auto damage=combat::weapon::blocked_attack_effect(*combat::weapon::attack_skill(skill,12),combat::weapon::AttackOutcome::block)->damage;check(std::abs(f.hp()-expected(500,damage,rr))<1.01);check(!f.w.has_condition(f.target,skill==341?combat::conditions::Kind::deep_wound:combat::conditions::Kind::bleeding,500));
 }
 for(unsigned skill:{327u,341u,356u,386u,1134u})for(int rr:{-1,0,12,20}){
  ++cases;Fixture f(true);block(f);check(f.w.stage_npc_bonds_of_torment(f.target,f.bonded,*effects::bonds_of_torment_npc(),100));if(rr>=0)f.reversal(rr);attack(f,skill);const auto damage=combat::weapon::blocked_attack_effect(*combat::weapon::attack_skill(skill,12),combat::weapon::AttackOutcome::block)->damage;const auto heal=rr<0?0u:std::min(damage,effects::reversal_fortune(rr)->maximum_healing);check(std::abs(double(f.w.resources(3)->health)-(500-damage+heal))<1.01);
 }
 for(unsigned skill:{327u,356u,1134u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(true,500,true,false,false);block(f);f.reversal(rr);attack(f,skill);check(!f.w.knocked_down(f.target,500));
 }
 // Condition-triggered Fragility occurs after the blocked secondary damage.
 for(unsigned skill:{341u,386u})for(bool npc:{false,true})for(int rr:{-1,0,12,20}){
  ++cases;Fixture f(npc);block(f);check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),100));if(rr>=0)f.reversal(rr);attack(f,skill);const auto damage=combat::weapon::blocked_attack_effect(*combat::weapon::attack_skill(skill,12),combat::weapon::AttackOutcome::block)->damage;const auto after=expected(500,damage,rr);check(std::abs(f.hp()-(after-(skill==341?100:0)-17))<1.01&&!f.rr(500));
 }
 for(unsigned skill:{327u,341u,356u,386u,1134u})for(bool npc:{false,true})for(int rr:{-1,0,12,20}){
  ++cases;Fixture f(npc);block(f);if(rr>=0)f.reversal(rr);attack(f,skill,12,true);check(f.hp()==500&&f.rr(500)==(rr>=0));
 }
 check(cases==481);std::cout<<"PASS "<<cases<<" blocked reactions:conversion,survival,conditions,Fragility,control,Bonds and missed contacts;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
