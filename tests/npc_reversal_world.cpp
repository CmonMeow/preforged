#include "gw_world_simulation.h"
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC Reversal World line "+std::to_string(at.line()));
}
unsigned u32(const world::Packet& p,unsigned at){unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(p.at(at+i))<<(8*i);return n;}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target,attacker;
 Fixture(unsigned hp=500,unsigned energy=40,bool protected_source=false,unsigned primary=3,bool ready=true,float x=100,unsigned source_hp=1000){
  std::vector<world::Instance::Npc> population;
  world::Instance::Npc n;n.id=n.definition.id=100;n.definition.primary=primary;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,0};
  d.relation=protected_source?combat::npcs::Relation::protected_ally:combat::npcs::Relation::allied_to_players;
  d.spawn_health={double(source_hp),1000};d.spawn_energy=combat::npcs::Energy{energy,40};d.energy_pips=0;
  d.damageable=d.conditions_allowed=!protected_source;d.healing_allowed=d.fleshy=true;
  d.healing_evidence=d.fleshy_evidence="synthetic friendly caster";
  if(!protected_source){d.knockdown_allowed=true;d.knockdown_evidence="synthetic interrupt acceptance";}
  n.combat_definition=d;population.push_back(n);
  n.id=n.definition.id=200;n.definition.primary=0;n.x=100;d.binding={146,200,200,0,0,0,0,0};
  d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=d.conditions_allowed=true;d.spawn_health={10,1000};n.combat_definition=d;population.push_back(n);
  w.configure_npcs(population);
  w.join(1,"Recipient",x,0,box,0,{}, {},100,1,ready);w.set_vitals(1,{hp,1000,100,100},100);
  w.join(2,"Attacker",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  source=*w.effect_scope(100);target=*w.effect_scope(1);attacker=*w.effect_scope(2);box->drain();
 }
 bool enroll(unsigned rank=12,unsigned divine=0){return w.configure_npc_reversal_caster(source,rank,divine,"synthetic reviewed Reversal rank and resources");}
 std::optional<effects::Entry> reversal(unsigned at){const auto entries=*w.status_snapshot(target,at);for(const auto& e:entries)if(e.skill==307)return e;return {};}
};
void packets(std::ofstream& out,const std::vector<world::Packet>& batch){bool first=true;for(const auto& p:batch){if(!first)out<<',';first=false;out<<'\"';constexpr char h[]="0123456789abcdef";for(auto b:p)out<<h[b>>4]<<h[b&15];out<<'\"';}}
int main(int argc,char** argv)try{
 std::ofstream trace;if(argc==2){trace.open(argv[1]);check(bool(trace));}else check(argc==1);unsigned cases=0;
 for(bool protected_source:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned divine:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  Fixture f(hp,40,protected_source);check(f.enroll(rank,divine));
  const auto token=f.w.begin_npc_reversal(f.source,f.target,100);check(token&&token->skill==skills::Slot{307,0}&&token->completes==350);
  check(f.w.npc_energy_snapshot(f.source)->current==35&&!f.reversal(100));
  check(!f.w.complete_npc_action(*token,350)&&!f.w.complete_npc_reversal(*token,250));
  f.w.tick(250);check(!f.reversal(250));f.w.tick(350);
  const auto entry=f.reversal(350);check(entry&&entry->rank==rank&&entry->cause==100&&entry->expires==8350);
  const auto before=std::min(1000u,hp+unsigned(std::lround(3.2*divine)));check(f.w.resources(1)->health==before);
  const auto application=f.box->drain();check(!f.w.complete_npc_reversal(*token,350));
  check(f.w.energy_drain(f.attacker,f.target,*effects::energy_drain(42,12),20,450));
  const double converted=std::min(72u,effects::reversal_fortune(rank)->maximum_healing),healed=std::min(1000.,before+converted);
  const auto expected=std::max(0.,healed-(72-converted));check(f.w.resources(1)->health==unsigned(std::ceil(expected))&&!f.reversal(450));
  const auto impact=f.box->drain();const auto healing=skills::particles::health_change_2022(1,100,healed-before,1000);
  check(std::count(impact.begin(),impact.end(),healing)==unsigned(healed>before));
  if(trace&&!divine&&hp==500){trace<<"{\"protected\":"<<protected_source<<",\"rank\":"<<rank<<",\"final\":"<<expected<<",\"gained\":"<<healed-before<<",\"application\":[";packets(trace,application);trace<<"],\"impact\":[";packets(trace,impact);trace<<"]}\n";}
  check(f.w.npc_energy_snapshot(f.source)->current==35);++cases;
 }
 // Spellbook owns both aftercast and recharge. Generic completion cannot steal
 // the typed pending action or apply the effect twice.
 {Fixture f;check(f.enroll());const auto first=f.w.begin_npc_reversal(f.source,f.target,100);check(first.has_value());f.w.tick(350);
  check(!f.w.begin_npc_reversal(f.source,f.target,1099));check(!f.w.begin_npc_reversal(f.source,f.target,2349));
  const auto second=f.w.begin_npc_reversal(f.source,f.target,2350);check(second&&f.w.npc_energy_snapshot(f.source)->current==30);
  auto forged=*second;++forged.sequence;check(!f.w.complete_npc_reversal(forged,second->completes));
  f.w.tick(second->completes);check(f.reversal(second->completes)&&!f.w.complete_npc_reversal(*second,second->completes));++cases;}
 for(unsigned mode=0;mode<4;++mode){
  Fixture f(500,40,false,3,true,100,mode==3?1:1000);check(f.enroll());const auto token=f.w.begin_npc_reversal(f.source,f.target,100);check(token.has_value());
  if(mode==0){f.w.leave(1);f.w.join(1,"Replacement",100,0,f.box,0,{}, {},150,1);f.w.set_vitals(1,{500,1000,100,100},100);}
  if(mode==1)check(f.w.kill(f.target,150));
  if(mode==2)check(f.w.knockdown_npc(f.source,1000,150)==combat::npcs::control::Knockdown::accepted);
  if(mode==3)check(f.w.apply_condition(f.attacker,f.source,combat::conditions::Kind::burning,150,10000));
  f.w.tick(350);const auto current=*f.w.effect_scope(1);const auto entries=*f.w.status_snapshot(current,350);
  check(std::none_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;}));
  check(f.w.npc_energy_snapshot(f.source)->current==35&&!f.w.complete_npc_reversal(*token,350));++cases;
 }
 {Fixture f;check(f.enroll());check(f.w.apply_condition(f.attacker,f.source,combat::conditions::Kind::dazed,100,5000));
  const auto token=f.w.begin_npc_reversal(f.source,f.target,100);check(token&&token->completes==600);f.w.tick(500);check(!f.reversal(500));f.w.tick(600);check(f.reversal(600).has_value());++cases;}
 for(unsigned mode=0;mode<6;++mode){
  Fixture f(500,mode==0?4:40,false,3,mode!=1,mode==2?2000.f:100.f);check(f.enroll());auto source=f.source,target=f.target;
  if(mode==3)++source.generation;if(mode==4)++target.generation;if(mode==5)target=f.source;
  check(!f.w.begin_npc_reversal(source,target,100)&&f.w.npc_energy_snapshot(f.source)->current==(mode==0?4:40));++cases;
 }
 for(unsigned mode=0;mode<4;++mode){Fixture f(500,40,false,mode==2?0:3);
  check(!f.w.configure_npc_reversal_caster(f.source,mode==0?21:12,mode==1?21:mode==2?12:0,mode==3?"":"evidence"));
  check(f.enroll()&&f.w.npc_energy_snapshot(f.source)->current==40);++cases;}
 // Profane prevents the effect and Divine Favor, but the paid spell still
 // completes. This is an enchantment application veto, not a target failure.
 {Fixture f;check(f.enroll(12,12));
  check(f.w.damage_npc(f.attacker,*f.w.effect_scope(200),{combat::Kind::armor_ignoring,1000,20,194},1248,101).killed);
  check(f.w.claim_well_corpse(f.attacker,*effects::well_cast_plan(94,12),1,102,252,[](const auto&){return true;}));
  const auto token=f.w.begin_npc_reversal(f.source,f.target,200);check(token.has_value());f.w.tick(token->completes);
  check(!f.reversal(token->completes)&&f.w.resources(1)->health==500&&f.w.npc_energy_snapshot(f.source)->current==35);++cases;}
 // Divine Favor can settle lethal wound debt on this player. It must retire
 // the newly applied enchantment and keep the action receipt consumed.
 {Fixture f(10);check(f.enroll(12,12));check(f.w.apply_condition(f.attacker,f.target,combat::conditions::Kind::deep_wound,100,10000));
  const auto token=f.w.begin_npc_reversal(f.source,f.target,100);check(token.has_value());f.w.tick(350);
  check(f.w.resources(1)->health==0&&!f.reversal(350)&&!f.w.complete_npc_reversal(*token,350));++cases;}
 // The newly installed enchantment must be visible to application reactions.
 // Soul Barbs can consume it immediately; DF then uses actual survival rather
 // than a saved pre-application Health value or restoring the new registry.
 for(unsigned rank:{0u,12u,20u})for(unsigned divine:{0u,12u,20u})for(unsigned hp:{1u,500u}){
  Fixture f(hp);check(f.enroll(rank,divine));const auto barbs=*effects::soul_barbs(20);
  check(f.w.soul_barbs(f.attacker,f.target,barbs,20,100));
  const auto token=f.w.begin_npc_reversal(f.source,f.target,100);check(token.has_value());f.w.tick(350);
  const auto healing=std::min(barbs.damage,effects::reversal_fortune(rank)->maximum_healing);
  const auto after=std::max(0.,std::min(1000.,double(hp)+healing)-(barbs.damage-healing));
  const auto expected=after>0?std::min(1000.,after+std::lround(3.2*divine)):0;
  check(f.w.resources(1)->health==unsigned(std::ceil(expected))&&!f.reversal(350));++cases;
 }
 // Linked drain hexes are regeneration/degeneration, not life-steal inputs.
 // They must change Health while leaving Reversal's one-shot effect intact.
 for(unsigned skill:{109u,126u}){
  Fixture f;check(f.enroll());const auto token=f.w.begin_npc_reversal(f.source,f.target,100);check(token.has_value());f.w.tick(350);
  f.w.set_vitals(2,{500,1000,100,100},100);const auto plan=*effects::degeneration_hex(skill,0);check(plan.life_drain);
  check(f.w.degeneration_hex(f.attacker,f.target,plan,450));f.w.tick(1450);
  check(f.w.resources(1)->health==500-2*plan.pips&&f.w.resources(2)->health==500+2*plan.pips&&f.reversal(1450));++cases;
 }
 std::cout<<cases<<" paid friendly NPC Reversal casts: Energy,timing,DF,conversion attribution,interrupt/death,target reuse,readiness/range,Profane and duplicate guards passed;player Runtime307 unbound\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
