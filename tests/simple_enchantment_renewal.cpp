#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Simple enchantment Renewal line "+std::to_string(at.line())+" "+context);}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target;unsigned id;
 Fixture(unsigned kind,unsigned version,bool known=true):id(kind==0?1:kind==1?2:100){
  if(kind==2){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={100,1000};if(known){d.healing_allowed=true;d.healing_evidence="authored test ally";}n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);w.set_vitals(1,{100,1000,50,100},100);
  w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{100,1000,50,100},100);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  source=*w.effect_scope(1);target=*w.effect_scope(id);check(w.ordinary_weapon(source,combat::weapon::Owned{123,1,{20,20,2},12,false,false,27},20));if(version){const auto p=*effects::ether_renewal(version,0);check(w.status_mutation(source,version,80,0,100,100+p.duration,true));}box->drain();
 }
 bool complete(unsigned skill,unsigned rank,unsigned divine,std::uint64_t event,std::uint64_t now){
  switch(skill){
   case 245:{auto p=effects::protective_spirit(rank);return p&&w.complete_protective_spirit(source,target,*p,divine,now,nullptr,event);}
   case 254:{auto p=effects::vigorous_spirit(rank);return p&&w.complete_vigorous_spirit(source,target,*p,divine,now,nullptr,event);}
   case 259:{auto p=effects::shield_deflection(rank);return p&&w.complete_shield_deflection(source,target,*p,divine,now,nullptr,event);}
   case 261:{auto p=effects::shield_regeneration(rank);return p&&w.complete_shield_regeneration(source,target,*p,divine,now,nullptr,event);}
   case 262:{auto p=effects::shield_judgment(rank);return p&&w.complete_shield_judgment(source,target,*p,divine,now,nullptr,event);}
   case 274:{auto p=effects::healing_seed(rank);return p&&w.complete_healing_seed(source,target,*p,divine,now,nullptr,event);}
   case 310:{auto p=effects::divine_spirit(rank);return p&&source==target&&w.complete_divine_spirit(source,*p,divine,now,nullptr,event);}
  }return false;
 }
};
int main()try{
 unsigned manual=0,paid=0;
 for(unsigned skill:{245u,254u,259u,261u,262u,274u,310u})for(unsigned kind=0;kind<3;++kind){
  if((skill==274&&kind==0)||(skill==310&&kind!=0))continue;
  for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<5;++mode){
   context="manual skill="+std::to_string(skill)+" kind="+std::to_string(kind)+" version="+std::to_string(version)+" mode="+std::to_string(mode);
   Fixture f(kind,version,mode!=3);const auto at=mode==4&&version?100+effects::ether_renewal(version,0)->duration:200;
   f.w.tick(at);check(f.w.before_spell_effect(f.source,skill,1,at));f.box->drain();const auto before=*f.w.resources(1);
   const unsigned divine=mode==3?12:0;
   bool ok=f.complete(skill,mode==2?21:0,divine,mode==1?2:1,at);
   const bool unknown=mode==3&&kind==2;
   if(mode==1||mode==2||unknown){check(!ok&&f.box->drain().empty());check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);
    if(unknown){++manual;continue;}ok=f.complete(skill,0,divine,1,at);}
   check(ok);const bool reward=version&&mode!=4;
   const unsigned self_heal=divine&&kind==0?38:0;
   check(f.w.resources(1)->health==before.health+(reward?5:0)+self_heal&&f.w.resources(1)->energy==before.energy+(reward?1:0));
   const auto packets=f.box->drain();const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish!=packets.end());check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
   if(reward){const auto heal=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[2]==55;});check(heal<finish);}
   check(!f.complete(skill,0,divine,1,at)&&f.box->drain().empty());++manual;
  }
  for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<5;++mode){
   context="paid skill="+std::to_string(skill)+" kind="+std::to_string(kind)+" version="+std::to_string(version)+" mode="+std::to_string(mode);
   Fixture f(kind,version);casting::Hooks h;casting::Runtime cast(1);unsigned calls=0;std::uint64_t admitted=0;unsigned before_health=0,before_energy=0;
   h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){f.box->enqueue(p);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto,float,bool){return true;};
   h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
   h.before_spell_effect=[&](auto id,auto event,auto at){f.w.tick(at);before_health=f.w.resources(1)->health;before_energy=f.w.resources(1)->energy;admitted=event;return f.w.before_spell_effect(f.source,id,event,at);};
   h.ally_enchantment_target_valid=[&](auto target,unsigned df,auto at){return f.w.valid_ally_enchantment_target(f.source,target,df,at);};h.healing_seed_target_valid=[&](auto target,unsigned df,auto at){return f.w.valid_healing_seed_target(f.source,target,df,at);};h.shield_judgment_ready=[](auto){return true;};h.enchantment_duration_ready=[](auto,auto,auto){return true;};
   const auto legacy=[](auto&&...)->casting::EffectCompletion{throw std::runtime_error("Typed enchantment downgraded");};h.protective_spirit=legacy;h.vigorous_spirit=legacy;h.shield_deflection=legacy;h.shield_regeneration=legacy;h.shield_judgment=legacy;h.healing_seed=legacy;h.divine_spirit=legacy;
   h.simple_enchantment_completed=[&](auto target,auto used,auto rank,auto divine,auto event,auto at)->casting::EffectCompletion{check(target==f.target&&used==skill&&event==admitted&&event);++calls;const bool ok=f.complete(skill,rank,divine,event,at);return {ok,{},{},{},false,true};};
   Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
   const auto reason=cast.start(100,{skill,0,f.id},bar,policy,ranks,true,h);check(reason==casting::Reason::accepted);
   if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(101,h);if(mode==3)h.simple_enchantment_completed={};if(mode==4)h.before_spell_effect={};
   const auto at=100+static_cast<std::uint64_t>(find_skill(skill)->activation*1000);f.box->drain();cast.update(at,h);const auto packets=f.box->drain();cast.update(at+10000,h);check(calls==(mode==0?1u:0u));
   if(mode==0){check(f.w.resources(1)->health==before_health+(version?5:0)&&f.w.resources(1)->energy==before_energy+(version?1:0));check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);}
   ++paid;
  }
 }
 std::cout<<manual<<" shared enchantment reward/admission/expiry/immunity cases and "<<paid<<" paid seven-skill completion/cancel/interrupt/hook-loss casts pass\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
