#include "dagger_successful_dual_fixture_draft.h"
int main()try{
 for(unsigned team_case=0;team_case<4;++team_case){
  Fixture f(783);auto owner=f.target;
  if(team_case==0)owner=f.source;
  if(team_case==1){f.world.join(3,"Same team owner",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);f.world.set_vitals(3,{1000,1000,30,30},100);owner=*f.world.effect_scope(3);}
  check(f.world.restore_pet_profile(owner,1,pets::Profile{pets::Pet{}},1));check(f.world.pet_owner_combat(owner,1,0,f.clock));
  check(f.world.reconcile_pet_bar_mode(owner,162,true,true,std::array{436u},f.clock));
  const auto pet=*f.world.owned_pet_scope(owner);f.target=pet;
  const auto before=f.world.npc_combat_snapshot(pet)->health->current;
  if(team_case==3){check(f.world.kill(owner,f.clock));check(f.world.owned_pet_scope(owner)==std::optional(pet));}
  const auto admitted=f.start();
  if(team_case<2){check(admitted!=skills::casting::Reason::accepted&&f.payments==0);check(f.world.npc_combat_snapshot(pet)->health->current==before);}
  else {check(admitted==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
   check(f.last_result.accepted&&f.world.npc_combat_snapshot(pet)->health->current<before);
   check(f.payments==1&&f.copied_observations==1&&f.completions==1);
   const auto after=f.world.npc_combat_snapshot(pet)->health->current;f.update(f.clock);check(f.world.npc_combat_snapshot(pet)->health->current==after&&f.payments==1);}
 }
 {Fixture f(783,false,true);f.target=*f.world.effect_scope(101);const auto before=f.world.npc_combat_snapshot(f.target)->health->current;
  check(f.start()!=skills::casting::Reason::accepted&&f.payments==0);check(f.world.npc_combat_snapshot(f.target)->health->current==before);}
 {Fixture f(783);const auto owner=f.target;
  check(f.world.restore_pet_profile(owner,1,pets::Profile{pets::Pet{}},1));check(f.world.pet_owner_combat(owner,1,0,f.clock));
  check(f.world.reconcile_pet_bar_mode(owner,162,true,true,std::array{436u},f.clock));
  const auto old_pet=*f.world.owned_pet_scope(owner);f.target=old_pet;
  check(f.start()==skills::casting::Reason::accepted);const auto release=f.last_permit->release_time();check(f.last_permit->open());
  f.world.leave(owner.actor);f.world.join(2,"New pet owner",60,0,std::make_shared<world::Outbox>(),0,{}, {},f.clock+1,2);f.world.set_vitals(2,{1000,1000,30,30},100);
  const auto next_owner=*f.world.effect_scope(2);check(next_owner!=owner);
  check(f.world.restore_pet_profile(next_owner,1,pets::Profile{pets::Pet{}},1));check(f.world.pet_owner_combat(next_owner,1,0,f.clock+1));
  check(f.world.reconcile_pet_bar_mode(next_owner,162,true,true,std::array{436u},f.clock+1));const auto fresh=*f.world.owned_pet_scope(next_owner);check(fresh!=old_pet);
  const auto before=f.world.npc_combat_snapshot(fresh)->health->current;f.update(release);
  check(!f.last_permit->open()&&!f.cast.active()&&f.payments==1&&f.completions==0&&f.copied_observations==0);
  check(f.world.npc_combat_snapshot(fresh)->health->current==before);f.update(release);check(f.world.npc_combat_snapshot(fresh)->health->current==before);
 }
 std::cout<<"PASS real ownedpet own/ally protected, enemy paid783 actualhit/replay; no flesh/KD inference\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
