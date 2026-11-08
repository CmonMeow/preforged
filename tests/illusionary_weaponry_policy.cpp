#include "gw_illusionary_weaponry.h"
#include "gw_attack_interrupts.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool ok){if(!ok)throw std::runtime_error("Illusionary Weaponry replacement invariant");}
int main()try{
 using namespace skills::effects;using namespace combat::weapon;
 const Scope source{1,1,1},target{1,2,1};Registry registry(source);
 check(registry.apply(source,{1,1,33,12,Category::enchantment,100,30100},Replace::same_skill).accepted);
 const auto replacement=illusionary_weaponry_replacement(registry,source,100,true,true);check(bool(replacement));
 AttackOutcomes attacks(source);check(attacks.release(source,target,1,100));
 const auto resolved=attacks.finish(source,target,1,replacement->outcome,200);
 check(resolved&&!resolved->failed_hit()&&resolved->outcome!=AttackOutcome::hit&&!resolved->before_damage);
 check(!attacks.finish(source,target,1,replacement->outcome,200));
 check(attacks.release(source,target,2,300)&&attacks.qualify_hit(source,target,2,301).has_value());
 check(!attacks.finish(source,target,2,AttackOutcome::replaced,302)); // Cannot retroactively undo an on-hit phase.
 check(!illusionary_weaponry_replacement(registry,{1,1,2},100,true,true));
 check(!illusionary_weaponry_replacement(registry,source,100,false,true));
 check(!illusionary_weaponry_replacement(registry,source,100,true,false));
 check(!illusionary_weaponry_replacement(registry,source,30100,true,true));
 skills::Slots bar{};bar[0]={33,0};bar[1]={11,0};bar[2]={26,0};
 check(illusionary_weaponry_armor(replacement->plan,bar)==10);
 check(illusionary_weaponry_armor(*illusionary_weaponry(3181,12),bar)==0);
 auto forged=replacement->plan;++forged.damage;check(!illusionary_weaponry_armor(forged,bar));
 for(unsigned rank=0;rank<=20;++rank){const auto plan=illusionary_weaponry(33,rank);check(plan&&plan->damage>=8&&plan->duration==30000);}
 check(illusionary_weaponry(33,15)->damage==40&&!illusionary_weaponry(33,21));
 std::cout<<"PASS IW replacement is neither hit nor failure,exact-once scope,sequence,expiry and PvE/PvP armor policy;World integration pending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
