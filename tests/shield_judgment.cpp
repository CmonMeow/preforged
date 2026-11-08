#include "gw_shield_judgment.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Shield Judgment line "+std::to_string(at.line()));}
int main()try{using namespace combat::weapon;const Scope caster{1,1,1},defender{1,2,3},attacker{1,3,2};unsigned cases=0;
 for(unsigned rank=0;rank<=20;++rank)for(auto delivery:{AttackDelivery::melee,AttackDelivery::ranged})for(bool sensitive:{false,true}){
  ++cases;const auto plan=*shield_judgment(rank);check(plan.damage==5+3*rank&&plan.duration==1000ull*interpolate(8,20,rank));
  const Entry effect{10,1,262,rank,Category::enchantment,1000,1000+plan.duration};AttackOutcomes ledger(attacker);
  check(ledger.release(attacker,defender,1,1000,delivery));check(bool(ledger.qualify_hit(attacker,defender,1,1100)));const auto hit=ledger.finish(attacker,defender,1,AttackOutcome::hit,1100);check(bool(hit));
  const auto reaction=shield_judgment_reaction(caster,defender,effect,*hit,true,{sensitive,{}},20);check(reaction&&reaction->damage.amount==plan.damage*(sensitive?2:1)&&reaction->knockdown_ms==2000&&reaction->effect==10&&reaction->attack_event==1);
  combat::Actor target;target.id=3;target.generation=2;target.damageable=true;target.health=combat::Health{1000,1000};const auto damage=combat::apply(target,reaction->damage,true);check(damage.reason==combat::Reason::accepted&&damage.damage==reaction->damage.amount);
  check(!ledger.finish(attacker,defender,1,AttackOutcome::hit,1100));
  for(auto outcome:{AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction,AttackOutcome::target_retired,AttackOutcome::cancelled,AttackOutcome::replaced}){auto failed=*hit;failed.outcome=outcome;check(!shield_judgment_reaction(caster,defender,effect,failed,true,{false,{}},20));}
  auto expired=*hit;expired.at=*effect.expires;check(!shield_judgment_reaction(caster,defender,effect,expired,true,{false,{}},20));
  const auto allied=shield_judgment_reaction(caster,defender,effect,*hit,false,{},20);check(allied&&allied->damage.amount==0&&allied->knockdown_ms==2000);
  check(!shield_judgment_reaction(caster,defender,effect,*hit,{}, {false,{}},20));check(!shield_judgment_reaction(caster,defender,effect,*hit,true,{},20));
  for(unsigned mode=0;mode<10;++mode){auto e=effect;auto h=*hit;auto owner=defender;auto source=caster;
   if(mode==0)++owner.generation;if(mode==1)h.before_damage.reset();if(mode==2)h.before_damage=999;if(mode==3)h.before_damage=1200;
   if(mode==4)e.cause=3;if(mode==5)e.kind=Category::hex;if(mode==6)e.applied=1200;if(mode==7)e.rank=21;if(mode==8)source.generation=0;if(mode==9)h.event=0;
   check(!shield_judgment_reaction(source,owner,e,h,true,{false,{}},20));
  }
 }
 check(!shield_judgment(21));std::cout<<"PASS84 Shield of Judgment rank/melee/ranged/holy-trait cases with failed-contact,scope,timing,ownership and allegiance guards;World integration pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
