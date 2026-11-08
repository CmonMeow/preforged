#include "dagger_successful_dual_fixture_draft.h"
static double ordinary(bool bow,unsigned extra){
 Fixture f(779);const auto owner=f.target;pets::Pet durable;durable.level=20;
 check(f.world.restore_pet_profile(owner,1,pets::Profile{durable},20));check(f.world.pet_owner_combat(owner,20,0,f.clock));
 check(f.world.reconcile_pet_bar_mode(owner,162,true,true,std::array{436u},f.clock));const auto pet=*f.world.owned_pet_scope(owner);
 auto weapon=f.weapon;weapon.item_type=bow?5:32;weapon.damage.raw_damage_type=bow?1:5;
 check(f.world.ordinary_weapon(f.source,weapon,20,f.clock,combat::weapon::CriticalProfile{7,12,20}));
 if(extra==1)check(f.world.conjure_enchantment(f.source,*effects::conjure(182,12),f.clock));
 if(extra==2)check(f.world.status_mutation(f.source,33,123,12,f.clock,f.clock+30000,true));
 if(extra==3){const auto plan=*combat::weapon::preparation_contact_definition(433,12);check(f.world.status_mutation(f.source,433,123,12,f.clock,f.clock+plan.duration,true));}
 const auto before=f.world.npc_combat_snapshot(pet)->health->current;
 check(f.world.ordinary_attack(f.source,pet.actor,f.clock));f.world.tick(f.clock+500);f.world.cancel_ordinary_attack(f.source,f.clock+501);f.world.tick(f.clock+700);
 const auto after=f.world.npc_combat_snapshot(pet)->health->current;const auto outcomes=f.world.take_attack_outcomes(f.source);
 check(outcomes.size()==1&&outcomes[0].outcome==(extra==2?combat::weapon::AttackOutcome::replaced:combat::weapon::AttackOutcome::hit));
 f.world.tick(f.clock+700);check(f.world.npc_combat_snapshot(pet)->health->current==after&&f.world.take_attack_outcomes(f.source).empty());return before-after;
}
static void blossom(){
 Fixture f(775);f.arm();const auto create=[&](effects::Scope owner){pets::Pet durable;durable.level=20;
  check(f.world.restore_pet_profile(owner,1,pets::Profile{durable},20));check(f.world.pet_owner_combat(owner,20,0,f.clock));
  check(f.world.reconcile_pet_bar_mode(owner,162,true,true,std::array{436u},f.clock));return *f.world.owned_pet_scope(owner);};
 const auto friendly=create(f.source),enemy=create(f.target);const auto before=f.world.npc_combat_snapshot(enemy)->health->current;
 const auto protected_hp=f.world.npc_combat_snapshot(friendly)->health->current;
 check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());check(bool(f.last_result.successful_second));
 const auto bonus=combat::weapon::detached_dagger_family_plan(775,12)->bonus;
 check(f.world.npc_combat_snapshot(enemy)->health->current==before-bonus);
 f.update(f.last_result.successful_second->due());check(f.world.npc_combat_snapshot(enemy)->health->current==before-2*bonus);
 check(f.world.npc_combat_snapshot(friendly)->health->current==protected_hp&&f.payments==1&&f.copied_observations==1);
 const auto after=f.world.npc_combat_snapshot(enemy)->health->current;f.update(f.clock);check(f.world.npc_combat_snapshot(enemy)->health->current==after);
}
int main()try{
 const auto melee=ordinary(false,0),conjure=ordinary(false,1),iw=ordinary(false,2);
 check(melee>0&&conjure>melee&&iw==effects::illusionary_weaponry(33,12)->damage);
 const auto bow=ordinary(true,0),kindle=ordinary(true,3);check(bow>0&&kindle>bow);blossom();
 std::cout<<"PASS real enemy-ownedpet ordinary melee/Conjure/IW and releasedBow/Kindle damage+single outcome/replay\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
