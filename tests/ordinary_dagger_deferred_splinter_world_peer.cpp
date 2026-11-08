#include "ordinary_dagger_deferred_peer_fixture.h"
static void secondary(Fixture& f){f.w.join(4,"Splash recipient",90,30,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(4,{1000,1000,100,100},100);}
static bool active(Fixture& f,std::uint64_t at){const auto s=f.w.status_snapshot(f.source,at);check(bool(s));return std::any_of(s->begin(),s->end(),[](const auto& e){return e.skill==792;});}
int main()try{
 {Fixture f;secondary(f);const auto plan=*weapon_spell(792,0,8,0);check(plan.charges==1&&f.w.apply_weapon_spell(f.source,f.source,plan,100));
  f.start();const auto first=f.first();check(first.size()==1&&first[0].outcome==combat::weapon::AttackOutcome::hit&&first[0].at==1500);
  check(f.w.resources(4)->health==1000-plan.amount&&!active(f,1500));const auto hp=f.health();
  const auto second=f.second();check(second.size()==1&&second[0].outcome==combat::weapon::AttackOutcome::hit&&second[0].at==2000&&second[0].event>first[0].event&&f.health()<hp);
  check(f.w.resources(4)->health==1000-plan.amount&&f.selections==1&&f.w.resources(1)->energy==100&&f.w.splinter_late_refusals()==0);
  f.w.cancel_ordinary_attack(f.source);f.w.tick(2000);check(f.outcomes().empty()&&f.w.resources(4)->health==1000-plan.amount);
 }
 {Fixture f;secondary(f);const auto plan=*weapon_spell(792,0,8,0);check(f.w.apply_weapon_spell(f.source,f.source,plan,100));check(f.w.complete_shield_judgment(f.target,f.target,*shield_judgment(0),0,100));
  f.start();check(f.first().empty()&&f.health()==1000&&f.w.resources(4)->health==1000&&f.w.resources(1)->health==500&&active(f,1500));
  f.w.cancel_ordinary_attack(f.source);check(f.w.player_holy_equipment(f.source,0));check(f.w.ordinary_attack(f.source,f.target.actor,3000));
  f.w.tick(3499);check(f.outcomes().empty());f.w.tick(3500);const auto first=f.outcomes();
  check(first.size()==1&&first[0].event==2&&first[0].outcome==combat::weapon::AttackOutcome::hit&&f.w.knocked_down(f.source,3500));
  check(f.w.resources(4)->health==1000-plan.amount&&!active(f,3500)&&f.selections==2);
  f.w.tick(4000);check(f.outcomes().empty()&&f.w.splinter_late_refusals()==0&&f.w.shield_judgment_failures()==0);
 }
 std::cout<<"PASS delayed canonical contacts lastSplintercharge/refused-first cleanup\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
