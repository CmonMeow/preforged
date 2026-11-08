#include "gw_reversal_npc_wire.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Reversal life-steal recipient line "+std::to_string(at.line()));
}
combat::npcs::LiveActor make(unsigned hp,unsigned maximum=1000,std::optional<bool> healing=true){
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={double(hp),maximum};
 d.damageable=d.conditions_allowed=true;d.relation=combat::npcs::Relation::hostile_to_players;d.healing_allowed=healing;
 if(healing.has_value())d.healing_evidence="synthetic";return combat::npcs::LiveActor(d,{1,1,1,146,100});
}
int main()try{unsigned cases=0;
 for(unsigned rank=0;rank<=20;++rank)for(unsigned hp:{1u,300u,1000u})for(double amount:{10.,67.,200.}){
  ++cases;auto a=make(hp);const auto life=a.lifetime();check(a.apply_reversal_fortune(life,rank,1,1,100));
  const auto receipt=a.apply_reversal_life_steal(life,amount,200);const double converted=std::min(amount,double(skills::effects::reversal_fortune(rank)->maximum_healing));
  const double healed=std::min(1000.,hp+converted),after=std::max(0.,healed-(amount-converted));
  check(receipt&&receipt->quote.input==skills::effects::ReversalInput::life_steal&&receipt->quote.reduced_amount==amount);
  check(receipt->before_healing.current==hp&&receipt->after_healing.current==healed&&receipt->after_loss.current==after&&receipt->lost==healed-after);
  check(a.actor(life)->health->current==after&&!a.owned_effect_snapshot(life)->newest_skill(a.hex_scope(),307,200));
  check(!a.apply_reversal_life_steal(life,amount,200));
 }
 check(cases==189);
 // The observed rank3 recipient split:28 healing,17 remaining from45.
 {auto a=make(300,455);const auto life=a.lifetime();check(a.apply_reversal_fortune(life,3,1,1,100));
  const auto r=a.apply_reversal_life_steal(life,45,200);check(r&&r->quote.split.healing==28&&r->quote.split.remaining==17&&r->after_loss.current==311);
 }
 for(unsigned hp:{1u,500u}){auto a=make(hp);const auto life=a.lifetime();check(a.apply_condition(life,combat::conditions::Kind::deep_wound,1,100,10000));check(a.apply_reversal_fortune(life,3,1,1,100));
  combat::deep_wound::State wound;double expected=hp;wound.reset(1000);wound.capacity(expected,1000,true);wound.event(expected,28,true);const auto healed=expected;if(expected>0)wound.event(expected,-17);
  const auto r=a.apply_reversal_life_steal(life,45,200);check(r&&r->after_healing.current==healed&&r->after_loss.current==expected&&a.actor(life)->health->current==expected);
  check(bool(a.actor(life)->effects&0x10)==(expected==0));if(healed==0)check(r->lost==0);
 }
 for(double amount:{0.,-1.,16000001.,std::numeric_limits<double>::quiet_NaN()}){auto a=make(300);const auto life=a.lifetime();check(a.apply_reversal_fortune(life,12,1,1,100));
  check(!a.apply_reversal_life_steal(life,amount,200));check(a.actor(life)->health->current==300&&a.owned_effect_snapshot(life)->newest_skill(a.hex_scope(),307,200));
 }
 for(unsigned mode=0;mode<3;++mode){auto a=make(300);const auto life=a.lifetime();check(a.apply_reversal_fortune(life,12,1,1,100));auto other=life;if(mode==0)++other.actor_generation;
  check(!a.apply_reversal_life_steal(other,45,mode==1?99:mode==2?8100:200));check(a.actor(life)->health->current==300);
 }
 for(auto healing:{std::optional<bool>{false},std::optional<bool>{}}){auto a=make(300,1000,healing);const auto life=a.lifetime();check(!a.apply_reversal_fortune(life,12,1,1,100)&&!a.apply_reversal_life_steal(life,45,200));}
 // Fractional reduced inputs must survive recipient wire validation.
 for(double amount:{100.1,73.3}){auto a=make(500);const auto life=a.lifetime();check(a.apply_reversal_fortune(life,12,1,1,100));
  const auto r=a.apply_reversal_life_steal(life,amount,200);check(r.has_value());const auto packets=combat::npcs::reversal_life_steal_recipient_packets_2022(100,2,*r);
  check(!packets.before_transfer.empty()&&packets.remaining_loss.size()==2);
  auto partial=*r;partial.lost=1;partial.after_loss.current=partial.after_healing.current-1;
  bool rejected=false;try{combat::npcs::reversal_life_steal_recipient_packets_2022(100,2,partial);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 }
 std::cout<<"PASS189 recipient conversions,observed split,wound death,invalid input,lifetime/clock/expiry/permission guards;World caster policy unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
