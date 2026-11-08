#include "gw_world_simulation.h"
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Reversal Mark of Subversion line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;bool npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target,ally;
 Fixture(bool n,unsigned hp,unsigned source_hp):npc(n){
  if(npc){std::vector<world::Instance::Npc> actors;
   for(unsigned id:{100u,101u}){world::Instance::Npc a;a.id=a.definition.id=id;a.x=60;a.definition.flags=8;a.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding={146,id,id,0,0,0,8,0};d.relation=combat::npcs::Relation::hostile_to_players;
    d.packed_allegiance=7;d.spawn_health={id==100?double(hp):1000.,1000};d.damageable=d.conditions_allowed=true;
    d.healing_allowed=d.fleshy=true;d.healing_evidence=d.fleshy_evidence="synthetic Mark recipient";a.combat_definition=d;actors.push_back(a);}
   w.configure_npcs(actors);
  }
  w.join(1,"Source",0,0,box,0,{}, {},100,1);w.set_vitals(1,{source_hp,1000,100,100},100);
  if(!npc)for(unsigned id:{2u,3u}){w.join(id,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{id==2?hp:1000u,1000,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);ally=*w.effect_scope(npc?101:3);
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 unsigned effect(unsigned skill,unsigned at=100){const auto entries=*w.status_snapshot(target,at);for(const auto& e:entries)if(e.skill==skill)return e.instance;return 0;}
 void reversal(int rank){if(rank>=0)check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,5,rank,100,8100,true));}
};
void export_trace(std::ofstream& out,Fixture& f,int rr,unsigned mark,unsigned reversal,double expected,double source){
 if(!out)return;const auto packets=f.box->drain();out<<"{\"npc\":"<<f.npc<<",\"rr\":"<<rr<<",\"mark_effect\":"<<mark<<",\"rr_effect\":"<<reversal<<",\"final\":"<<expected<<",\"source_final\":"<<source<<",\"packets\":[";
 bool first=true;for(const auto& p:packets){if(!first)out<<',';first=false;out<<'\"';constexpr char h[]="0123456789abcdef";for(auto b:p)out<<h[b>>4]<<h[b&15];out<<'\"';}out<<"]}\n";
}
int main(int argc,char** argv)try{
 std::ofstream trace;if(argc==2){trace.open(argv[1]);check(bool(trace));}else check(argc==1);unsigned cases=0;
 using Result=effects::TargetedSpellFailure;
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})
 for(unsigned hp:{10u,300u,1000u})for(unsigned source_hp:{100u,990u,1000u})for(bool other:{false,true})for(bool absorption:{false,true}){
  Fixture f(npc,hp,source_hp);const auto plan=*effects::mark_subversion(rank);check(f.w.stage_mark_subversion(f.source,f.target,plan,100));
  if(absorption)check(npc?f.w.stage_npc_shield_absorption(f.target,12,100):f.w.status_mutation(f.target,1399,81,12,100,6100,true));
  f.reversal(rr);const auto mark=f.effect(127),reversal=f.effect(307);f.box->drain();
  const double converted=rr<0?0:std::min(plan.amount,effects::reversal_fortune(rr)->maximum_healing);
  const double healed=std::min(1000.,hp+converted),stolen=std::min(double(plan.amount)-converted,healed+1);
  const double expected=std::max(0.,healed-stolen),source=std::min(1000.,source_hp+stolen);
  check(f.w.before_spell_effect(f.target,281,1,100));
  check(f.w.mark_subversion_spell_failure(f.target,281,other?f.ally:f.target,1,100)==Result::failed);
  if(f.hp()!=expected||f.w.resources(1)->health!=source)std::cerr<<"npc="<<npc<<" rank="<<rank<<" rr="<<rr<<" hp="<<hp<<" actual="<<f.hp()<<'/'<<f.w.resources(1)->health<<" expected="<<expected<<'/'<<source<<'\n';
  check(f.hp()==expected&&f.w.resources(1)->health==source&&!f.effect(127)&&!f.effect(307));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(expected<=0));
  if(!other&&!absorption&&rank==12&&hp==300&&source_hp==100)export_trace(trace,f,rr,mark,reversal,expected,source);
  f.box->drain();check(f.w.mark_subversion_spell_failure(f.target,281,f.target,1,100)==Result::rejected&&f.box->drain().empty());
  if(expected>0){check(f.w.before_spell_effect(f.target,281,2,100));check(f.w.mark_subversion_spell_failure(f.target,281,f.target,2,100)==Result::none);check(f.hp()==expected&&f.w.resources(1)->health==source);}
  ++cases;
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  Fixture f(npc,10,100);check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
  check(f.w.stage_mark_subversion(f.source,f.target,*effects::mark_subversion(12),100));f.reversal(rr);
  check(f.w.before_spell_effect(f.target,281,1,100));check(f.w.mark_subversion_spell_failure(f.target,281,f.target,1,100)==Result::failed);
  check(f.hp()==0&&f.w.resources(1)->health==100&&!f.effect(127)&&!f.effect(307));if(npc)check(f.w.take_npc_deaths(f.source).size()==1);++cases;
 }
 for(unsigned rr:{0u,12u,20u}){
  Fixture f(false,300,100);check(f.w.stage_mark_subversion(f.source,f.target,*effects::mark_subversion(0),100));
  check(f.w.status_mutation(f.target,299,81,20,100,10100,true));f.reversal(rr);
  check(f.w.before_spell_effect(f.target,281,1,100));check(f.w.mark_subversion_spell_failure(f.target,281,f.target,1,100)==Result::failed);
  check(f.hp()==300&&f.w.resources(1)->health==100&&!f.effect(127)&&f.effect(307));++cases;
 }
 for(bool npc:{false,true})for(unsigned selection=0;selection<4;++selection){
  Fixture f(npc,300,100);check(f.w.stage_mark_subversion(f.source,f.target,*effects::mark_subversion(12),100));f.reversal(12);
  auto selected=std::optional(f.source);if(selection==1)selected.reset();if(selection==2){selected=f.ally;++selected->generation;}
  const auto at=selection==3?6100u:100u;check(f.w.before_spell_effect(f.target,281,1,at));
  check(f.w.mark_subversion_spell_failure(f.target,281,selection==3?std::optional(f.target):selected,1,at)==(selection==2?Result::rejected:Result::none));
  check(f.hp()==300&&f.w.resources(1)->health==100&&bool(f.effect(127,at))==(selection!=3)&&f.effect(307,at));++cases;
 }
 std::cout<<cases<<" Mark conversion cases: allied spell failure and hex consumption,zero input,source cap,wound death,selection/expiry guards and duplicate events passed;paid307 unbound\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
