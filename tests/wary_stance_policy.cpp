#include "gw_wary_stance.h"
#include "gw_block_attribution.h"
#include "gw_wary_block_reward.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;using namespace fw::gw::skills::effects;
void check(bool b){if(!b)throw std::runtime_error("Wary policy");}
int main()try{
 for(unsigned r=0;r<=20;++r){auto p=wary_stance(r);check(p&&p->duration==1000ull*interpolate(1,6,r)&&p->energy_reward==5&&p->raw_adrenaline_parameter==35);}check(!wary_stance(21));
 check(wary_attack_skill(0)==false);check(wary_attack_skill(340)==true&&wary_attack_skill(399)==true);
 check(wary_attack_skill(281)==false);check(!wary_attack_skill(3433));
 unsigned pets=0;for(const auto& row:skill_catalog_2022)if(row.type==20){++pets;check(wary_attack_skill(row.id)==false);}check(pets>0);
 std::vector<Entry> entries{{123,25,377,12,Category::stance,100,5100}};
 check(wary_would_block(entries,340,100)==true&&wary_would_block(entries,399,100)==true);
 check(reviewed_block_probability(entries,false,100,{}, {},340)==1.);
 check(reviewed_block_probability(entries,false,100)==0.);
 check(reviewed_block_probability(entries,true,100,{}, {},340)==0.);
 const auto sources=reviewed_block_sources(entries,100,{}, {},340);check(sources&&sources->size()==1&&sources->front().skill==377&&sources->front().probability==1.);
 check(reviewed_block_sources(entries,100)->empty());
 check(wary_would_block(entries,0,100)==false&&wary_would_block(entries,281,100)==false);
 check(wary_would_block(entries,340,99)==false&&wary_would_block(entries,340,5100)==false);
 entries[0].rank=21;check(!wary_would_block(entries,340,100));
 const Scope owner{1,25,1};Registry registry(owner);
 check(registry.apply(owner,{123,25,377,0,Category::stance,100,1100},Replace::category).accepted);
 Slots bar{};bar[0]={344,1};CooldownBarAuthority authority(owner);const auto snapshot=*authority.publish(owner,bar,100);
 check(!prepare_wary_accepted_use(registry,owner,{344,0},snapshot,200));
 check(!prepare_wary_accepted_use(registry,{1,25,2},{344,1},snapshot,200));
 check(!prepare_wary_accepted_use(registry,owner,{344,1},snapshot,99));
 const auto ended=prepare_wary_accepted_use(registry,owner,{344,1},snapshot,200);
 check(ended&&ended->ended&&registry.lookup(owner,123)&&!ended->registry.lookup(owner,123));
 const auto twice=prepare_wary_accepted_use(ended->registry,owner,{344,1},snapshot,200);check(twice&&!twice->ended);
 const Scope attacker{1,31,2};const AttributedBlock blocked{fw::gw::combat::weapon::AttackOutcome::block,BlockSource{377,123,1.},1};
 const auto reward=wary_block_reward(registry,owner,attacker,blocked,340,200);
 check(reward&&reward->owner==owner&&reward->attacker==attacker&&reward->effect_instance==123&&reward->energy==5&&reward->at==200);
 check(wary_block_reward(registry,owner,attacker,blocked,399,200).has_value());
 check(!wary_block_reward(registry,owner,attacker,blocked,0,200));
 check(!wary_block_reward(registry,owner,attacker,blocked,281,200));
 check(!wary_block_reward(registry,owner,attacker,blocked,3433,200));
 check(!wary_block_reward(registry,owner,attacker,{fw::gw::combat::weapon::AttackOutcome::miss,BlockSource{377,123,1.},1},340,200));
 check(!wary_block_reward(registry,owner,attacker,{fw::gw::combat::weapon::AttackOutcome::hit,BlockSource{377,123,1.},1},340,200));
 check(!wary_block_reward(registry,owner,attacker,{fw::gw::combat::weapon::AttackOutcome::block,BlockSource{377,124,1.},1},340,200));
 check(!wary_block_reward(registry,owner,attacker,{fw::gw::combat::weapon::AttackOutcome::block,BlockSource{378,123,1.},1},340,200));
 check(!wary_block_reward(registry,owner,{2,31,2},blocked,340,200));
 check(!wary_block_reward(registry,owner,attacker,blocked,340,1100));
 check(!wary_block_reward(ended->registry,owner,attacker,blocked,340,200));
 std::cout<<"PASS Wary pinned policy and ordinary/attack-skill/pet classification,expiry and invalid rank\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
