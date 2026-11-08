#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Area Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;unsigned skill,primary_id,side_id;bool mode;effects::Scope source,primary,side,monk;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned id,unsigned hp,unsigned rank,bool expanded,unsigned main=2,bool unknown=false):skill(id),primary_id(main),side_id(main==2?3:2),mode(expanded){
  if(unknown){world::Instance::Npc n;n.id=n.definition.id=100;n.x=740;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;d.creature_type="spirit";d.creature_type_evidence="authored unsupported Seed gain recipient";d.fleshy=false;d.fleshy_evidence="authored non-fleshy spirit";n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned actor:{1u,2u,3u,4u}){w.join(actor,"Actor",actor==1?0.f:actor==primary_id?500.f:actor==side_id?620.f:770.f,0,actor==primary_id?box:std::make_shared<world::Outbox>(),0,{}, {},100,actor==1?1:2);
   w.set_vitals(actor,{actor==1?1000u:actor==side_id?hp:500u,1000,actor==1?50u:100u,100},100);check(w.elemental_armor(*w.effect_scope(actor),60,60));check(w.player_holy_equipment(*w.effect_scope(actor),0));}
  source=*w.effect_scope(1);primary=*w.effect_scope(primary_id);side=*w.effect_scope(side_id);monk=*w.effect_scope(4);
  check(w.party_action(side_id,158,4,100)&&w.party_action(4,154,side_id,100));
  check(w.complete_healing_seed(monk,side,*effects::healing_seed(rank),0,100));
  if(skill==903&&mode)check(w.movement_hex(source,primary,*effects::movement_hex(210,0),100));
  if((skill==1950||skill==2093)&&mode){check(w.apply_condition(source,primary,combat::conditions::Kind::blind,100,10000));check(w.apply_condition(source,side,combat::conditions::Kind::blind,100,10000));}
  box->drain();
 }
 bool side_hit()const{return skill!=903||mode;}
 double damage()const{if(skill==903)return effects::icy_prism(12)->damage;if(skill==294||skill==2887)return effects::signet_judgment(12,skill==2887)->damage;return effects::signet_of_corruption(skill,6)->damage;}
 void rr(int rank){if(rank>=0)check(w.status_mutation(side,307,900,rank,100,8100,true));box->drain();}
 bool cast(){if(skill==903)return w.stage_icy_prism(source,primary,*effects::icy_prism(12),20,200);if(skill==294||skill==2887)return w.complete_signet_judgment(source,primary,*effects::signet_judgment(12,skill==2887),20,200);return w.signet_of_corruption(source,primary,*effects::signet_of_corruption(skill,6),20,200);}
 unsigned hp()const{return w.resources(side_id)->health;}
 unsigned main_hp()const{return w.resources(primary_id)->health;}
 unsigned ally()const{return w.resources(4)->health;}
};
int main()try{
 unsigned cases=0;
 for(unsigned skill:{903u,294u,2887u,1950u,2093u})for(bool mode:{false,true})for(unsigned primary:{2u,3u})for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,500u,1000u})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,hp,rank,mode,primary);f.rr(rr);check(f.cast());const auto raw=f.side_hit()?f.damage():0.,converted=rr<0?0.:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));
  const auto healed=std::min(1000.,hp+converted),loss=std::min(healed,raw-converted);const bool triggers=loss>0;const auto heal=effects::healing_seed(rank)->healing;
  const auto wanted=healed-loss>0?std::min(1000.,healed-loss+(triggers?heal:0)):0.;
  if(std::abs(double(f.hp())-wanted)>=1.01)throw std::runtime_error("skill="+std::to_string(skill)+" main="+std::to_string(primary)+" actual="+std::to_string(f.hp())+" wanted="+std::to_string(wanted));
  check(f.main_hp()==500-f.damage()+(triggers?heal:0)&&f.ally()==500+(triggers?heal:0));
  if(skill==294||skill==2887)check(f.w.knocked_down(f.primary,200)&&!f.w.knocked_down(f.side,200));
  if(skill==1950||skill==2093)check(f.w.resources(1)->energy==50+(mode?4u:0u));
  if(!wanted)check(f.w.player_death_times({f.side_id,4})[0]==200);f.w.tick(201);check(f.ally()==500+(triggers?heal:0));++cases;
 }
 // Two Seed effects retain existing provider order without restoring old Health snapshots.
 for(unsigned skill:{903u,294u,2887u,1950u,2093u})for(unsigned primary:{2u,3u})for(int rr:{-1,0,12,20})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,500,rank,true,primary);f.rr(rr);check(f.w.complete_healing_seed(f.monk,f.primary,*effects::healing_seed(rank),0,100));check(f.cast());
  const auto raw=f.damage(),converted=rr<0?0.:std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));const bool second=raw>converted;const auto heal=effects::healing_seed(rank)->healing;
  check(f.main_hp()==500-raw+heal+(second?heal:0)&&f.hp()==500+converted-(raw-converted)+heal+(second?heal:0)&&f.ally()==500+(second?heal:0));++cases;
 }
 // A failing later Seed preflight must leave every earlier area target untouched.
 for(unsigned skill:{903u,294u,2887u,1950u,2093u})for(unsigned primary:{2u,3u})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0}){
  Fixture f(skill,500,rank,true,primary,true);f.rr(rr);check(!f.cast());check(f.main_hp()==500&&f.hp()==500&&f.ally()==500&&f.box->drain().empty()&&f.w.resources(1)->energy==50);
  const auto status=*f.w.status_snapshot(f.side,200);check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;})==bool(rr>=0));
  check(f.w.status_mutation(f.side,307,901,20,200,8200,true));check(f.cast());check(f.main_hp()==500-f.damage()&&f.hp()==500+f.damage()&&f.ally()==500);++cases;
 }
 for(unsigned skill:{903u,294u,2887u,1950u,2093u})for(unsigned primary:{2u,3u})for(unsigned rank:{0u,12u,20u})for(int rr:{0,12,20}){
  Fixture f(skill,10,rank,true,primary);f.rr(rr);check(f.w.apply_condition(f.source,f.side,combat::conditions::Kind::deep_wound,100,10000));const auto epoch=*f.w.control_epoch(f.side);check(f.cast());
  check(f.hp()==0&&f.main_hp()==500-f.damage()&&f.ally()==500&&*f.w.control_epoch(f.side)==epoch+1);f.w.tick(201);check(*f.w.control_epoch(f.side)==epoch+1);++cases;
 }
 for(unsigned skill:{903u,294u,2887u,1950u,2093u})for(unsigned primary:{2u,3u})for(unsigned rank:{0u,12u,20u}){
  Fixture f(skill,500,rank,true,primary);f.w.set_vitals(4,{10,1000,100,100},100);check(f.w.apply_condition(f.source,f.monk,combat::conditions::Kind::deep_wound,100,10000));const auto epoch=*f.w.control_epoch(f.monk);check(f.cast());
  check(f.ally()==0&&*f.w.control_epoch(f.monk)==epoch+1&&f.w.player_death_times({4,f.side_id})[0]==200);f.w.tick(201);check(*f.w.control_epoch(f.monk)==epoch+1);++cases;
 }
 check(cases==1260);std::cout<<"PASS "<<cases<<" Icy Prism/Judgment/Corruption Seed cases: conditional area,variants,source energy,primary control,actor order,conversion,survival and whole-area admission\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
