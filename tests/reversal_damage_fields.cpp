#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Field Reversal line "+std::to_string(at.line()));
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
int main()try{
 unsigned cases=0;
 for(unsigned skill:{187u,192u,167u,215u,197u,195u,1094u,247u,1113u,830u})for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);const auto at=f.cast(skill,rank);const auto plan=*effects::damage_field(skill,rank);f.w.tick(at);
  const auto wanted=expected(hp,plan.damage,rr);check(std::abs(f.hp()-wanted)<1.01&&!f.rr(at));if(plan.burning_ms)check(f.w.has_condition(f.target,combat::conditions::Kind::burning,at)==(wanted>0));
  if(plan.blind_ms&&wanted>0)check(f.w.has_condition(f.target,combat::conditions::Kind::blind,at));
  if(plan.knockdown_ms&&wanted>0)check(f.w.knocked_down(f.target,at));if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));
 }
 for(unsigned skill:{187u,192u,167u,215u,197u,195u,1094u,247u,1113u,830u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc,10);f.wound();f.reversal(rr);const auto at=f.cast(skill,0);f.w.tick(at);check(f.hp()==0&&!f.rr(at));if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned stance:{0u,16u,17u}){
  ++cases;Fixture f(npc);f.reversal(rr);if(stance)check(npc?f.w.npc_interrupt_stance(f.target,stance,12,10100,100):f.w.status_mutation(f.target,stance,950,12,100,10100,true));
  std::optional<combat::npcs::control::Token> token;if(npc){token=f.w.begin_npc_action(f.target,combat::npcs::control::Action::spell,100,1200,skills::Slot{82,0});check(token.has_value());}else check(f.w.cast_activity(f.target,82,100,1200));
  const auto at=f.cast(215);f.w.tick(at);check(std::abs(f.hp()-expected(500,22,rr))<1.01);
  if(npc)check(f.w.complete_npc_action(*token,1200)==bool(stance));else check(bool(f.w.casting_skill(f.target,at))==bool(stance));
  const auto energy=npc?f.w.npc_energy_snapshot(f.target)->current:f.w.resources(2)->energy;check(energy==(stance==17?15u:20u));
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(bool frequent:{false,true}){
  ++cases;Fixture f(npc);f.reversal(rr);f.cast(197,0);if(frequent)for(unsigned t=200;t<=10100;t+=100)f.w.tick(t);else f.w.tick(10100);check(std::abs(f.hp()-460)<1.01&&!f.rr(10100));
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc);f.reversal(rr);const auto at=f.cast(197);check(f.w.kill(f.source,200));f.w.tick(at);check(std::abs(f.hp()-expected(500,29,rr))<1.01&&!f.rr(at));
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(npc);f.reversal(rr);const auto at=f.cast(197);f.w.leave(1);f.w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);f.w.set_vitals(1,{1000,1000,100,100},100);f.w.tick(at);check(f.hp()==500&&f.rr(at));
 }
 for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(true,hp);f.reversal(rr);check(f.w.stage_npc_bonds_of_torment(f.target,f.bonded,*effects::bonds_of_torment_npc(),100));const auto at=f.cast(197);f.w.tick(at);const auto heal=std::min(29u,effects::reversal_fortune(rr)->maximum_healing);const auto loss=std::min(std::min(1000.,double(hp+heal)),double(29-heal));check(std::abs(double(f.w.resources(3)->health)-(500-loss))<1.01);
 }
 // The party ally is also hit later in this targeted field. Only the victim's
 // actual positive remaining damage triggers Seed before that ally's own hit.
 for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u}){
  ++cases;Fixture f(false,hp,true,true);check(f.w.party_action(2,158,3,100)&&f.w.party_action(3,154,2,100));const auto ally=*f.w.effect_scope(3);check(f.w.complete_healing_seed(ally,f.target,*effects::healing_seed(12),0,100));if(rr>=0)f.reversal(rr);const auto at=f.cast(197);f.box->drain();f.w.tick(at);
  const auto heal=rr<0?0u:std::min(29u,effects::reversal_fortune(rr)->maximum_healing);const bool seed=29>heal;check(f.w.resources(3)->health==500+(seed?effects::healing_seed(12)->healing:0)-29);
  const auto packets=f.box->drain();unsigned damage=unsigned(packets.size()),gain=damage;
  for(unsigned i=0;i<packets.size();++i){const auto& p=packets[i];if(p.size()!=18||p[0]!=162)continue;const auto value=std::bit_cast<float>(word(p,14));if(word(p,6)==2&&value<0)damage=std::min(damage,i);if(word(p,6)==3&&value>0)gain=std::min(gain,i);}
  if(seed)check(damage<gain&&gain<packets.size());else check(gain==packets.size());
 }
 for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(false,10,true,true);check(f.w.party_action(2,158,3,100)&&f.w.party_action(3,154,2,100));check(f.w.complete_healing_seed(*f.w.effect_scope(3),f.target,*effects::healing_seed(12),0,100));f.wound();f.reversal(rr);const auto at=f.cast(197);f.w.tick(at);check(f.hp()==0&&f.w.resources(3)->health==471);check(f.w.player_death_times({2,3})[0]==at);
 }
 for(unsigned skill:{167u,830u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(true,500,false);f.reversal(rr);const auto at=f.cast(skill);f.w.tick(at);check(std::abs(f.hp()-expected(500,effects::damage_field(skill,12)->damage,rr))<1.01);check(!f.w.has_condition(f.target,combat::conditions::Kind::blind,at)&&!f.w.has_condition(f.target,combat::conditions::Kind::burning,at));
 }
 // Empty attack-hex journals cannot suppress ordinary degeneration deaths.
 {++cases;Fixture f(true,28);check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(31,0),100));f.cast(247,0);f.w.tick(5100);const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==1&&deaths.front().time==2100);}
 {++cases;Fixture f(true,6);check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(31,0),100));f.w.tick(5100);const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==1&&deaths.front().time==700);}
 {++cases;Fixture f(true,8);check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::poison,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));f.w.tick(5100);const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==1&&deaths.front().time==1100);}
 check(cases==851);std::cout<<"PASS "<<cases<<" ten-field Reversal pulse/condition/control/Seed/Bonds/schedule/source-lifetime cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
