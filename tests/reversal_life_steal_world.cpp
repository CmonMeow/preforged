#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Life-steal recipient World line "+std::to_string(at.line()));
}
unsigned u32(const world::Packet& p,unsigned at){unsigned value=0;for(unsigned n=0;n<4;++n)value|=unsigned(p.at(at+n))<<(8*n);return value;}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 Fixture(unsigned hp){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={double(hp),1000};d.damageable=d.conditions_allowed=true;
  d.relation=combat::npcs::Relation::hostile_to_players;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,100,100},100);source=*w.effect_scope(1);target=*w.effect_scope(100);
 }
 double health(){return w.npc_combat_snapshot(target)->health->current;}
};
int main()try{unsigned cases=0;
 for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(double amount:{10.,67.,200.}){
  ++cases;Fixture f(hp);check(f.w.stage_npc_reversal(f.target,rr,100));f.box->drain();std::optional<skills::effects::ReversalLifeStealReceipt> receipt;
  check(f.w.npc_reversal_life_steal_recipient(f.source,f.target,amount,200,&receipt)&&receipt.has_value());
  const double converted=std::min(amount,double(skills::effects::reversal_fortune(rr)->maximum_healing)),healed=std::min(1000.,hp+converted),expected=std::max(0.,healed-(amount-converted));
  check(f.health()==expected&&f.w.resources(1)->health==100);const auto packets=f.box->drain();
  auto retired=packets.end(),positive=packets.end(),negative=packets.end();
  for(auto i=packets.begin();i!=packets.end();++i){if(i->size()==10&&(*i)[0]==67&&u32(*i,2)==100&&u32(*i,6)==receipt->removed.instance)retired=i;
   if(i->size()==18&&(*i)[0]==162){check(u32(*i,2)==55&&u32(*i,6)==100);const float delta=std::bit_cast<float>(u32(*i,14));
    if(delta>0){positive=i;check(u32(*i,10)==100&&std::abs(delta-(healed-hp)/1000)<1e-6);}else if(delta<0){negative=i;check(u32(*i,10)==1&&std::abs(delta+(healed-expected)/1000)<1e-6);}
   }
  }
  check(retired!=packets.end());if(healed>hp)check(positive!=packets.end()&&positive<retired);else check(positive==packets.end());
  if(healed>expected)check(negative!=packets.end()&&retired<negative);else check(negative==packets.end());
  check(f.w.take_npc_deaths(f.source).size()==unsigned(expected==0));
  check(!f.w.npc_reversal_life_steal_recipient(f.source,f.target,amount,200,&receipt)&&!receipt&&f.health()==expected&&f.box->drain().empty());
 }
 check(cases==27);
 {Fixture f(10);check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));check(f.w.condition_transfer(f.source,f.target,*skills::effects::condition_transfer(132,12),100));
  check(f.w.stage_npc_reversal(f.target,12,100));f.box->drain();check(f.w.npc_reversal_life_steal_recipient(f.source,f.target,45,200));check(f.health()==0&&f.w.take_npc_deaths(f.source).size()==1);
  const auto packets=f.box->drain();check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162;}));
 }
 {Fixture f(500);check(f.w.stage_npc_watch_yourself_recipient(f.target,f.target,*skills::effects::watch_yourself(0,true),100));check(f.w.stage_npc_reversal(f.target,0,100));
  check(f.w.npc_reversal_life_steal_recipient(f.source,f.target,200,200));const auto entries=*f.w.status_snapshot(f.target,200);check(std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==2858;}));
 }
 {Fixture f(500);check(f.w.stage_npc_reversal(f.target,12,100));auto stale=f.target;++stale.generation;f.box->drain();
  check(!f.w.npc_reversal_life_steal_recipient(f.source,stale,45,200)&&f.health()==500&&f.box->drain().empty());
  for(double amount:{0.,-1.,16000001.,std::numeric_limits<double>::quiet_NaN()})check(!f.w.npc_reversal_life_steal_recipient(f.source,f.target,amount,200));
  std::optional<skills::effects::ReversalLifeStealReceipt> receipt;check(f.w.npc_reversal_life_steal_recipient(f.source,f.target,45,200,&receipt));
  for(unsigned mode=0;mode<4;++mode){auto invalid=*receipt;if(mode==0)invalid.quote.input=skills::effects::ReversalInput::damage;if(mode==1)++invalid.removed.instance;if(mode==2)invalid.lost=1;if(mode==3)++invalid.after_loss.maximum;
   bool rejected=false;try{combat::npcs::reversal_life_steal_recipient_packets_2022(100,1,invalid);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
  }
 }
 std::cout<<"PASS27 recipient-only World conversions,heal-before-retirement-before-loss,one-use/death,non-damage Watch Yourself,stale/invalid receipt guards;no caster payout\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
