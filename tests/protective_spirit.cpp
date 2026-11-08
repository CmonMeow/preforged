#include "gw_world_simulation.h"
#include "gw_protective_spirit.h"
#include "gw_chilblains_npc.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Protective Spirit line "+std::to_string(at.line()));}
combat::npcs::LiveActor make_npc(unsigned maximum=55,combat::npcs::Relation relation=combat::npcs::Relation::hostile_to_players){combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=relation;d.spawn_health=combat::Health{double(maximum),maximum};d.earth_armor=d.cold_armor=d.fire_armor=60;d.damageable=d.conditions_allowed=true;return {d,{1,1,1,146,100}};}
int main()try{using namespace skills::effects;
 const Scope caster{1,1,1};unsigned cases=0;
 for(unsigned rank=0;rank<=20;++rank)for(unsigned mode=0;mode<10;++mode){++cases;auto n=make_npc(55,mode==8?combat::npcs::Relation::neutral:combat::npcs::Relation::hostile_to_players);const auto life=n.lifetime();const auto plan=*protective_spirit(rank);
  auto bad=plan;++bad.maximum_health_percent;check(!n.apply_protective_spirit(life,caster,bad,1,1000));auto stale=life;++stale.actor_generation;check(!n.apply_protective_spirit(stale,caster,plan,1,1000));
  check(n.apply_protective_spirit(life,caster,plan,1,1000));check(!n.apply_protective_spirit(life,caster,plan,1,1000));
  if(mode==0){const auto hit=n.apply_damage(life,{combat::Kind::cold,200,20,144},1000);check(hit.reason==combat::Reason::accepted&&hit.damage==5.5);}
  if(mode==1){const auto hit=n.apply_weapon_damage(life,200,1000);check(hit&&hit->damage==5.5);}
  if(mode==2){const auto hit=n.apply_caster_weapon_damage(life,200,5,1000);check(hit&&hit->damage==5.5);}
  if(mode==3){const auto hit=n.apply_shadow_damage(life,*shadow_damage(219,12),{100,1000},20,1000);check(hit&&hit->hit.damage==5.5);}
  if(mode==4){const auto hit=n.apply_earth_aoe(life,*caster_earth_aoe(174,12),0,20,1,1000);check(hit&&hit->accepted&&hit->hits.size()==1&&hit->hits[0].damage==5.5);}
  if(mode==5){const auto hit=n.apply_elemental(life,*elemental(191,12),20,1,1000);check(hit&&hit->accepted&&hit->hits[0].damage==5.5);}
  if(mode==6){const auto hit=n.apply_life_steal(life,*life_steal(153,12),{100,1000});check(hit&&hit->stolen==52);}
  if(mode==7){const auto plan=*feast_of_corruption(12);const auto hit=n.apply_damage(life,{combat::Kind::armor_ignoring,double(plan.damage),20,151},1000);check(hit.reason==combat::Reason::accepted&&hit.damage==5.5);}
  if(mode==8){const auto hit=n.apply_guardian_weapon_damage(life,200,1000);check(hit&&hit->damage==5.5);}
  if(mode==9){check(n.remove_reviewed_enchantment(life,1,1000));const auto hit=n.apply_weapon_damage(life,30,1000);check(hit&&hit->damage==30);}
 }
 for(unsigned maximum:{1u,55u,101u,480u,1000u}){auto n=make_npc(maximum);const auto life=n.lifetime();const auto plan=*protective_spirit(12);check(n.apply_protective_spirit(life,caster,plan,1,1000));
  check(!n.apply_weapon_damage(life,30,999));const auto ended=n.apply_weapon_damage(life,30,1000+plan.duration);check(ended&&ended->damage==30);
 }
 {combat::Actor actor{1,1,combat::Health{1000,1000},60,0,true};actor.cold_armor=60;actor.incoming_damage_cap=100;actor.incoming_damage_multiplier=2;actor.elemental_protection=combat::Kind::cold;actor.elemental_multiplier=.5;actor.incoming_reduction=10;
  check(combat::apply(actor,{combat::Kind::cold,200,20,144},true).damage==40); // Amplify,cap,Mantra,flat.
  actor.incoming_damage_cap=std::numeric_limits<double>::quiet_NaN();const auto before=actor.health->current;check(combat::apply(actor,{combat::Kind::cold,200,20,144},true).reason==combat::Reason::invalid&&actor.health->current==before);
 }
 {auto n=make_npc();const auto life=n.lifetime();check(n.apply_protective_spirit(life,caster,*protective_spirit(12),1,1000));
  const std::vector<ChilblainsNpcRecipient> inputs{{n,life,0}};
  const auto proposal=prepare_chilblains_npcs(*chilblains(12),caster,20,{},inputs,captured_chilblains_order,1000);
  check(proposal&&proposal->recipients.size()==1&&proposal->recipients[0].hit.damage==5.5&&proposal->recipients[0].removed.size()==1&&proposal->recipients[0].removed[0].skill==245);
  auto removed=proposal->recipients[0].actor;check(removed.apply_weapon_damage(life,30,1000)->damage==30);
 }
 {auto n=make_npc();const auto life=n.lifetime();check(n.apply_protective_spirit(life,caster,*protective_spirit(12),1,1000));
  check(n.apply_condition(life,combat::conditions::Kind::deep_wound,1,1000,10000));
  const auto hit=n.apply_weapon_damage(life,30,1000);check(hit&&std::abs(hit->damage-4.4)<1e-9);
 }
 for(bool chilblains:{false,true}){world::Instance w;
  w.join(1,"Foe",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Protected",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{1000,1000,100,100},100);w.set_vitals(2,{55,55,100,100},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.elemental_armor(target,60,60));check(w.status_mutation(target,245,444,12,1000,20000,true));
  if(chilblains){check(w.complete_chilblains(source,target,*skills::effects::chilblains(12),20,1000));check(!w.has_enchantment(source,2,1000));}
  else check(w.shadow_damage(source,target,*skills::effects::shadow_damage(219,12),20,1000));
  check(w.resources(2)->health==50);
  if(!chilblains)check(w.status_mutation(target,245,444,12,1001,{},false));
  check(w.shadow_damage(source,target,*skills::effects::shadow_damage(219,12),20,1001));check(w.resources(2)->health==0);
 }
 std::cout<<"PASS "<<cases<<" NPC Protective Spirit damage-path cases plus low maximum,expiry,removal,life-steal isolation and modifier-order checks;normal casting still unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
