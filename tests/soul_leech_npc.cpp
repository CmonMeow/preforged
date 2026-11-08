#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace combat;using namespace combat::npcs;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Soul Leech NPC line "+std::to_string(at.line()));
}
int main()try{
 const Lifetime life{1,2,3,146,100};const skills::effects::Scope source{1,1,1};
 Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
 d.relation=Relation::hostile_to_players;d.spawn_health=Health{1000,1000};d.damageable=d.conditions_allowed=true;
 for(unsigned rank=0;rank<=20;++rank){
  LiveActor a(d,life);const auto plan=*skills::effects::soul_leech(rank);
  check(a.apply_soul_leech(life,source,plan,77,100).has_value());
  const auto r=a.soul_leech_reaction(life,281,100);check(r&&r->plan==plan&&r->hex.cause==1);
  check(a.actor(life)->health->current==1000&&(a.actor(life)->effects&0x800));
  check(!a.soul_leech_reaction(life,320,100)&&!a.soul_leech_reaction(life,281,99));
  check(!a.apply_soul_leech(life,source,plan,78,200)); // World must settle first.
  check(a.advance_conditions(life,200).has_value());
  const skills::effects::Scope second{1,4,5};
  const auto replacement=a.apply_soul_leech(life,second,plan,78,200);
  check(replacement&&replacement->removed.size()==1&&replacement->removed[0].instance==77);
  check(a.soul_leech_reaction(life,281,200)->hex.cause==4);
  check(a.remove_hex_effect(life,78,200)&&!a.soul_leech_reaction(life,281,200));
  check(a.actor(life)->health->current==1000);
 }
 for(unsigned mode=0;mode<5;++mode){
  auto profile=d;if(mode==0)profile.relation=Relation::neutral;if(mode==1)profile.damageable=false;
  LiveActor a(profile,life);auto expected=life;if(mode==3)++expected.actor_generation;
  if(mode==2)check(a.apply_damage(life,{Kind::armor_ignoring,2000,20,28},100).killed);
  auto caster=source;if(mode==4)++caster.instance;
  check(!a.apply_soul_leech(expected,caster,*skills::effects::soul_leech(12),77,100));
  check(!a.hex_effect(life,77));
 }
 {LiveActor a(d,life);check(a.apply_soul_leech(life,source,*skills::effects::soul_leech(12),77,100).has_value());
  auto stale=life;++stale.actor_generation;check(!a.soul_leech_reaction(stale,281,200));
  check(a.advance_conditions(life,10100).has_value());check(!a.soul_leech_reaction(life,281,10100));
  check(a.refresh_hexes(life,10100));
  check(!(a.actor(life)->effects&0x800));}
 std::cout<<"PASS scoped NPC Soul Leech application/replacement/removal/expiry and settled-clock guards;Health reaction commit remains pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
