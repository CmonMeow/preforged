#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Replacement/swarm Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;effects::Scope source,target,bonded;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool is_npc,unsigned hp=500,bool conditions=true,bool friend_near=false):npc(is_npc){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={double(npc?hp:500),1000};d.damageable=true;d.conditions_allowed=conditions;d.healing_allowed=true;d.healing_evidence="synthetic";
  d.earth_armor=d.fire_armor=d.cold_armor=60;d.fire_armor_evidence=d.cold_armor_evidence="synthetic";
  d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="synthetic";d.knockdown_allowed=true;d.knockdown_evidence="synthetic";
  d.spawn_energy=combat::npcs::Energy{20,100};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:friend_near?100:700,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:id==2&&!npc?hp:500u,1000,id==1?100u:20u,100},100);check(w.combat_armor(*w.effect_scope(id),60));check(w.elemental_armor(*w.effect_scope(id),60,60));check(w.player_holy_equipment(*w.effect_scope(id),0));check(w.energy_capacity(id,6,0,100,{80,-2}));}
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
unsigned strike(Fixture& f,unsigned skill,unsigned rank=12,unsigned mode=0){
 if(skill==105){check(f.w.deathly_swarm(f.source,f.target,*effects::deathly_swarm(rank),20,100));f.w.tick(1100);return 1100;}
 skills::Slots bar{};bar[0]={skill,0};check(f.w.configure_adrenaline(f.source,bar,100));check(f.w.status_mutation(f.source,skill,123,rank,100,30100,true));
 combat::weapon::Owned axe{12,1,{20,20,2},12,false,false,2};check(f.w.ordinary_weapon(f.source,axe,20));
 if(mode==1){check(f.w.ordinary_attack(f.source,f.target.actor,200));f.w.tick(700);f.w.cancel_ordinary_attack(f.source);return 700;}
 check(f.w.melee_attack(f.source,mode==2?f.source:f.target,axe,*combat::weapon::attack_skill(mode==2?330:340,12),200));return 200;
}
unsigned amount(unsigned skill,unsigned rank){return skill==105?unsigned(effects::deathly_swarm(rank)->damage):effects::illusionary_weaponry(skill,rank)->damage;}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{33u,3181u,105u})for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);const auto at=strike(f,skill,rank);const auto wanted=expected(hp,amount(skill,rank),rr);check(std::abs(f.hp()-wanted)<1.01&&!f.rr(at));if(npc){const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==unsigned(wanted==0));}
 }
 for(unsigned skill:{33u,3181u})for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned mode:{1u,2u}){
  ++cases;Fixture f(npc);if(rr>=0)f.reversal(rr);const auto at=strike(f,skill,12,mode);check(std::abs(f.hp()-expected(500,amount(skill,12),rr))<1.01&&!f.rr(at));
  const auto outcomes=f.w.take_attack_outcomes(f.source);check(!outcomes.empty());for(const auto& e:outcomes)check(e.outcome==combat::weapon::AttackOutcome::replaced&&!e.before_damage);
 }
 for(unsigned skill:{33u,3181u,105u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,10);f.wound();f.reversal(rr);const auto at=strike(f,skill,0);check(f.hp()==0&&!f.rr(at));if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 for(unsigned skill:{33u,3181u,105u})for(int rr:{-1,0,12,20}){
  ++cases;Fixture f(true);check(f.w.stage_npc_bonds_of_torment(f.target,f.bonded,*effects::bonds_of_torment_npc(),100));if(rr>=0)f.reversal(rr);strike(f,skill);const auto dmg=amount(skill,12);const auto heal=rr<0?0u:std::min(dmg,effects::reversal_fortune(rr)->maximum_healing);check(std::abs(double(f.w.resources(3)->health)-(500-dmg+heal))<1.01);
 }
 // Three distinct mixed recipients must each consume only their own lease.
 for(int rr:{-1,0,12,20})for(bool late:{false,true}){
  ++cases;Fixture f(false,500,true,true);const auto other=*f.w.effect_scope(100);if(rr>=0){f.reversal(rr);check(f.w.status_mutation(f.bonded,307,901,rr,100,8100,true));check(f.w.stage_npc_reversal(other,rr,100));}
  check(f.w.deathly_swarm(f.source,f.target,*effects::deathly_swarm(12),20,100));if(late)f.w.tick(3100);else for(unsigned at:{1100u,2100u,3100u})f.w.tick(at);
  const auto wanted=expected(500,amount(105,12),rr);check(std::abs(f.hp()-wanted)<1.01&&std::abs(double(f.w.resources(3)->health)-wanted)<1.01&&std::abs(f.w.npc_combat_snapshot(other)->health->current-wanted)<1.01);f.w.tick(4100);check(std::abs(f.hp()-wanted)<1.01);
 }
 for(bool npc:{false,true})for(unsigned state:{1u,2u,3u}){
  ++cases;Fixture f(npc);f.reversal(12);check(f.w.deathly_swarm(f.source,f.target,*effects::deathly_swarm(12),20,100));if(state==1)check(f.w.kill(f.source,200));else f.w.leave(1);if(state==3){f.w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);f.w.set_vitals(1,{1000,1000,100,100},100);}f.w.tick(3100);check(f.hp()==500&&f.rr(3100));
 }
 for(unsigned skill:{33u,3181u})for(int rr:{-1,0,12,20}){
  ++cases;Fixture f(false,500,true,true);check(f.w.party_action(2,158,3,100)&&f.w.party_action(3,154,2,100));check(f.w.complete_healing_seed(f.bonded,f.target,*effects::healing_seed(12),0,100));if(rr>=0)f.reversal(rr);strike(f,skill);const auto dmg=amount(skill,12);const bool remaining=rr<0||dmg>effects::reversal_fortune(rr)->maximum_healing;check(f.w.resources(3)->health==500+(remaining?effects::healing_seed(12)->healing:0));
 }
 // Expired replacement cannot bypass ordinary unknown-armor admission.
 for(unsigned skill:{33u,3181u})for(bool npc:{false,true}){
  ++cases;Fixture f(npc);skills::Slots bar{};bar[0]={skill,0};check(f.w.configure_adrenaline(f.source,bar,100));check(f.w.status_mutation(f.source,skill,123,12,100,30100,true));combat::weapon::Owned axe{12,1,{20,20,2},12,false,false,2};check(f.w.ordinary_weapon(f.source,axe,20));check(!f.w.ordinary_attack(f.source,f.target.actor,30100));check(f.hp()==500);
 }
 check(cases==304);std::cout<<"PASS "<<cases<<" IW/Swarm conversion,attack modes,Seed,Bonds,chains and source-lifetime cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
