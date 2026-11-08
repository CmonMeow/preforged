#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Fragility Reversal line "+std::to_string(at.line()));
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
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?60:friend_near?100:700,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{id==1?1000u:id==2&&!npc?hp:500u,1000,id==1?100u:20u,100},100);check(w.combat_armor(*w.effect_scope(id),60));check(w.elemental_armor(*w.effect_scope(id),60,60));check(w.player_holy_equipment(*w.effect_scope(id),0));check(w.energy_capacity(id,6,0,100,{80,-2}));}
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
int main()try{
 unsigned cases=0;
 for(unsigned skill:{19u,2998u})for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(unsigned state:{0u,1u,2u,3u}){
  ++cases;Fixture f(npc,hp);check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(skill,rank),100));if(rr>=0)f.reversal(rr);
  if(state==1)check(f.w.kill(f.source,100));if(state>=2)f.w.leave(1);if(state==3){f.w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);f.w.set_vitals(1,{1000,1000,100,100},100);}
  // A different live source changes the condition; credit belongs to the hex.
  check(f.w.blindness(f.bonded,f.target,*effects::blindness(220,0),200));
  const auto amount=effects::fragility(skill,rank)->damage;const auto wanted=expected(hp,amount,rr);check(std::abs(f.hp()-wanted)<1.01&&!f.rr(200));
  if(npc&&state<2){const auto deaths=f.w.take_npc_deaths(f.source);if(deaths.size()!=unsigned(wanted==0))throw std::runtime_error("death count case="+std::to_string(cases)+" skill="+std::to_string(skill)+" rr="+std::to_string(rr)+" rank="+std::to_string(rank)+" hp="+std::to_string(hp)+" state="+std::to_string(state)+" count="+std::to_string(deaths.size()));if(!deaths.empty())check(deaths.front().time==200);}
  if(state==3)check(f.w.resources(1)->health==1000);
  if(wanted>0){const auto before=f.hp();check(f.w.blindness(f.bonded,f.target,*effects::blindness(220,0),300));check(std::abs(f.hp()-before)<1.01);f.w.tick(3300);check(std::abs(f.hp()-std::max(0.,wanted-amount))<1.01);}
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,10);f.wound();check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),100));f.reversal(rr);
  check(f.w.blindness(f.bonded,f.target,*effects::blindness(220,0),200));check(f.hp()==0&&!f.rr(200));
  if(npc){const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==1&&deaths.front().time==200);}
 }
 // Blind precedes Eruption's own hit. Fragility consumes conversion first.
 for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(npc,hp);check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),100));if(rr>=0)f.reversal(rr);const auto at=f.cast(167);f.w.tick(at);
  const auto first=expected(hp,effects::fragility(19,12)->damage,rr);const auto wanted=std::max(0.,first-effects::damage_field(167,12)->damage);
  check(std::abs(f.hp()-wanted)<1.01&&!f.rr(at));if(npc){const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==unsigned(wanted==0));}
 }
 // Bonds receives only actual remaining damage, never the converted input.
 for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(true,hp);check(f.w.elemental_armor(f.bonded,60,60));check(f.w.stage_npc_bonds_of_torment(f.target,f.bonded,*effects::bonds_of_torment_npc(),100));
  check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),100));if(rr>=0)f.reversal(rr);
  check(f.w.blindness(f.bonded,f.target,*effects::blindness(220,0),200));const auto heal=rr<0?0u:std::min(17u,effects::reversal_fortune(rr)->maximum_healing);
  const auto loss=std::min(double(17-heal),std::min(1000.,hp+double(heal)));check(std::abs(double(f.w.resources(3)->health)-(500-loss))<1.01);
 }
 // Conversion introduced after the gain applies to the later expiry, once.
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,500);check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),100));check(f.w.blindness(f.bonded,f.target,*effects::blindness(220,0),200));check(f.hp()==483);
  check(npc?f.w.stage_npc_reversal(f.target,rr,300):f.w.status_mutation(f.target,307,900,rr,300,8300,true));f.w.tick(3300);const auto wanted=expected(483,17,rr);if(!(std::abs(f.hp()-wanted)<1.01&&!f.rr(3300)))throw std::runtime_error("expiry npc="+std::to_string(npc)+" rr="+std::to_string(rr)+" actual="+std::to_string(f.hp())+" expected="+std::to_string(wanted)+" retained="+std::to_string(f.rr(3300)));f.w.tick(3400);check(std::abs(f.hp()-wanted)<1.01);
 }
 check(cases==624);std::cout<<"PASS "<<cases<<" Fragility gain/refresh/expiry/source/field/conversion cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
