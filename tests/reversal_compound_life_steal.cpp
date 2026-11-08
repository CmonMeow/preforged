#include "gw_world_simulation.h"
#include <iostream>
#include <fstream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Compound Reversal life steal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target;bool npc;unsigned initial;
 Fixture(bool n,unsigned hp,unsigned source_hp=100):npc(n),initial(hp){
  if(npc){world::Instance::Npc a;a.id=a.definition.id=100;a.x=60;a.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,0};d.spawn_health={double(hp),1000};
   d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=d.conditions_allowed=true;
   d.healing_allowed=d.fleshy=true;d.healing_evidence=d.fleshy_evidence="synthetic living recipient";a.combat_definition=d;w.configure_npcs({a});}
  w.join(1,"Source",0,0,box,0,{}, {},100,1);w.set_vitals(1,{source_hp,1000,100,100},100);
  if(!npc){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
 void absorption(bool primed){
  check(npc?w.stage_npc_shield_absorption(target,12,100):w.status_mutation(target,1399,80,12,100,6100,true));
  if(primed){for(unsigned i=0;i<13;++i){
   if(npc)check(w.damage_npc(source,target,{combat::Kind::armor_ignoring,1,20,42},1248,100).reason==combat::Reason::accepted);
   else { w.set_vitals(2,{1000,1000,100,100},100); check(w.wound_spell(source,target,*effects::wound_spell(1052,0),20,100)); }
  }if(!npc)w.set_vitals(2,{initial,1000,100,100},100);}
 }
 void reversal(int rank){if(rank>=0)check(npc?w.stage_npc_reversal(target,unsigned(rank),100):w.status_mutation(target,307,5,unsigned(rank),100,8100,true));}
 void cover(unsigned index){
  if(index==0)check(npc?w.stage_npc_shield_absorption(target,12,100):w.status_mutation(target,1399,80,12,100,6100,true));
  if(index==1)check(npc?w.stage_npc_shield_deflection(target,*effects::shield_deflection(0),100):w.complete_shield_deflection(target,target,*effects::shield_deflection(0),0,100));
  if(index==2)check(npc?w.stage_npc_vigorous_spirit(target,*effects::vigorous_spirit(0),100):w.complete_vigorous_spirit(target,target,*effects::vigorous_spirit(0),0,100));
 }
 void hex(){check(w.degeneration_hex(source,target,*effects::degeneration_hex(19,0),100));}
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 bool feast(unsigned rank){const auto anchor=effects::acquire_feast_anchor(source,{target,60,0,0,true,100},100);check(anchor.has_value());return w.feast_of_corruption(source,*anchor,*effects::feast_of_corruption(rank),100);}
 unsigned rr_effect(){const auto entries=*w.status_snapshot(target,100);for(const auto& e:entries)if(e.skill==307)return e.instance;return 0;}
};
struct Expected{double target,source;bool reversal;};
Expected expected(unsigned initial,unsigned source,double damage,unsigned amount,bool follow,int rank){
 Expected e{double(initial),double(source),rank>=0};
 const auto convert=[&](double input){
  if(input<=0||!e.reversal)return input;
  const auto converted=std::min(input,double(effects::reversal_fortune(unsigned(rank))->maximum_healing));
  e.target=std::min(1000.,e.target+converted);e.reversal=false;return input-converted;
 };
 const auto remaining=convert(damage);e.target=std::max(0.,e.target-remaining);
 if(e.target>0&&follow){const auto reduced=convert(amount),stolen=std::min(reduced,e.target+1);e.source+=std::min(stolen,1000.-e.source);e.target=std::max(0.,e.target-stolen);}
 return e;
}
void verify(Fixture& f,const Expected& e,unsigned skill,unsigned rank,int rr){
 if(f.hp()!=e.target||f.w.resources(1)->health!=e.source)std::cerr<<"skill="<<skill<<" rank="<<rank<<" rr="<<rr<<" npc="<<f.npc<<" initial="<<f.initial<<" actual="<<f.hp()<<'/'<<f.w.resources(1)->health<<" expected="<<e.target<<'/'<<e.source<<'\n';
 check(f.hp()==e.target&&f.w.resources(1)->health==e.source);check(bool(f.rr_effect())==e.reversal);
 if(f.npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(e.target<=0));
}
void export_trace(std::ofstream& out,Fixture& f,unsigned skill,int rr,unsigned effect,unsigned amount,const Expected& e){
 if(!out)return;const auto packets=f.box->drain();
 out<<"{\"skill\":"<<skill<<",\"npc\":"<<f.npc<<",\"rr\":"<<rr<<",\"effect\":"<<effect<<",\"initial\":"<<f.initial<<",\"amount\":"<<amount<<",\"final\":"<<e.target<<",\"source_final\":"<<e.source<<",\"packets\":[";
 bool first=true;for(const auto& p:packets){if(!first)out<<',';first=false;out<<'\"';constexpr char hex[]="0123456789abcdef";for(auto b:p)out<<hex[b>>4]<<hex[b&15];out<<'\"';}out<<"]}\n";
}
int main(int argc,char** argv)try{
 std::ofstream trace;if(argc==2){trace.open(argv[1]);check(bool(trace));}else check(argc==1);unsigned cases=0;
 for(unsigned skill:{102u,1067u})for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})
 for(unsigned hp:{500u,600u,1000u})for(unsigned source_hp:{100u,990u,1000u})for(bool primed:{false,true}){
  Fixture f(npc,hp,source_hp);f.absorption(primed);f.reversal(rr);const auto effect=f.rr_effect();f.box->drain();
  const auto plan=*effects::shadow_damage(skill,rank);const double damage=std::max(0.,double(plan.damage)-(primed?70:5));
  const auto e=expected(hp,source_hp,damage,plan.steal_above_half,hp>500,rr);
  check(f.w.shadow_damage(f.source,f.target,plan,20,100));verify(f,e,skill,rank,rr);
  if(primed&&rank==12&&hp==600&&source_hp==100)export_trace(trace,f,skill,rr,effect,plan.steal_above_half,e);++cases;
 }
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{10u,300u,1000u})
 for(unsigned source_hp:{100u,990u,1000u})for(bool primed:{false,true})for(bool hexed:{false,true}){
  Fixture f(npc,hp,source_hp);f.absorption(primed);if(hexed)f.hex();f.reversal(rr);const auto effect=f.rr_effect();f.box->drain();
  const auto plan=*effects::feast_of_corruption(rank);const double damage=std::max(0.,double(plan.damage)-(primed?70:5));
  const auto e=expected(hp,source_hp,damage,plan.steal,hexed,rr);check(f.feast(rank));verify(f,e,151,rank,rr);check(f.w.feast_late_refusals()==0);
  if(primed&&hexed&&rank==12&&hp==300&&source_hp==100)export_trace(trace,f,151,rr,effect,plan.steal,e);++cases;
 }
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,300u,1000u})
 for(unsigned source_hp:{100u,990u,1000u})for(unsigned mode=0;mode<4;++mode){
  Fixture f(npc,hp,source_hp);const auto plan=*effects::enchantment_removal(143,rank);check(plan.count<=3);
  if(mode==1||mode==2)f.reversal(rr);
  if(mode>=2)for(unsigned i=0;i<plan.count;++i)f.cover(i);
  if(mode==3)f.reversal(rr);
  const auto effect=f.rr_effect();f.box->drain();const bool any=mode>=2?plan.count>0:mode==1&&rr>=0;
  const bool removed=plan.count>0&&any;const int remains=(mode==2||!removed)&&mode!=0?rr:-1;
  const auto e=expected(hp,source_hp,0,plan.steal,removed,remains);
  check(f.w.enchantment_removal(f.source,f.target,plan,20,100));verify(f,e,143,rank,rr);
  if(mode==2&&rank==12&&hp==300&&source_hp==100)export_trace(trace,f,143,rr,effect,plan.steal,e);++cases;
 }
 // Pending wound debt can turn the residual conversion heal lethal. Do not
 // fabricate a source payout/loss after that real healing event.
 for(unsigned skill:{151u,143u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  Fixture f(npc,10);if(skill==151)f.absorption(true);
  if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));}
  else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));
  if(skill==151)f.hex();f.reversal(rr);
  const auto strip=*effects::enchantment_removal(143,12);if(skill==143)for(unsigned i=0;i<strip.count;++i)f.cover(i);
  const auto before=f.w.resources(1)->health;check(skill==151?f.feast(12):f.w.enchantment_removal(f.source,f.target,strip,20,100));
  check(f.hp()==0&&f.w.resources(1)->health==before);if(npc)check(f.w.take_npc_deaths(f.source).size()==1);++cases;
 }
 std::cout<<cases<<" compound transfer cases: shadow qualification, Feast hex/zero-first-hit, Strip selection and residual conversion, source cap, current ownership and healing death passed; paid307 unbound\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
