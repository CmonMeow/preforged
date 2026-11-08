// BEFORE probe: actual owned enemy pet, player-owned Fragility and Healing Seed,
// then actual shared condition application. No new926/1470 code is needed.
#include "dagger_successful_dual_fixture_draft.h"
int main()try{
 Fixture f(779);const auto owner=f.target;
 f.world.set_vitals(owner.actor,{500,1000,30,30},100);
 pets::Pet durable;durable.level=20;
 check(f.world.restore_pet_profile(owner,1,pets::Profile{durable},20));check(f.world.pet_owner_combat(owner,20,0,f.clock));
 check(f.world.reconcile_pet_bar_mode(owner,162,true,true,std::array{436u},f.clock));const auto pet=*f.world.owned_pet_scope(owner);
 const auto seed=*effects::healing_seed(12);check(f.world.complete_healing_seed(owner,pet,seed,0,f.clock));
 const auto fragility=*effects::degeneration_hex(19,12);check(f.world.degeneration_hex(f.source,pet,fragility,f.clock));
 const auto before=f.world.npc_combat_snapshot(pet)->health->current;f.box->drain();
 check(f.world.apply_condition(f.source,pet,combat::conditions::Kind::bleeding,f.clock+1,10000));
 check(f.world.npc_combat_snapshot(pet)->health->current<before);
 check(f.world.resources(owner.actor)->health==500+seed.healing);
 const auto after=f.world.resources(owner.actor)->health;f.world.tick(f.clock+1);check(f.world.resources(owner.actor)->health==after);
 std::cout<<"PASS actual NPC Fragility damage triggers owned Seed Health gain once\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
