#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ear Bite Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;effects::Scope source,target,bonded;
 std::vector<std::shared_ptr<world::Outbox>> boxes;
 Fixture(bool is_npc,unsigned hp=500,bool conditions=true,int armor=60):npc(is_npc){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),1000};d.damageable=true;d.conditions_allowed=conditions;
  d.fleshy=true;d.fleshy_evidence="synthetic living target";d.physical_armor=armor;d.physical_armor_evidence="synthetic";d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){auto box=std::make_shared<world::Outbox>();boxes.push_back(box);w.join(id,"Actor",id==1?0:id==2?60:700,0,box,0,{}, {},100,id);w.set_vitals(id,{id==2?hp:1000u,1000,100,100},100);check(w.physical_armor(*w.effect_scope(id),armor));check(w.elemental_armor(*w.effect_scope(id),{},60));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);bonded=*w.effect_scope(3);
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;}
 void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,900,rank,100,8100,true));}
 bool rr(){const auto entries=*w.status_snapshot(target,101);return std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});}
 unsigned drain(){unsigned count=0;for(auto& box:boxes)count+=unsigned(box->drain().size());return count;}
 bool cast(unsigned rank,bool deferred){
  std::function<void()> publish;drain();const bool ok=w.ear_bite(source,target,*effects::ear_bite(rank),20,101,deferred?&publish:nullptr);
  if(ok){if(deferred){check(drain()==0&&bool(publish));publish();check(drain()>0);publish();check(drain()==0);}else check(drain()>0);}
  return ok;
 }
};
double expected(unsigned hp,double damage,int rank){const double heal=rank<0?0:std::min(damage,double(effects::reversal_fortune(rank)->maximum_healing));return std::max(0.,std::min(1000.,hp+heal)-(damage-heal));}
int main()try{
 unsigned cases=0;
 for(bool npc:{false,true})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,5u,10u})for(unsigned hp:{10u,500u,1000u})for(bool deferred:{false,true}){
  ++cases;Fixture f(npc,hp);if(rr>=0)f.reversal(rr);check(f.cast(rank,deferred));const auto wanted=expected(hp,effects::ear_bite(rank)->damage,rr);
  check(std::abs(f.hp()-wanted)<1.01&&!f.rr());check(f.w.has_condition(f.target,combat::conditions::Kind::bleeding,101)==(wanted>0));if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(wanted==0));
 }
 // NPC condition immunity suppresses only Bleeding, never conversion/damage.
 for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(true,hp,false);f.reversal(rr);check(f.cast(10,true));check(std::abs(f.hp()-expected(hp,70,rr))<1.01&&!f.rr()&&!f.w.has_condition(f.target,combat::conditions::Kind::bleeding,101));
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(bool deferred:{false,true}){
  ++cases;Fixture f(npc,10);
  if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
  f.reversal(rr);check(f.cast(5,deferred));check(f.hp()==0&&!f.rr()&&!f.w.has_condition(f.target,combat::conditions::Kind::bleeding,101));if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(int armor:{40,100}){
  ++cases;Fixture f(npc,500,true,armor);f.reversal(rr);check(f.cast(5,false));const auto damage=60*std::exp2((60.-armor)/40.);check(std::abs(f.hp()-expected(500,damage,rr))<1.01);
 }
 for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  ++cases;Fixture f(true,hp);f.reversal(rr);check(f.w.stage_npc_bonds_of_torment(f.target,f.bonded,*effects::bonds_of_torment_npc(),100));
  const auto heal=std::min(60u,effects::reversal_fortune(rr)->maximum_healing);const auto loss=std::min(std::min(1000.,double(hp+heal)),double(60-heal));
  check(f.cast(5,true));check(std::abs(double(f.w.resources(3)->health)-(1000-loss))<1.01);
 }
 check(cases==186);std::cout<<"PASS "<<cases<<" Ear Bite Reversal survival/Bleeding/immunity/armor/Bonds/deferred-publication cases;307 unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
