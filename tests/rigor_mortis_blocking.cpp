#include "gw_blocking.h"
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool value){if(!value)throw std::runtime_error("Rigor Mortis blocking policy regression");}
int main()try{
 check(rigor_mortis(0)->duration==8000&&rigor_mortis(12)->duration==18000&&rigor_mortis(15)->duration==20000&&rigor_mortis(20)->duration==24000&&!rigor_mortis(21));
 const std::array<double,2> chances{.5,.5};
 check(attack_block_probability(chances,false,{},100)==.75);
 check(attack_block_probability({},false,{},100)==0);
 check(attack_block_probability(chances,true,{},100)==0);
 Registry registry({1,2,3});const Scope scope{1,2,3};
 check(registry.apply(scope,{1,1,137,12,Category::hex,100,18100},Replace::same_skill).accepted);
 auto entries=registry.active(scope,100);check(attack_block_probability(chances,false,entries,100)==0);
 check(attack_block_probability(chances,false,entries,99)==.75);
 check(attack_block_probability(chances,false,entries,18100)==.75);
 check(attack_block_probability(chances,false,registry.active({1,2,4},100),100)==.75);
 registry.clear(scope);check(attack_block_probability(chances,false,registry.active(scope,100),100)==.75);
 for(auto malformed:{Entry{0,1,137,12,Category::hex,100,18100},Entry{1,0,137,12,Category::hex,100,18100},Entry{1,1,137,21,Category::hex,100,18100},Entry{1,1,137,12,Category::other,100,18100},Entry{1,1,137,12,Category::hex,100,{}}}){
  check(attack_block_probability(chances,false,std::span<const Entry>(&malformed,1),100)==.75);
 }
 for(double invalid:{-.1,1.1,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
  check(!attack_block_probability(std::span<const double>(&invalid,1),true,entries,100));
 }
 check(guardian_block(0)->duration==2000&&guardian_block(12)->duration==6000&&guardian_block(15)->duration==7000&&guardian_block(20)->duration==9000&&!guardian_block(21));
 Registry protected_actor(scope);
 check(protected_actor.apply(scope,{2,1,258,12,Category::enchantment,100,6100},Replace::same_skill).accepted);
 auto protection=protected_actor.active(scope,100);
 check(reviewed_block_probability(protection,false,100)==.5);
 protection.push_back(protection.front());check(reviewed_block_probability(protection,false,100)==.5);
 check(reviewed_block_probability(protection,false,6100)==0);
 check(reviewed_block_probability(protection,true,100)==0);
 protection.insert(protection.end(),entries.begin(),entries.end());check(reviewed_block_probability(protection,false,100)==0);
 using Outcome=fw::gw::combat::weapon::AttackOutcome;
 check(resolve_block_contact(Outcome::hit,.5,.499)==Outcome::block);
 check(resolve_block_contact(Outcome::hit,.5,.5)==Outcome::hit);
 for(auto earlier:{Outcome::miss,Outcome::dodge,Outcome::obstruction,Outcome::cancelled,Outcome::target_retired,Outcome::block})
  check(resolve_block_contact(earlier,0,0)==earlier);
 check(!resolve_block_contact(Outcome::hit,.5,1)&&!resolve_block_contact(Outcome::hit,-.1,0));
 std::cout<<"PASS Rigor Mortis canonical duration,block-only suppression,expiry/removal/generation and multiplicative blocking policy;runtime and rendered acceptance pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
