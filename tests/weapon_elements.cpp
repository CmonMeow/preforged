#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool x){if(!x)throw std::runtime_error("weapon element regression");}
int main(){using namespace combat;
 for(unsigned raw=0;raw<=255;++raw){const auto kind=weapon::element(raw);check(bool(kind)==(raw==3||raw==4||raw==5||raw==11));}
 check(!weapon::element({}));check(weapon::element(3)==Kind::cold&&weapon::element(4)==Kind::lightning&&weapon::element(5)==Kind::fire&&weapon::element(11)==Kind::earth);
 npcs::Definition d;d.binding.map=146;d.binding.actor=100;d.relation=npcs::Relation::hostile_to_players;d.spawn_health=Health{500,500};d.damageable=true;
 d.cold_armor=100;d.lightning_armor=80;d.fire_armor=60;d.earth_armor=40;
 for(unsigned raw:{3u,4u,5u,11u}){
  npcs::LiveActor actor(d,{1,2,3,146,100});const auto life=actor.lifetime();
  check(actor.final_caster_armor(life,raw,100)==(raw==3?100:raw==4?80:raw==5?60:40));
  const auto hit=actor.apply_caster_weapon_damage(life,17,raw,100);check(hit&&hit->damage==17&&hit->kind==*weapon::element(raw)&&actor.actor(life)->health->current==483);
  auto stale=life;++stale.actor_generation;check(!actor.final_caster_armor(stale,raw,100)&&!actor.apply_caster_weapon_damage(stale,17,raw,100));
 }
 auto unknown=d;unknown.cold_armor.reset();unknown.lightning_armor.reset();npcs::LiveActor actor(unknown,{1,2,3,146,100});
 check(!actor.final_caster_armor(actor.lifetime(),3,100)&&!actor.final_caster_armor(actor.lifetime(),4,100));
 check(!actor.apply_caster_weapon_damage(actor.lifetime(),17,8,100));
 std::cout<<"PASS explicit weapon element mapping,cold/lightning NPC armor,typed resolved hits,unknown armor and stale lifetime\n";
}
