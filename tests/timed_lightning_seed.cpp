#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Timed lightning Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;unsigned target_id,monk_id,skill;std::uint64_t cast_at,due;effects::Scope source,target,monk;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned id,unsigned hp,unsigned rank,unsigned owner=2,bool expires=false,bool unknown=false):target_id(owner),monk_id(owner==2?3:2),skill(id),cast_at(expires?7100:1000),due(cast_at+3000){
  if(unknown){world::Instance::Npc n;n.id=n.definition.id=100;n.x=80;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned actor:{1u,2u,3u}){w.join(actor,"Actor",actor==1?0.f:actor==target_id?60.f:100.f,0,actor==target_id?box:std::make_shared<world::Outbox>(),0,{}, {},100,actor==1?1:2);w.set_vitals(actor,{actor==1?1000u:actor==target_id?1000u:500u,1000,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(target_id);monk=*w.effect_scope(monk_id);check(w.lightning_armor(target,80));
  check(w.party_action(target_id,158,monk_id,100)&&w.party_action(monk_id,154,target_id,100));
  if(expires)check(w.complete_healing_seed(monk,target,*effects::healing_seed(rank),0,100));
  if(skill==222){check(w.activate_overcast_energy(source,219,0,1,100));check(w.lightning_strike(source,target,12,20,cast_at));}
  else check(w.lightning_surge(source,target,12,20,cast_at));
  w.set_vitals(target_id,{hp,1000,100,100},100);w.set_vitals(monk_id,{500,1000,100,100},100);
  if(!expires)check(w.complete_healing_seed(monk,target,*effects::healing_seed(rank),0,cast_at+1000));
  box->drain();
 }
 void rr(int rank){if(rank>=0)check(w.status_mutation(target,307,900,rank,cast_at+1000,due+8000,true));box->drain();}
 double damage()const{return skill==222?effects::lightning_strike(12)->damage:effects::lightning_surge(12)->damage;}
 unsigned hp()const{return w.resources(target_id)->health;}
 unsigned ally()const{return w.resources(monk_id)->health;}
 void tick(bool late){if(late)w.tick(due+6000);else{for(auto at=cast_at+1100;at<due;at+=100)w.tick(at);w.tick(due);}}
};
int main()try{
 unsigned cases=0;
 for(unsigned skill:{205u,222u})for(unsigned target:{2u,3u})for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,500u,1000u})for(unsigned rank:{0u,12u,20u})for(bool late:{false,true}){
  Fixture f(skill,hp,rank,target);f.rr(rr);f.tick(late);const auto raw=f.damage(),converted=rr<0?0.:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));
  const auto healed=std::min(1000.,hp+converted),loss=std::min(healed,raw-converted);const bool triggers=loss>0;const auto healing=effects::healing_seed(rank)->healing;
  const auto wanted=healed-loss>0?std::min(1000.,healed-loss+(triggers?healing:0)):0.;check(std::abs(double(f.hp())-wanted)<1.01&&f.ally()==500+(triggers?healing:0));
  if(!wanted)check(f.w.player_death_times({f.target_id,f.monk_id})[0]==f.due);
  if(skill==205){check(f.w.has_condition(f.target,combat::conditions::Kind::cracked_armor,late?f.due+6000:f.due)==bool(wanted>0));if(!late)check(f.w.knocked_down(f.target,f.due)==bool(wanted>0));}
  f.w.tick(f.due+6001);check(f.ally()==500+(triggers?healing:0));++cases;
 }
 // Exact Seed expiry at the damage deadline must not trigger healing.
 for(unsigned skill:{205u,222u})for(unsigned target:{2u,3u})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,500,rank,target,true);f.rr(rr);f.tick(true);const auto raw=f.damage(),converted=rr<0?0.:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));check(std::abs(double(f.hp())-(500+converted-(raw-converted)))<1.01&&f.ally()==500);++cases;
 }
 for(unsigned skill:{205u,222u})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,500,rank);f.rr(rr);check(f.w.remove_effects(f.target,f.target_id,*effects::removal(301,0,0),f.cast_at+1100));f.tick(true);check(f.hp()==500&&f.ally()==500);++cases;
  Fixture gone(skill,500,rank);gone.rr(rr);gone.w.leave(1);gone.tick(true);check(gone.hp()==500&&gone.ally()==500);++cases;
 }
 // Missing reviewed healing rules reject this one-shot deadline without
 // consuming Reversal or publishing partial damage. Full conversion needs no Seed preflight.
 for(unsigned skill:{205u,222u})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,500,rank,2,false,true);f.rr(rr);f.tick(false);const auto converted=rr<0?0.:std::min(f.damage(),double(effects::reversal_fortune(rr)->maximum_healing));const bool rejected=f.damage()>converted;
  check(std::abs(double(f.hp())-(rejected?500.:500.+converted))<1.01&&f.ally()==500);const auto status=*f.w.status_snapshot(f.target,f.due);check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;})==bool(rr>=0&&rejected));++cases;
 }
 for(unsigned skill:{205u,222u})for(int rr:{0,12,20})for(unsigned rank:{0u,12u,20u})for(bool late:{false,true}){
  Fixture f(skill,10,rank);f.rr(rr);check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,f.cast_at+1000,20000));const auto epoch=*f.w.control_epoch(f.target);f.tick(late);
  check(f.hp()==0&&f.ally()==500&&*f.w.control_epoch(f.target)==epoch+1&&f.w.player_death_times({f.target_id,f.monk_id})[0]==f.due);f.w.tick(f.due+6001);check(*f.w.control_epoch(f.target)==epoch+1);++cases;
 }
 for(unsigned skill:{205u,222u})for(unsigned rank:{0u,12u,20u})for(bool late:{false,true}){
  Fixture f(skill,500,rank);f.w.set_vitals(f.monk_id,{10,1000,100,100},100);check(f.w.apply_condition(f.source,f.monk,combat::conditions::Kind::deep_wound,f.cast_at+1000,20000));const auto epoch=*f.w.control_epoch(f.monk);f.tick(late);
  check(f.ally()==0&&*f.w.control_epoch(f.monk)==epoch+1&&f.w.player_death_times({f.monk_id,f.target_id})[0]==f.due);f.w.tick(f.due+6001);check(*f.w.control_epoch(f.monk)==epoch+1);++cases;
 }
 check(cases==552);std::cout<<"PASS "<<cases<<" timed Lightning Strike/Surge Seed cases: actor order,sparse/frequent,conversion,expiry,strip,source departure,metadata rejection and death deadlines\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
