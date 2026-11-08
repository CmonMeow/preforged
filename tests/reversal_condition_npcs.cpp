#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC conditional Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope source,selected;std::vector<effects::Scope> targets;
 Fixture(bool npc_selected=true,unsigned hp=500,bool conditioned=true,bool conditions_allowed=true,unsigned wounded=0,bool cold_known=true){
  const unsigned primary=npc_selected?100:2;std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u,102u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?60:id==101?100:400;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health={double(id==wounded?10:id==primary?hp:500),1000};d.damageable=true;d.conditions_allowed=conditions_allowed;d.healing_allowed=true;d.healing_evidence="synthetic";
   if(cold_known){d.cold_armor=60;d.cold_armor_evidence="synthetic";}n.combat_definition=d;population.push_back(n);
  }w.configure_npcs(population);
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Actor",id==1?0:id==2?60:id==3?100:400,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==1?1000u:id==wounded?10u:id==primary?hp:500u,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),{},60));}
  source=*w.effect_scope(1);selected=*w.effect_scope(primary);for(unsigned id:{2u,3u,4u,100u,101u,102u})targets.push_back(*w.effect_scope(id));
  if(conditioned){
   for(unsigned id:{2u,3u})check(w.apply_condition(source,*w.effect_scope(id),combat::conditions::Kind::blind,100,10000));
   if(conditions_allowed)for(unsigned id:{100u,101u}){check(w.apply_condition(source,source,combat::conditions::Kind::blind,100,10000));check(w.condition_transfer(source,*w.effect_scope(id),*effects::condition_transfer(132,12),100));}
  }
 }
 double hp(unsigned id){return id>=100?w.npc_combat_snapshot(*w.effect_scope(id))->health->current:w.resources(id)->health;}
 bool has(unsigned id,unsigned skill,unsigned now=101){const auto entries=*w.status_snapshot(*w.effect_scope(id),now);return std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.skill==skill;});}
 void reversal(unsigned rank,unsigned actor=0){for(auto t:targets)if(!actor||t.actor==actor)check(t.actor>=100?w.stage_npc_reversal(t,rank,100):w.status_mutation(t,307,900,rank,100,8100,true));}
 bool cast(unsigned skill,unsigned rank=12,unsigned now=101){return w.condition_damage(source,selected,*effects::condition_damage(skill,rank),20,now);}
};
double expected(unsigned hp,unsigned damage,int rank){const auto heal=rank<0?0u:std::min(damage,effects::reversal_fortune(rank)->maximum_healing);return std::max(0.,std::min(1000.,double(hp+heal))-double(damage-heal));}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{828u,864u})for(bool npc_selected:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,500u,1000u})for(bool conditioned:{false,true}){
  ++cases;Fixture f(npc_selected,hp,conditioned);if(rr>=0)f.reversal(rr);const auto damage=effects::condition_damage(skill,rank)->damage;check(f.cast(skill,rank));unsigned npc_deaths=0;
  for(auto t:f.targets){const bool hit=skill==828?t==f.selected:t.actor!=4&&t.actor!=102;const auto base=t==f.selected?hp:500u;const auto wanted=hit?expected(base,damage,rr):base;
   check(std::abs(f.hp(t.actor)-wanted)<1.01);check(f.has(t.actor,307)==(!hit&&rr>=0));check(f.has(t.actor,828)==(skill==828&&hit&&conditioned&&wanted>0));
   for(auto kind:{combat::conditions::Kind::poison,combat::conditions::Kind::weakness})check(f.w.has_condition(t,kind,101)==(skill==864&&hit&&conditioned&&wanted>0));
   if(t.actor>=100&&wanted==0)++npc_deaths;
  }check(f.w.take_npc_deaths(f.source).size()==npc_deaths);
 }
 for(unsigned skill:{828u,864u})for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f(true,500,true,false);f.reversal(rr);check(f.cast(skill));const auto damage=effects::condition_damage(skill,12)->damage;
  check(std::abs(f.hp(100)-expected(500,damage,rr))<1.01&&!f.has(100,828));for(auto kind:{combat::conditions::Kind::poison,combat::conditions::Kind::weakness})check(!f.w.has_condition(*f.w.effect_scope(100),kind,101));
 }
 for(unsigned skill:{828u,864u})for(unsigned wounded:{100u,101u})for(unsigned rr:{0u,12u,20u}){
  if(skill==828&&wounded==101)continue;
  ++cases;Fixture f(true,500,true,true,wounded);const auto t=*f.w.effect_scope(wounded);check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,t,*effects::condition_transfer(132,12),100));f.reversal(rr);check(f.cast(skill));check(f.hp(wounded)==0&&!f.has(wounded,307)&&!f.has(wounded,828));check(f.w.take_npc_deaths(f.source).size()==1);
 }
 for(unsigned rr:{0u,12u,20u}){
  ++cases;Fixture f;f.reversal(rr,100);const auto npc=*f.w.effect_scope(100),bonded=*f.w.effect_scope(4);check(f.w.stage_npc_bonds_of_torment(npc,bonded,*effects::bonds_of_torment_npc(),100));check(f.cast(828));const auto heal=std::min(54u,effects::reversal_fortune(rr)->maximum_healing);check(std::abs(f.hp(4)-(500.-(54-heal)))<1.01);
 }
 // Real NPC degeneration integrates once across renewal and exact expiry.
 {++cases;Fixture f;check(f.cast(828));check(f.hp(100)==446);f.w.tick(1101);check(f.hp(100)==438);check(f.cast(828,12,1101));check(f.hp(100)==384);f.w.tick(2101);check(f.hp(100)==376);f.w.tick(11101);check(f.hp(100)==304&&!f.has(100,828,11101));f.w.tick(12101);check(f.hp(100)==304);}
 for(unsigned skill:{828u,864u}){
  ++cases;Fixture f(true,500,true,true,0,false);const bool ok=f.cast(skill);check(ok==(skill==864));check(f.hp(100)==(skill==864?474:500));if(skill==828)check(f.hp(2)==500);
 }
 {++cases;Fixture f;auto forged=*effects::condition_damage(828,12);++forged.pips;check(!f.w.condition_damage(f.source,f.selected,forged,20,101));auto stale=f.selected;++stale.generation;check(!f.w.condition_damage(f.source,stale,*effects::condition_damage(864,12),20,101));check(f.hp(100)==500&&f.hp(2)==500);}
 check(cases==310);std::cout<<"PASS "<<cases<<" Vile/Oppressive NPC and mixed area conversion,immunity,survival,degeneration renewal/expiry,Bonds and authority guards;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
