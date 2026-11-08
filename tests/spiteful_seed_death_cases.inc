// Shared publisher death handling through Spiteful's deferred area boundary.
static unsigned spiteful_seed_death_cases(){
 unsigned cases=0;const auto damage=effects::spiteful_spirit(12)->damage;
 // Reversal healing can settle lethal Deep Wound debt before any actual damage.
 // That death must not fabricate a Seed trigger or cancel the other area hit.
 for(unsigned rank:{0u,12u,20u})for(int rr:{0,12,20})for(bool completion:{false,true}){
  Fixture f(10,rank);f.rr(rr);
  check(f.w.apply_condition(f.foe,f.target,combat::conditions::Kind::deep_wound,100,10000));
  const auto epoch=*f.w.control_epoch(f.target);
  check(f.w.spiteful_action(f.action(completion)));
  check(f.w.resources(3)->health==0&&f.w.resources(2)->health==500-damage&&f.w.resources(4)->health==500);
  check(*f.w.control_epoch(f.target)==epoch+1&&f.w.player_death_times({3,4})[0]==200);
  f.w.tick(201);check(*f.w.control_epoch(f.target)==epoch+1&&f.w.resources(3)->health==0);++cases;
 }
 // The Seed heal itself can kill a different, wounded party recipient. Its
 // World cleanup must survive deferred packet publication and occur only once.
 for(unsigned rank:{0u,12u,20u})for(unsigned state:{0u,1u,2u,3u})for(bool completion:{false,true}){
  Fixture f(500,rank);f.w.set_vitals(4,{10,1000,100,100},100);
  check(f.w.apply_condition(f.foe,f.monk,combat::conditions::Kind::deep_wound,100,10000));
  const auto epoch=*f.w.control_epoch(f.monk);f.source_state(state);
  check(f.w.spiteful_action(f.action(completion)));
  const auto healing=effects::healing_seed(rank)->healing;
  check(f.w.resources(2)->health==500-damage+healing&&f.w.resources(3)->health==500-damage+healing&&f.w.resources(4)->health==0);
  check(*f.w.control_epoch(f.monk)==epoch+1&&f.w.player_death_times({4,3})[0]==200);
  f.w.tick(201);check(*f.w.control_epoch(f.monk)==epoch+1&&f.w.resources(4)->health==0);++cases;
 }
 check(cases==42);return cases;
}
