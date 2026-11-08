#include "dagger_successful_dual_fixture_draft.h"
// BEFORE probe: normal known Assassin critical authority must reach type32.
// Uses declared existing shared critical model, not invented retail RNG odds.
int main()try{
 Fixture f(782);unsigned draws=0;f.world.configure_attack_critical_draws([&]{++draws;return 0.;});
 check(f.world.consume_energy(f.source.actor,10,f.source,f.clock));check(f.world.resources(1)->energy==20);
 const auto before=f.health();check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
 const auto health=f.health();const auto energy=f.world.resources(1)->energy;
 std::cout<<"known_dagger critical_draws="<<draws<<" target_health="<<health<<" Energy="<<energy<<" one_fee="<<f.payments<<'\n';
 check(f.last_result.accepted&&f.payments==1&&f.paid==5&&f.copied_observations==1);
 check(draws==1&&health<before-20&&energy==17); // rank12 Critical Strikes grants2 afterrealcritical.
 f.update(f.clock);check(draws==1&&f.health()==health&&f.world.resources(1)->energy==energy&&f.payments==1);return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
