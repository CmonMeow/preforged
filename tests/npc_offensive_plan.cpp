#include "gw_npc_offensive_plan.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
int main()try{
 const auto check=[](bool ok){if(!ok)throw std::runtime_error("NPC offensive plan");};
 namespace offensive=combat::ai::offensive;
 const skills::effects::Scope source{1,100,1},destination{1,2,1};
 const offensive::Plan ice{*skills::effects::water_projectile(214,12)};
 check(ice.valid()&&ice.skill()==214&&ice.rank()==12&&ice.projectile()&&!ice.radius()&&ice.kind()==combat::Kind::cold);
 check(ice==offensive::Plan{*skills::effects::water_projectile(214,12)});
 check(!(ice==offensive::Plan{*skills::effects::water_projectile(214,11)}));
 check(!offensive::known_zero({},source,1000));
 offensive::OvercastObservation authority{source,0,1000,"authored live zero-Overcast fixture"};
 check(offensive::known_zero(authority,source,1000));check(!offensive::known_zero(authority,source,999));
 authority.amount=1;check(!offensive::known_zero(authority,source,1000));authority.amount=0;
 check(!offensive::known_zero(authority,{1,100,2},1000));authority.evidence.clear();check(!offensive::known_zero(authority,source,1000));
 combat::Actor actor;actor.id=2;actor.generation=1;actor.health=combat::Health{500,500};actor.damageable=true;actor.cold_armor=60;
 const auto hit=offensive::target(ice,source,destination,actor,{},20,1000);
 check(hit.accepted&&hit.hits.size()==1&&hit.actor.health->current==450&&hit.additions.empty()&&!hit.energy);
 actor.cold_armor.reset();check(!offensive::target(ice,source,destination,actor,{},20,1000).accepted);
 actor.cold_armor=60;check(!offensive::target(ice,source,{1,2,2},actor,{},20,1000).accepted);
 bool rejected=false;try{(void)offensive::Plan{*skills::effects::water_projectile(213,12)};}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 auto forged=*skills::effects::water_projectile(214,12);++forged.damage;rejected=false;
 try{(void)offensive::Plan{forged};}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 const offensive::Plan fire{*skills::effects::elemental(186,12)};
 check(fire.valid()&&fire.radius()==166&&fire.kind()==combat::Kind::fire&&fire.projectile());
 const offensive::Plan burst{*skills::effects::elemental(183,12)};
 check(burst.valid()&&burst.caster_center()&&burst.range()==166&&burst.radius()==166&&!burst.projectile());
 const auto* native=skills::find_skill(183);
 check(native&&native->energy_cost()==10&&native->activation==.75f&&native->aftercast==.75f&&native->recharge==10);
 actor.fire_armor=60;const auto area=offensive::target(burst,source,destination,actor,{},20,1000);
 check(area.accepted&&area.hits.size()==1&&area.actor.health->current==386);
 // Projection controls survival-dependent effects, while incoming hits remain
 // available for the authoritative damage/reaction commit.
 actor.health=combat::Health{1,500};
 const combat::DamageProjection saved=[](combat::Actor& body,const combat::Damage& damage)->std::optional<combat::DamageStep>{
  auto incoming=combat::apply(body,damage,true);if(incoming.reason!=combat::Reason::accepted)return {};
  auto actual=incoming;actual.killed=false;actual.applied=0;actual.damage=0;actual.after={100,500};
  body.health=actual.after;body.effects=0;return combat::DamageStep{incoming,actual};
 };
 const offensive::Plan immolate{*skills::effects::elemental(191,12)};
 const auto rescued=offensive::target(immolate,source,destination,actor,{},20,1000,saved);
 check(rescued.accepted&&rescued.hits.front().killed&&rescued.actor.health->current==100&&rescued.additions.size()==1);
 const auto rescued_ice=offensive::target(ice,source,destination,actor,{},20,1000,saved);
 check(rescued_ice.accepted&&rescued_ice.hits.front().killed&&rescued_ice.actor.health->current==100);
 check(!offensive::target(ice,source,destination,actor,{},20,1000,[](auto&,const auto&)->std::optional<combat::DamageStep>{return {};} ).accepted);
 check(!offensive::Plan{}.valid());
 std::cout<<"PASS immutable NPC offensive plans and explicit zero-Overcast authority\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
