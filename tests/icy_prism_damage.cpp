#include "gw_icy_prism.h"
#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Icy Prism damage line "+std::to_string(at.line()));
}
combat::Actor actor(Scope scope,int armor=60){combat::Actor a;a.id=scope.actor;a.generation=scope.generation;a.health=combat::Health{1000,1000};a.damageable=true;a.cold_armor=armor;return a;}
int main()try{
 const Scope caster{1,1,1},target{1,2,1},other{1,100,3};Registry registry(target);
 check(registry.apply(target,{1,1,209,12,Category::hex,1000,4000},Replace::same_skill).accepted);
 const auto plan=*icy_prism(12);const IcyPrismAnchor anchor{caster,target,100,0,0,registry};
 const std::vector<IcyPrismRecipient> candidates{{target,100,0,0,true,true,true,60},{other,200,0,0,true,true,true,60}};
 const auto selection=select_icy_prism(plan,anchor,candidates,2000);check(selection&&selection->recipients.size()==2);
 for(unsigned mode=0;mode<6;++mode){
  std::vector<IcyPrismDamageInput> inputs{{target,actor(target),{}},{other,actor(other),{}}};
  if(mode==1)inputs[1].actor.cold_armor=100;
  if(mode==2)inputs[1].actor.incoming_reduction=15;
  if(mode==3)inputs[1].actor.incoming_damage_multiplier=2;
  if(mode==4){inputs[1].actor.elemental_protection=combat::Kind::cold;inputs[1].actor.elemental_multiplier=.5;}
  if(mode==5){inputs[1].actor.cold_armor=100;check(inputs[1].conditions.apply(combat::conditions::Kind::cracked_armor,1,1000,5000));}
  const auto hits=prepare_icy_prism_damage(*selection,inputs,20,2000);check(hits&&hits->size()==2&&(*hits)[0].owner==target&&(*hits)[0].hit.applied==63);
  const double expected=mode==1?31.5:mode==2?48:mode==3?126:mode==4?31.5:mode==5?63/std::sqrt(2.):63;
  check(std::abs((*hits)[1].hit.applied-expected)<1e-7);
  check(inputs[0].actor.health->current==1000&&inputs[1].actor.health->current==1000);
 }
 std::vector<IcyPrismDamageInput> inputs{{target,actor(target),{}},{other,actor(other),{}}};
 for(unsigned mode=0;mode<5;++mode){auto bad=inputs;
  if(mode==0)bad[1].actor.cold_armor.reset();if(mode==1)++bad[1].actor.generation;
  if(mode==2)bad[1].actor.health->current=0;if(mode==3)bad[1].actor.incoming_damage_multiplier=std::numeric_limits<double>::quiet_NaN();
  if(mode==4)bad[1]=bad[0];check(!prepare_icy_prism_damage(*selection,bad,20,2000));check(bad[0].actor.health->current==1000);
 }
 auto duplicated=*selection;duplicated.recipients.push_back(target);auto duplicate_inputs=inputs;duplicate_inputs.push_back(inputs[0]);check(!prepare_icy_prism_damage(duplicated,duplicate_inputs,20,2000));
 auto forged=*selection;forged.expanded=false;check(!prepare_icy_prism_damage(forged,inputs,20,2000));
 // The same reviewed damage intent goes through existing NPC Health authority.
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
 d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.cold_armor=60;
 const combat::npcs::Lifetime life{1,2,3,146,100};combat::npcs::LiveActor npc(d,life);
 const auto hit=npc.apply_damage(life,{combat::Kind::cold,63,20,903},2000);check(hit.reason==combat::Reason::accepted&&hit.applied==63&&npc.actor(life)->health->current==937);
 std::cout<<"PASS Icy Prism copy-only all-recipient cold damage preflight,once-only primary,armor/protection/CrackedArmor,atomic rejection and existing NPC Health authority;World/Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
