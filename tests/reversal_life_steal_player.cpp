#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Player life-steal recipient line "+std::to_string(at.line()));
}
unsigned u32(const world::Packet& p,unsigned at){unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(p.at(at+i))<<(8*i);return n;}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 Fixture(unsigned hp){w.join(1,"Source",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,100,100},100);
  w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);source=*w.effect_scope(1);target=*w.effect_scope(2);
 }
 void reversal(unsigned rank=12){check(w.status_mutation(target,307,5,rank,100,8100,true));}
};
int main()try{unsigned cases=0;
 for(unsigned rank=0;rank<=20;++rank)for(unsigned hp:{1u,300u,1000u})for(double amount:{10.,67.,200.})for(unsigned style=0;style<3;++style){
  ++cases;Fixture f(hp);f.reversal(rank);combat::deep_wound::State wound;wound.reset(1000);double expected=hp,factor=1;
  if(style==1){check(f.w.status_mutation(f.target,785,6,12,100,8100,true));factor=.67;}
  if(style==2){check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));wound.capacity(expected,1000,true);}
  const double initial=expected;const double converted=std::min(amount,double(skills::effects::reversal_fortune(rank)->maximum_healing));wound.event(expected,converted,true,factor);const double healed=expected;if(expected>0)wound.event(expected,-(amount-converted));
  f.box->drain();std::optional<world::Instance::PlayerReversalLifeStealReceipt> receipt;
  check(f.w.player_reversal_life_steal_recipient(f.source,f.target,amount,200,&receipt)&&receipt.has_value());
  check(f.w.resources(2)->health==unsigned(std::ceil(expected))&&f.w.resources(1)->health==100);
  check(std::abs(receipt->after_healing.current-healed)<1e-7&&std::abs(receipt->after_loss.current-expected)<1e-7);
  check(f.w.resources(2)->max_health==(expected==0?1000:wound.maximum()));const auto packets=f.box->drain();auto retired=packets.end(),positive=packets.end(),negative=packets.end();unsigned retirements=0;
  for(auto i=packets.begin();i!=packets.end();++i){if(i->size()==10&&(*i)[0]==67&&u32(*i,2)==2&&u32(*i,6)==5){retired=i;++retirements;}
   if(i->size()==18&&(*i)[0]==162&&u32(*i,2)==55&&u32(*i,6)==2){const auto delta=std::bit_cast<float>(u32(*i,14));if(delta>0)positive=i;else if(delta<0)negative=i;}
  }
  if(retirements!=1||((healed>initial)!=(positive!=packets.end()))||(positive!=packets.end()&&!(positive<retired)))
   throw std::runtime_error("ordering rank="+std::to_string(rank)+" hp="+std::to_string(hp)+" amount="+std::to_string(amount)+" style="+std::to_string(style)+" retired="+std::to_string(retirements)+" positive="+std::to_string(positive!=packets.end())+" healed="+std::to_string(healed));
  check(retirements==1);if(healed>initial)check(positive!=packets.end()&&positive<retired);else check(positive==packets.end());
  if(healed>expected)check(negative!=packets.end()&&retired<negative);else check(negative==packets.end());
  check(!f.w.player_reversal_life_steal_recipient(f.source,f.target,amount,200,&receipt)&&!receipt&&f.box->drain().empty());
 }
 check(cases==567);
 {Fixture f(500);check(f.w.stage_watch_yourself_recipient(f.target,f.target,*skills::effects::watch_yourself(0,true),100));f.reversal(0);
  check(f.w.player_reversal_life_steal_recipient(f.source,f.target,200,200)&&f.w.watch_yourself_armor(f.target,200)>0u);
 }
 {Fixture f(500);f.reversal();auto stale=f.target;++stale.generation;f.box->drain();check(!f.w.player_reversal_life_steal_recipient(f.source,stale,45,200)&&f.w.resources(2)->health==500&&f.box->drain().empty());
  for(double amount:{0.,-1.,16000001.,std::numeric_limits<double>::quiet_NaN()})check(!f.w.player_reversal_life_steal_recipient(f.source,f.target,amount,200));
  check(f.w.player_reversal_life_steal_recipient(f.source,f.target,45,200));
 }
 for(double amount:{100.1,73.3}){Fixture f(500);f.reversal();std::optional<world::Instance::PlayerReversalLifeStealReceipt> receipt;
  check(f.w.player_reversal_life_steal_recipient(f.source,f.target,amount,200,&receipt));check(receipt&&receipt->lost>0&&std::abs(receipt->before_loss.current-receipt->after_loss.current-receipt->lost)<1e-9);
 }
 for(unsigned mode=0;mode<3;++mode){Fixture f(500);f.reversal();
  if(mode==2){f.w.join(3,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);f.w.set_vitals(3,{500,1000,100,100},100);const auto ally=*f.w.effect_scope(3);check(f.w.status_mutation(ally,307,5,12,100,8100,true));check(!f.w.player_reversal_life_steal_recipient(f.source,ally,45,200));}
  else check(!f.w.player_reversal_life_steal_recipient(f.source,f.target,45,mode==0?99:8100));
  check(f.w.resources(2)->health==500&&f.w.resources(1)->health==100);
 }
 std::cout<<"PASS567 player recipient conversions/penalties/wound settlements,owned ordering,one-use,non-damage/invalid guards;no caster payout\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
