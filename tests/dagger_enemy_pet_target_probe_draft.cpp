#include "dagger_successful_dual_fixture_draft.h"
// BEFORE diagnostic: real owned pet relation versus actual Session target gate.
// No authored hostile relation/flesh overrides and no payment bypass.
int main()try{
 Fixture f(782);const auto owner=f.target;
 check(f.world.restore_pet_profile(owner,1,pets::Profile{pets::Pet{}},1));
 check(f.world.pet_owner_combat(owner,1,0,f.clock));
 check(f.world.reconcile_pet_bar_mode(owner,162,true,true,std::array{436u},f.clock));
 const auto pet=f.world.owned_pet_scope(owner);check(pet.has_value());
 const auto relation=f.world.subversion_spell_target(f.source,*pet);
 check(relation&&relation->relation==effects::SpellTargetRelation::foe);
 check(f.world.npc_combat_snapshot(*pet)->health->current>0);
 // Same canonical production hook as gw_auth: this must admit enemy pet
 // targeting independently of any later skill-specific flesh readiness.
 const bool gate=f.world.melee_target(f.source,pet->actor,f.clock,782);
 std::cout<<"enemy_pet actor="<<pet->actor<<" relation=foe melee_target="<<gate<<" payments="<<f.payments<<'\n';
 f.target=*pet;const auto admitted=f.start();
 std::cout<<"paid_jagged_reason="<<unsigned(admitted)<<" payments="<<f.payments<<"\n";
 check(gate);check(admitted==skills::casting::Reason::accepted);
 const auto before=f.world.npc_combat_snapshot(*pet)->health->current;
 f.update(f.last_permit->release_time());
 check(f.world.npc_combat_snapshot(*pet)->health->current<before);
 check(f.world.has_condition(*pet,combat::conditions::Kind::bleeding,f.clock));
 check(f.payments==1&&f.completions==1&&f.copied_observations==1);
 const auto after=f.world.npc_combat_snapshot(*pet)->health->current;
 f.update(f.clock);check(f.world.npc_combat_snapshot(*pet)->health->current==after&&f.payments==1&&f.copied_observations==1);return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
