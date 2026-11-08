#include "gw_reversal_npc_wire.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC Reversal line "+std::to_string(at.line()));
}
combat::npcs::LiveActor make(unsigned hp,std::optional<bool> healing=true){
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
 d.spawn_health=combat::Health{double(hp),1000};d.damageable=true;d.conditions_allowed=true;
 d.relation=combat::npcs::Relation::hostile_to_players;d.healing_allowed=healing;
 if(healing.has_value())d.healing_evidence="synthetic trait";
 return combat::npcs::LiveActor(d,{1,1,1,146,100});
}
int main(int argc,char** argv)try{
 check(argc<=2);std::ofstream fixture;if(argc==2)fixture.open(argv[1]);
 unsigned count=0;
 for(unsigned rank=0;rank<=20;++rank)for(unsigned hp:{1u,300u,1000u})for(double damage:{10.,67.,200.}){
  ++count;auto a=make(hp);const auto life=a.lifetime();
  check(a.apply_reversal_fortune(life,rank,1,1,100));
  const auto receipt=a.apply_reversal_damage(life,{combat::Kind::armor_ignoring,damage,20,42},200,true);
  const double converted=std::min(damage,double(skills::effects::reversal_fortune(rank)->maximum_healing));
  const double healed=std::min(1000.,hp+converted),expected=std::max(0.,healed-(damage-converted));
  check(receipt&&receipt->damage&&receipt->removed.skill==307&&receipt->before_healing.current==hp&&receipt->after_healing.current==healed);
  check(receipt->damage->before.current==healed&&receipt->damage->damage==damage-converted&&a.actor(life)->health->current==expected);
  check(!a.owned_effect_snapshot(life)->newest_skill(a.hex_scope(),307,200));
  check(!a.apply_reversal_damage(life,{combat::Kind::armor_ignoring,damage,20,42},200,true));
 }
 check(count==189);
 for(unsigned hp:{10u,1000u}){auto a=make(hp);const auto life=a.lifetime();
  check(a.apply_reversal_fortune(life,12,1,1,100));
  const auto receipt=a.apply_reversal_damage(life,{combat::Kind::armor_ignoring,72,20,42},200,true);check(receipt.has_value());
  const auto packets=combat::npcs::reversal_damage_packets_2022(100,2,*receipt);
  check(a.actor(life)->health->current==(hp==10?72:995));
  check(packets.front().size()==10&&packets.front()[0]==67);
  if(fixture){fixture<<"{\"initial_health\":"<<hp<<",\"final_health\":"<<a.actor(life)->health->current<<",\"packets\":[";
   bool comma=false;for(const auto& packet:packets){if(comma)fixture<<',';comma=true;fixture<<'\"';for(auto b:packet)fixture<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(b);fixture<<std::dec<<'\"';}fixture<<"]}\n";}
 }

 {auto a=make(500);const auto life=a.lifetime();
  check(a.apply_protective_spirit(life,{1,1,1},*skills::effects::protective_spirit(12),2,100));
  check(a.apply_reversal_fortune(life,12,1,1,100));
  const auto r=a.apply_reversal_damage(life,{combat::Kind::armor_ignoring,1000,20,42},200,true);
  check(r&&r->damage&&r->after_healing.current==567&&r->damage->damage==33&&a.actor(life)->health->current==534);
 }
 for(unsigned hp:{1u,500u}){auto a=make(hp);const auto life=a.lifetime();
  check(a.apply_condition(life,combat::conditions::Kind::deep_wound,1,100,10000));
  const auto before=a.actor(life)->health->current;
  combat::deep_wound::State wound;double expected=hp;wound.reset(1000);wound.capacity(expected,1000,true);
  check(expected==before&&a.apply_reversal_fortune(life,12,1,1,100));
  wound.event(expected,42,true);const bool healing_death=expected<=0;
  const auto r=a.apply_reversal_damage(life,{combat::Kind::armor_ignoring,42,20,42},200,true);
  check(r&&r->damage.has_value()!=healing_death&&a.actor(life)->health->current==expected);
  check(bool(a.actor(life)->effects&0x10)==healing_death);
  const auto packets=combat::npcs::reversal_damage_packets_2022(100,2,*r);
  if(healing_death)check(packets.size()==2&&packets.back()[0]==161);

 }
 unsigned reduced_cases=0;
 for(unsigned hp:{10u,1000u})for(int armor:{60,80,100})for(double reduction:{0.,10.,50.}){
  ++reduced_cases;auto a=make(hp);const auto life=a.lifetime();check(a.apply_reversal_fortune(life,12,1,1,100));
  auto actor=*a.actor(life);actor.lightning_armor=armor;actor.incoming_reduction=reduction;
  const auto hit=combat::apply(actor,{combat::Kind::lightning,100,20,222,25},true);
  check(hit.reason==combat::Reason::accepted&&hit.damage>0);
  const double converted=std::min(hit.damage,67.),expected=std::max(0.,std::min(1000.,hp+converted)-(hit.damage-converted));
  const auto r=a.apply_reversal_hit(life,hit,200);check(r&&r->damage&&r->quote.application);
  check(std::abs(a.actor(life)->health->current-expected)<1e-8&&r->quote.reduced_amount==hit.damage);
 }
 check(reduced_cases==18);
 for(unsigned mode=0;mode<3;++mode){auto current=make(500);const auto life=current.lifetime();
  check(current.apply_reversal_fortune(life,12,1,1,100));auto proposal=current;
  const auto receipt=proposal.apply_reversal_damage(life,{combat::Kind::armor_ignoring,72,20,42},200,true);check(receipt.has_value());
  auto unconsumed=current;check(!unconsumed.preserve_current_hexes(current,life,200,{receipt->quote}));
  if(mode==1){check(current.remove_hex_effect(life,1,100));check(current.apply_reversal_fortune(life,12,1,1,100));}
  if(mode==2)check(current.remove_hex_effect(life,1,100));
  check(current.apply_aura_restoration(life,*skills::effects::aura_restoration_plan(0),2,100));
  const auto before=proposal.actor(life)->health->current;
  check(proposal.preserve_current_hexes(current,life,200,{receipt->quote})==(mode==0));
  check(proposal.actor(life)->health->current==before);
  if(mode==0){check(proposal.owned_effect_snapshot(life)->newest_skill(proposal.hex_scope(),180,200).has_value());
   check(!proposal.owned_effect_snapshot(life)->newest_skill(proposal.hex_scope(),307,200));}
  else check(current.owned_effect_snapshot(life)->newest_skill(current.hex_scope(),307,200).has_value()==(mode==1));
 }
 {auto a=make(500);const auto life=a.lifetime();check(a.apply_reversal_fortune(life,12,1,1,100));
  auto actor=*a.actor(life);const auto hit=combat::apply(actor,{combat::Kind::armor_ignoring,72,20,42},true);
  for(unsigned mode=0;mode<4;++mode){auto forged=hit;
   if(mode==0)++forged.generation;if(mode==1)forged.applied=0;if(mode==2)forged.before.current=499;if(mode==3)forged.skill=3433;
   check(!a.apply_reversal_hit(life,forged,200));check(a.actor(life)->health->current==500);
  }
  check(a.apply_reversal_hit(life,hit,200).has_value());
 }
 for(auto permission:{std::optional<bool>{},std::optional<bool>{false}}){auto a=make(500,permission);const auto life=a.lifetime();
  check(!a.apply_reversal_fortune(life,12,1,1,100)&&a.actor(life)->health->current==500);}
 {auto a=make(500);const auto life=a.lifetime();check(a.apply_reversal_fortune(life,12,1,1,100));
  auto stale=life;++stale.actor_generation;
  check(!a.apply_reversal_damage(stale,{combat::Kind::armor_ignoring,50,20,42},200,true));
  check(!a.apply_reversal_damage(life,{combat::Kind::lightning,50,20,222},200,true));
  check(!a.apply_reversal_damage(life,{combat::Kind::armor_ignoring,0,20,42},200,true));
  check(!a.apply_reversal_damage(life,{combat::Kind::armor_ignoring,50,20,42},200,false));
  check(a.actor(life)->health->current==500&&a.owned_effect_snapshot(life)->newest_skill(a.hex_scope(),307,200));
  check(!a.apply_reversal_damage(life,{combat::Kind::armor_ignoring,50,20,42},8100,true));
  check(a.actor(life)->health->current==500);
 }
 std::cout<<"PASS189 NPC Reversal healing-before-loss cases,permissions,scope,armor,zero,expiry;full NPC consumer integration/paid307 remain pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
