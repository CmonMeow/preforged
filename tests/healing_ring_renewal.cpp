#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Healing Ring Renewal line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope owner;
 Fixture(unsigned version){w.join(1,"Ring caster",0,0,box,0,{}, {},100,1);w.join(2,"Recipient",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{100,1000,40,40},100);w.set_vitals(1,{100,1000,50,100},100);owner=*w.effect_scope(1);
  if(version){const auto p=*skills::effects::ether_renewal(version,0);check(w.status_mutation(owner,version,80,0,100,100+p.duration,true));}box->drain();}
};
int main()try{
 unsigned manual=0,paid=0;
 for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<6;++mode){
  Fixture f(version);std::uint64_t at=mode==3&&version?100+skills::effects::ether_renewal(version,0)->duration:200;
  f.w.tick(at);check(f.w.before_spell_effect(f.owner,1262,1,at));f.box->drain();
  if(mode==4)check(f.w.health_capacity(*f.w.effect_scope(2),1001,at+300));
  f.box->drain();
  const auto before=*f.w.resources(1);const auto target_before=f.w.resources(2)->health;auto scope=f.owner;if(mode==2)++scope.generation;
  auto event=mode==1?2u:1u;
  bool ok=f.w.healing_ring(scope,*skills::effects::healing_ring(12,0),at,event);
  if(mode==1||mode==2||mode==4){check(!ok&&f.box->drain().empty());check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);ok=f.w.healing_ring(f.owner,*skills::effects::healing_ring(12,0),mode==4?at+300:at,1);}
  check(ok);const bool reward=version&&mode!=3;
  check(f.w.resources(1)->health==before.health+(reward?5:0)&&f.w.resources(1)->energy==before.energy+(reward?1:0));
  const auto effective=mode==4?at+300:at;
  check(f.w.resources(2)->health==target_before+150);
  f.box->drain();check(!f.w.healing_ring(f.owner,*skills::effects::healing_ring(12,0),effective,1)&&f.box->drain().empty());
  if(mode==5){check(f.w.before_spell_effect(f.owner,1262,2,at+100));const auto again=*f.w.resources(1);check(f.w.healing_ring(f.owner,*skills::effects::healing_ring(12,0),at+100,2));
   check(f.w.resources(1)->health==again.health+(version?5:0)&&f.w.resources(1)->energy==again.energy+(version?1:0));}
  ++manual;
 }
 for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<6;++mode){
  Fixture f(version);skills::casting::Runtime cast(1);skills::casting::Hooks h;unsigned commits=0;std::uint64_t admitted=0;
  h.events=[&](const auto& batch){f.box->enqueue(batch);};h.resources=[&](auto id){return f.w.resources(id);};
  h.consume_energy_at=[&](unsigned cost,auto now){return f.w.consume_energy(1,cost,f.owner,now);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto id,auto,bool){return id==1;};
  h.before_spell_effect=[&](auto skill,auto event,auto now){admitted=event;f.w.tick(now);return f.w.before_spell_effect(f.owner,skill,event,now);};
  h.enchantment_duration_ready=[](auto,auto,auto){return true;};
  h.healing_ring_completed=[&](auto scope,const auto& plan,auto event,auto now){check(event==admitted&&event);++commits;skills::casting::EffectCompletion result;result.accepted=f.w.healing_ring(scope,plan,now,event,&result.publication);return result;};
  h.healing_ring=[](auto,const auto&,auto)->bool{throw std::runtime_error("Typed completion silently downgraded to legacy");};
  skills::Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[1262]=1;skills::Slots bar{};bar[0]={1262,0};attributes::Ranks ranks{};ranks[13]=12;
  check(cast.start(100,{1262,0,1},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);check(f.w.resources(1)->energy==45);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(200,h);if(mode==3)h.healing_ring_completed={};if(mode==4)h.before_spell_effect={};
  if(mode==5){check(f.w.kill(f.owner,200));}
  f.box->drain();cast.update(1100,h);const auto packets=f.box->drain();cast.update(2000,h);check(commits==(mode==0?1u:0u));
  if(mode==0){check(f.w.resources(1)->health==100+(version?5:0)&&f.w.resources(1)->energy==45+(version?1:0));
   const auto finish=std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1));check(finish!=packets.end());
   if(version){const auto heal=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[2]==55;});check(heal<finish);}
   check(std::count(packets.begin(),packets.end(),skills::casting::generic(21,1,499))==1);
  }else if(mode!=5)check(f.w.resources(1)->health==100&&f.w.resources(1)->energy==45);
  ++paid;
 }
 std::cout<<manual<<" Healing Ring Renewal admission/replacement/expiry/current-clock cases and "<<paid<<" paid reward-order/cancel/interrupt/hook-loss/death casts pass\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
