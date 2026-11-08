#include "dagger_radius_fixture_draft.h"
int main()try{
 for(unsigned skill:{775u,776u,777u,3061u}){Fixture f(false);f.offhand();
  const auto plan=*combat::weapon::attack_skill(skill,0,0,0,true);const auto before=f.health();
  check(!f.world.melee_attack(f.source,f.target,f.weapon,plan,1000,{},true));
  check(f.health()==before&&f.world.take_attack_outcomes(f.source).empty());
  check(f.world.energy_capacity(1,7,0,1000,{10,-2}));
  test::RetainedDaggerOwnerDraft owner(f.world,f.source,f.box);
  auto stale=f.target;++stale.generation;
  check(owner.begin(stale,skill,0,1000)==skills::casting::Reason::target&&owner.payments==0);
  check(f.health()==before&&f.world.take_attack_outcomes(f.source).empty());
  check(owner.begin(f.target,skill,0,1000)==skills::casting::Reason::accepted);
  check(owner.paid==skills::find_skill(skill)->energy_cost()&&owner.payments==1);
  const auto first=owner.permit->release_time();owner.update(first-1);check(f.health()==before);
  owner.update(first);check(owner.active()&&owner.first_result.outcomes.size()==1&&f.health()<before);
  const auto phase=owner.first_result.successful_second;check(bool(phase));const auto hp=f.health();
  owner.update(phase->due()-1);check(f.health()==hp);owner.update(phase->due());
  check(!owner.active()&&owner.second_result.outcomes.size()==1&&f.health()<hp);
  check(owner.first_result.outcomes[0].event!=owner.second_result.outcomes[0].event&&owner.first_result.outcomes[0].at<owner.second_result.outcomes[0].at);
  check(owner.payments==1&&owner.completions==1&&owner.completed_observers==1);
  const auto& learned=owner.durable.learned;check(learned.size()==1&&learned.contains(combat::weapon::dagger_role(skill)));
 }
 std::cout<<"PASS raw bool dual closed, common paid Runtime/World retained caller, actual separate contacts and one fee\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
