#include "dagger_successful_dual_fixture_draft.h"
int main()try{
 for(unsigned mode=0;mode<3;++mode){
  Fixture f(779);const auto create=[&](effects::Scope owner){pets::Pet durable;durable.level=20;
   check(f.world.restore_pet_profile(owner,1,pets::Profile{durable},20));check(f.world.pet_owner_combat(owner,20,0,f.clock));
   check(f.world.reconcile_pet_bar_mode(owner,162,true,true,std::array{436u},f.clock));return *f.world.owned_pet_scope(owner);};
  const auto friendly=create(f.source),enemy=create(f.target);const auto before=f.world.npc_combat_snapshot(enemy)->health->current;
  const auto protected_hp=f.world.npc_combat_snapshot(friendly)->health->current;unsigned expected=0;
  if(mode==0){const auto plan=*effects::weapon_spell(792,12,8,0);expected=plan.amount;check(f.world.apply_weapon_spell(f.source,f.source,plan,f.clock));}
  else {auto caster=f.source;
   if(mode==2){f.world.join(3,"Retained hex caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);f.world.set_vitals(3,{1000,1000,30,30},100);caster=*f.world.effect_scope(3);}
   check(f.world.degeneration_hex(caster,f.target,*effects::degeneration_hex(150,12),f.clock));expected=effects::mark_of_pain(12)->damage;
   if(mode==2)check(f.world.kill(caster,f.clock));
  }
  check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
  check(f.last_result.accepted&&f.world.npc_combat_snapshot(enemy)->health->current==before-expected);
  check(f.world.npc_combat_snapshot(friendly)->health->current==protected_hp&&f.payments==1&&f.copied_observations==1);
  f.update(f.clock);check(f.world.npc_combat_snapshot(enemy)->health->current==before-expected);
  check(f.world.splinter_late_refusals()==0);
 }
 std::cout<<"PASS actualpaid779 Splinter/MarkPain enemyPetsecondary, ownpetprotection, retaineddeadcurrentcaster andonce/replay\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
