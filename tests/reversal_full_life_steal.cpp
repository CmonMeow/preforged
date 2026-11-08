#include "gw_world_simulation.h"
#include <iostream>
#include <fstream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Full Reversal life steal line "+std::to_string(at.line()));
}
unsigned u32(const world::Packet& p,unsigned at){unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(p.at(at+i))<<(8*i);return n;}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target;bool npc;
 Fixture(bool n,unsigned hp,unsigned source_hp=100):npc(n){
  if(npc){world::Instance::Npc a;a.id=a.definition.id=100;a.x=100;a.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,0};d.spawn_health={double(hp),1000};
   d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=d.conditions_allowed=true;
   d.healing_allowed=d.fleshy=true;d.healing_evidence=d.fleshy_evidence="synthetic living recipient";
   a.combat_definition=d;w.configure_npcs({a});}
  w.join(1,"Source",0,0,box,0,{}, {},100,1);w.set_vitals(1,{source_hp,1000,50,100},100);
  check(w.energy_capacity(1,4,0,100,{80,-2}));
  if(!npc){w.join(2,"Target",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
 void reversal(unsigned rank){check(npc?w.stage_npc_reversal(target,rank,100):w.status_mutation(target,307,5,rank,100,8100,true));}
 double health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 bool cast(unsigned skill,unsigned rank){
  if(skill==110)return w.area_life_steal(source,*effects::area_life_steal(skill,rank),200);
  return w.life_steal(source,target,*effects::life_steal(skill,rank),200);
 }
};
int main(int argc,char** argv)try{
 std::ofstream trace;if(argc==2){trace.open(argv[1]);check(bool(trace));}else check(argc==1);unsigned cases=0;
 for(unsigned skill:{153u,156u,1077u,902u,1076u,2189u,110u})for(bool npc:{false,true})
 for(unsigned rank:{0u,12u,20u})for(int rr:{-1,0,12,20})for(unsigned hp:{1u,10u,300u,1000u})for(unsigned source_hp:{100u,990u,1000u}){
  Fixture f(npc,hp,source_hp);if(rr>=0)f.reversal(unsigned(rr));f.box->drain();
  const double amount=skill==110?effects::area_life_steal(skill,rank)->amount:effects::life_steal(skill,rank)->amount;
  const double converted=rr<0?0:std::min(amount,double(effects::reversal_fortune(unsigned(rr))->maximum_healing));
  const double healed=std::min(1000.,hp+converted),stolen=std::min(amount-converted,healed+1),gain=std::min(stolen,1000.-source_hp);
  const double expected=std::max(0.,healed-stolen);
  check(f.cast(skill,rank));
  if(f.health()!=expected||f.w.resources(1)->health!=source_hp+gain)std::cerr<<"skill="<<skill<<" npc="<<npc<<" rank="<<rank<<" rr="<<rr<<" hp="<<hp<<" source="<<source_hp<<" actual="<<f.health()<<'/'<<f.w.resources(1)->health<<" expected="<<expected<<'/'<<source_hp+gain<<'\n';
  check(f.health()==expected&&f.w.resources(1)->health==source_hp+gain);
  const auto status=*f.w.status_snapshot(f.target,200);check(std::none_of(status.begin(),status.end(),[](const auto& e){return e.skill==307;}));
  const auto packets=f.box->drain();auto healing=packets.end(),removed=packets.end(),payout=packets.end(),loss=packets.end();
  for(auto i=packets.begin();i!=packets.end();++i){
   if(i->size()==10&&(*i)[0]==67&&u32(*i,2)==f.target.actor)removed=i;
   if(i->size()==18&&(*i)[0]==162&&u32(*i,2)==55){const auto target=u32(*i,6);const auto delta=std::bit_cast<float>(u32(*i,14));
    if(target==f.target.actor&&delta>0)healing=i;if(target==f.target.actor&&delta<0)loss=i;if(target==1&&delta>0)payout=i;}
  }
  if(rr>=0){check(removed!=packets.end());if(healed>hp)check(healing!=packets.end()&&healing<removed);
   if(gain>0)check(payout!=packets.end()&&removed<payout);else check(payout==packets.end());
   if(stolen>0){check(loss!=packets.end()&&removed<loss);if(gain>0)check(payout<loss);}else check(loss==packets.end());}
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(expected<=0));
  if(trace&&skill==153&&rank==12&&hp==300&&source_hp==100){
   trace<<"{\"npc\":"<<npc<<",\"rr\":"<<rr<<",\"amount\":"<<amount<<",\"initial\":"<<hp<<",\"final\":"<<expected<<",\"source_final\":"<<source_hp+gain<<",\"packets\":[";
   bool first=true;for(const auto& p:packets){if(!first)trace<<',';first=false;trace<<'\"';constexpr char hex[]="0123456789abcdef";for(auto b:p)trace<<hex[b>>4]<<hex[b&15];trace<<'\"';}trace<<"]}\n";
  }
  ++cases;
 }
 // Conversion healing that settles pending wound debt is allowed to kill;
 // no subsequent source payout or recipient loss may then be fabricated.
 for(unsigned skill:{153u,156u,1077u,902u,1076u,2189u,110u})for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  Fixture f(npc,10);check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));
  check(f.w.condition_transfer(f.source,f.target,*effects::condition_transfer(132,12),100));f.reversal(rr);f.box->drain();
  const auto before=f.w.resources(1)->health;check(f.cast(skill,12));check(f.health()==0&&f.w.resources(1)->health==before);
  const auto packets=f.box->drain();check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162&&u32(p,2)==55;}));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==1);++cases;
 }
 // Weakness uses actual survival after conversion, not the raw lethal quote.
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  Fixture f(npc,1);f.reversal(rr);
  if(npc)check(f.w.begin_npc_action(f.target,combat::npcs::control::Action::attack,100,2000).has_value());
  else check(f.w.attack_activity(f.target,2000,100));
  check(f.cast(902,0)&&f.health()==6);check(f.w.has_condition(f.target,combat::conditions::Kind::weakness,200));++cases;
 }
 // Conditional Energy remains a successful-use reward even when all of the
 // life steal is converted. It must not become proportional to source payout.
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u}){
  Fixture f(npc,300);f.reversal(rr);check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,10000));
  const auto plan=*effects::life_steal(2189,0);check(f.cast(2189,0));check(f.w.resources(1)->health==100&&f.w.resources(1)->energy==50+plan.energy_if_condition);++cases;
 }
 // Reduction precedes conversion. A zero incoming event must keep307 and
 // must not invent source gain or settle a nonexistent recipient loss.
 for(unsigned skill:{153u,156u,1077u,902u,1076u,2189u,110u})for(unsigned rank:{0u,12u})
 for(unsigned shielding:{0u,20u})for(int rr:{-1,0,12,20}){
  Fixture f(false,300);check(f.w.protective_enchantment(f.target,f.target,*skills::regeneration::reviewed(299,shielding),0,100));
  if(rr>=0)f.reversal(unsigned(rr));
  const auto amount=skill==110?effects::area_life_steal(skill,rank)->amount:effects::life_steal(skill,rank)->amount;
  const auto reduced=std::max(0.,double(amount)-effects::interpolate(3,18,shielding));
  const double converted=rr<0?0:std::min(reduced,double(effects::reversal_fortune(unsigned(rr))->maximum_healing));
  const double stolen=reduced-converted;check(f.cast(skill,rank));
  check(f.health()==300+converted-stolen&&f.w.resources(1)->health==100+stolen);
  const auto entries=*f.w.status_snapshot(f.target,200);
  const bool retained=std::any_of(entries.begin(),entries.end(),[](const auto& e){return e.skill==307;});
  check(retained==(rr>=0&&reduced==0));++cases;
 }
 // Damage-only armor, caps and per-hit counters do not consume or reduce a
 // life-steal event. NPC publication must keep both owned protections intact.
 for(unsigned skill:{153u,156u,1077u,902u,1076u,2189u,110u})for(int rr:{-1,0,12,20}){
  Fixture f(true,300);check(f.w.stage_npc_shield_absorption(f.target,12,100));
  check(f.w.stage_npc_watch_yourself_recipient(f.target,f.target,*effects::watch_yourself(0,true),100));
  if(rr>=0)f.reversal(unsigned(rr));
  const auto amount=skill==110?effects::area_life_steal(skill,12)->amount:effects::life_steal(skill,12)->amount;
  const double converted=rr<0?0:std::min(double(amount),double(effects::reversal_fortune(unsigned(rr))->maximum_healing));
  check(f.cast(skill,12));check(f.health()==300+converted-(amount-converted)&&f.w.resources(1)->health==100+amount-converted);
  const auto entries=*f.w.status_snapshot(f.target,200);
  for(unsigned skill:{1399u,2858u})check(std::any_of(entries.begin(),entries.end(),[&](const auto& e){return e.skill==skill;}));++cases;
 }
 // Caster payout is Health gain, not healing. A healing penalty on the source
 // cannot reduce it; a penalty on the recipient only modifies conversion heal.
 for(unsigned skill:{153u,156u,1077u,902u,1076u,2189u,110u})for(bool npc:{false,true})for(int rr:{-1,0,12,20}){
  Fixture f(npc,300);check(f.w.status_mutation(f.source,785,80,12,100,8100,true));if(rr>=0)f.reversal(unsigned(rr));
  const auto amount=skill==110?effects::area_life_steal(skill,12)->amount:effects::life_steal(skill,12)->amount;
  const double converted=rr<0?0:std::min(double(amount),double(effects::reversal_fortune(unsigned(rr))->maximum_healing));
  check(f.cast(skill,12));check(f.w.resources(1)->health==100+amount-converted);++cases;
 }
 for(unsigned skill:{153u,156u,1077u,902u,1076u,2189u,110u})for(unsigned rr:{0u,12u,20u})for(bool wound:{false,true}){
  Fixture f(false,300);combat::deep_wound::State capacity;capacity.reset(1000);double expected=300;
  if(wound){check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,100,10000));capacity.capacity(expected,1000,true);}
  else check(f.w.status_mutation(f.target,785,80,12,100,8100,true));f.reversal(rr);
  const auto amount=skill==110?effects::area_life_steal(skill,12)->amount:effects::life_steal(skill,12)->amount;
  const double converted=std::min(double(amount),double(effects::reversal_fortune(rr)->maximum_healing));
  capacity.event(expected,converted,true,wound?1:.67);const auto stolen=std::min(double(amount)-converted,expected+1);capacity.event(expected,-stolen);
  check(f.cast(skill,12));check(f.health()==std::ceil(expected)&&f.w.resources(1)->health==100+stolen);++cases;
 }
 // A real source gain may settle lethal pending capacity debt; remaining
 // recipient loss still belongs to that admitted transfer. A zero payout is
 // not a Health event and must leave the pending source alive.
 for(unsigned skill:{153u,156u,1077u,902u,1076u,2189u,110u})for(bool npc:{false,true})for(unsigned rr:{0u,20u}){
  Fixture f(npc,300,1);check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,10000));f.reversal(rr);
  const auto amount=skill==110?effects::area_life_steal(skill,12)->amount:effects::life_steal(skill,12)->amount;
  const double converted=std::min(double(amount),double(effects::reversal_fortune(rr)->maximum_healing));
  check(f.cast(skill,12));check(f.health()==300+converted-(amount-converted));check(f.w.resources(1)->health==(rr==0?0u:1u));++cases;
 }
 std::cout<<cases<<" full life-steal cases: seven families, player/NPC recipient, source cap, conversion order, wound death, Weakness and conditional Energy passed; paid307 remains unbound\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
