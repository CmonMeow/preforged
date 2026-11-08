#include "gw_hex_breaker_transaction.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok){if(!ok)throw std::runtime_error("Hex Breaker retaliation transaction regression");}
int main()try{
 const Scope owner{1,2,1},caster{1,3,4};Registry registry(owner);
 check(registry.apply(owner,{1,2,10,12,Category::stance,100,65100},Replace::category).accepted);
 for(unsigned mode=0;mode<6;++mode){
  combat::Actor actor{3,4,combat::Health{500,500},200,0,true};
  if(mode==1){actor.incoming_damage_multiplier=2;actor.incoming_reduction=5;}
  if(mode==2)actor.incoming_reduction=100;
  if(mode==3)actor.damageable=false;
  if(mode==4)actor.health.reset();
  if(mode==5)actor.health=combat::Health{20,500};
  const auto result=hex_breaker_transaction(registry,owner,caster,actor,28,20,200);check(bool(result));
  check(registry.lookup(owner,1)&&!result->registry.lookup(owner,1));
  check(result->reaction.incoming_skill==28&&result->reaction.damage==39);
  if(mode==3)check(result->retaliation.reason==combat::Reason::not_damageable&&result->caster.health->current==500);
  else if(mode==4)check(result->retaliation.reason==combat::Reason::unknown_health&&!result->caster.health);
  else{
   check(result->retaliation.reason==combat::Reason::accepted&&result->retaliation.skill==10&&result->retaliation.target==3);
   check(result->caster.health->current==(mode==1?427:mode==2?500:mode==5?0:461));
   check(result->retaliation.killed==(mode==5));
  }
 }
 combat::Actor actor{3,4,combat::Health{500,500},{},0,true};
 check(!hex_breaker_transaction(registry,owner,{1,3,5},actor,28,20,200));
 check(!hex_breaker_transaction(registry,owner,caster,actor,28,0,200));
 actor.incoming_damage_multiplier=-1;check(!hex_breaker_transaction(registry,owner,caster,actor,28,20,200));
 check(bool(registry.lookup(owner,1)));
 std::cout<<"PASS Hex Breaker speculative registry plus armor-ignoring retaliation,multipliers/reduction/zero/lethal damage and immune/unknownHealth rejection without allowing hex;World integration pending\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
