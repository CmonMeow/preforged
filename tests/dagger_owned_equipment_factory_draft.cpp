#include "dagger_successful_dual_fixture_draft.h"
#include "../tools/client/dagger_review_gear.h"
static inventory::Profile equipped(){
 const auto grant=test::dagger_review::grant(inventory::baseline());check(bool(grant));
 auto current=inventory::decode(inventory::encode(grant->first));attributes::Ranks ranks{};ranks[29]=12;
 equipment::proposal::Policy policy{true,7,"Assassin",ranks,[&](const auto& definition){return equipment::metadata::reviewed(current,definition);}};
 const auto change=equipment::proposal::propose(current,{inventory::Change::equip,grant->second},policy);check(change.accepted());
 return inventory::decode(inventory::encode(change.profile));
}
int main()try{
 auto profile=equipped();combat::weapon::Policy policy;policy.primary=7;policy.character="Assassin";policy.caster_weapon=true;policy.effective[29]=12;
 const auto parsed=combat::weapon::owned_weapon(profile,901,policy);
 check(parsed&&parsed->item_type==32&&!parsed->caster_weapon&&parsed->mastery==12&&parsed->critical_penalty==0.);
 check(parsed->damage.minimum==7&&parsed->damage.maximum==17&&parsed->damage.raw_damage_type==1);
 auto ea=policy;ea.primary=6;ea.character="Cmon Meow";check(combat::weapon::owned_weapon(profile,901,ea).has_value());
 auto below=policy;below.effective[29]=8;check(!combat::weapon::owned_weapon(profile,901,below));
 const auto item=parsed->item;
 {auto copy=profile;copy.items.at(item).customized.reset();check(!combat::weapon::owned_weapon(copy,901,policy));}
 {auto copy=profile;copy.items.at(item).profession=11;check(!combat::weapon::owned_weapon(copy,901,policy));}
 {auto copy=profile;copy.items.at(item).definition.modifiers.push_back(0x27680117u);check(!combat::weapon::owned_weapon(copy,901,policy));}
 for(unsigned type:{0u,1u,2u,5u}){auto copy=profile;copy.items.at(item).definition.modifiers[1]=0x24b80000u|(type<<8);
  const auto value=combat::weapon::owned_weapon(copy,901,policy);check(bool(value)==(type!=0));}
 {Fixture f(782);f.weapon=*parsed;check(f.world.ordinary_weapon(f.source,f.weapon,20,f.clock,combat::weapon::CriticalProfile{7,12,20}));
  f.hooks.weapon_snapshot=[&]{return combat::weapon::owned_weapon(profile,901,policy);};
  const auto before=f.health();check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
  check(f.last_result.accepted&&f.health()<before&&f.world.has_condition(f.target,combat::conditions::Kind::bleeding,f.clock));
  check(f.payments==1&&f.paid==skills::find_skill(782)->energy_cost()&&f.copied_observations==1);
  const auto outcomes=f.world.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes[0].outcome==combat::weapon::AttackOutcome::hit);}
 std::cout<<"PASS real FWI metadata/equip/owned_weapon32/mastery29 into paid World Jagged; unsupported words/profession/customization/requirement remain closed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
