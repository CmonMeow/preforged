#include "gw_mending.h"
#include "gw_mending_wire.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mending line "+std::to_string(at.line()));}
int main()try{
 const Scope caster{1,1,1},target{1,2,1},other{1,3,1};
 for(unsigned rank=0;rank<=20;++rank){Registry registry(target);skills::regeneration::State regeneration(target,100);MaintainedEnchantments ownership;const auto plan=*mending(rank);
  const auto result=prepare_mending(plan,caster,target,7,100,registry,regeneration,ownership);check(result&&result->entry.expires==std::nullopt&&result->regeneration.pips(1000000000)==int(plan.pips));check(result->ownership.count(caster,100)==1&&ownership.size()==0&&registry.size()==0);
  auto state=result->regeneration;double health=100;check(state.advance(target,1100,health,1000));check(health==100+2*plan.pips&&state.lookup(target,7)&&!state.lookup(target,7)->ends);
  check(state.end(target,7,2100));check(state.advance(target,4100,health,1000));check(health==100+4*plan.pips&&!state.lookup(target,7));
  auto live=result->regeneration;health=100;check(live.advance(target,1100,health,1000));const auto replacement=prepare_mending(*mending(0),other,target,8,1100,result->registry,live,result->ownership);
  check(replacement&&replacement->removed.size()==1&&replacement->ownership.count(caster,1100)==0&&replacement->ownership.count(other,1100)==1&&replacement->regeneration.pips(1100)==1);
  check(result->ownership.count(caster,100)==1&&result->registry.lookup(target,7));
  auto forged=plan;++forged.upkeep;check(!prepare_mending(forged,caster,target,7,100,registry,regeneration,ownership));check(!prepare_mending(plan,caster,target,7,101,registry,regeneration,ownership));
 }
 {Registry registry(target);skills::regeneration::State regeneration(target,100);MaintainedEnchantments ownership;check(!prepare_mending(*mending(12),{2,1,1},target,7,100,registry,regeneration,ownership));check(!prepare_mending(*mending(12),caster,{1,2,2},7,100,registry,regeneration,ownership));}
 // Permanent sources are reviewed Mending only;ordinary timed sources retain expiry.
 {skills::regeneration::State state(target,100);check(!state.apply(target,{7,1,288,3,100,{}}));check(state.apply(target,{7,1,288,3,100,1100}));double health=100;check(state.advance(target,2100,health,1000)&&health==106&&state.pips(2100)==0);}
 {const Scope source{1,42,1},recipient{1,43,1};Entry entry{71,42,290,12,Category::enchantment,100,{}};const auto wire=mending_wire(source,recipient,entry);check(wire.has_value());
  const combat::Packet upkeep{62,0,43,0,0,0,42,0,0,0,34,1,12,0,0,0,71,0,0,0};auto effect=upkeep;effect[0]=64;check(wire->upkeep==upkeep&&wire->effect==effect);
  entry.expires=1000;check(!mending_wire(source,recipient,entry));entry.expires.reset();++entry.cause;check(!mending_wire(source,recipient,entry));}

 for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<6;++mode){Registry registry(target);skills::regeneration::State regeneration(target,100);MaintainedEnchantments ownership;const auto applied=*prepare_mending(*mending(rank),caster,target,7,100,registry,regeneration,ownership);auto expected=caster;auto recipient=target;unsigned effect=7;if(mode==1)expected=other;if(mode==2)++expected.generation;if(mode==3)++recipient.generation;if(mode==4)effect=8;
  const auto result=prepare_mending_end(expected,recipient,effect,mode==5?99:1100,applied.registry,applied.regeneration,applied.ownership);check(bool(result)==(mode==0));check(applied.registry.lookup(target,7)&&applied.ownership.count(caster,100)==1);
  if(!result)continue;check(!result->registry.lookup(target,7)&&result->ownership.count(caster,1100)==0);auto state=result->regeneration;double health=100;check(state.advance(target,2100,health,1000)&&health==100+2*mending(rank)->pips&&!state.lookup(target,7));check(!prepare_mending_end(caster,target,7,1100,result->registry,result->regeneration,result->ownership));
  const auto packets=mending_end_wire(caster,target,result->entry);check(packets&&packets->upkeep==combat::Packet({63,0,1,0,0,0,7,0,0,0})&&packets->effect==combat::Packet({67,0,2,0,0,0,7,0,0,0}));
 }
 for(unsigned flag:{0u,128u}){const combat::Packet valid{39,static_cast<std::uint8_t>(flag),71,0,0,0};check(decode_drop_maintained(valid)==71);for(unsigned n=0;n<6;++n)check(!decode_drop_maintained(std::span(valid).first(n)));auto extra=valid;extra.push_back(0);check(!decode_drop_maintained(extra));auto zero=valid;zero[2]=0;check(!decode_drop_maintained(zero));auto wrong=valid;wrong[0]=40;check(!decode_drop_maintained(wrong));wrong=valid;wrong[1]=1;check(!decode_drop_maintained(wrong));}
 {Registry registry(target);skills::regeneration::State regeneration(target,100);MaintainedEnchantments ownership;const auto first=*prepare_mending(*mending(12),caster,target,7,100,registry,regeneration,ownership);Registry second_registry(other);skills::regeneration::State second_regeneration(other,100);check(!prepare_mending(*mending(12),caster,other,7,100,second_registry,second_regeneration,first.ownership));}
 {Registry registry(target);skills::regeneration::State regeneration(target,100);MaintainedEnchantments ownership;const auto first=*prepare_mending(*mending(12),caster,target,7,100,registry,regeneration,ownership);check(first.ownership.maintained_target(caster,7,100)==target&&!first.ownership.maintained_target(other,7,100));const auto ended=*prepare_mending_end(caster,target,7,100,first.registry,first.regeneration,first.ownership);check(!prepare_mending(*mending(12),caster,target,7,100,ended.registry,ended.regeneration,ended.ownership));check(prepare_mending(*mending(12),caster,target,8,100,ended.registry,ended.regeneration,ended.ownership).has_value());}
 check(!mending(21));std::cout<<"PASS21 Mending ranks,indefinite regeneration,explicit historical end,cross-caster replacement and plan/scope/clock guards;policy-only; World/Runtime/wire checked separately\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
