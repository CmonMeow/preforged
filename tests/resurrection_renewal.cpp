#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Resurrection Renewal line "+std::to_string(at.line())+" "+context);}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 Fixture(){w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Corpse",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(1,{100,1000,100,200},100);w.set_vitals(2,{1000,1000,50,50},100);
  check(w.party_action(1,158,2,0)&&w.party_action(2,154,1,0));source=*w.effect_scope(1);target=*w.effect_scope(2);check(w.kill(target,100));box->drain();}
 void renewal(unsigned version,unsigned at=100){if(version){const auto plan=*skills::effects::ether_renewal(version,0);check(w.status_mutation(source,version,80,0,at,at+plan.duration,true));}box->drain();}
 bool complete(unsigned skill,unsigned rank,std::uint64_t event,unsigned at,std::function<void()>* publication=nullptr){
  if(const auto p=skills::effects::party_resurrection(skill))return w.resurrect_party(source,*p,at,event,publication);
  const auto p=*skills::effects::resurrection(skill,rank);
  return skill==791?w.flesh_my_flesh(source,target,p,at,publication,event):w.resurrect_target(source,target,p,at,event,publication);
 }
};
int main()try{unsigned manual=0,paid=0;
 for(unsigned skill:{305u,314u,1128u,791u,304u})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<5;++mode){
  context="manual skill="+std::to_string(skill)+" version="+std::to_string(version)+" mode="+std::to_string(mode);Fixture f;f.renewal(version);
  const auto now=mode==3?8100u:200u;f.w.tick(now);check(f.w.before_spell_effect(f.source,skill,1,now));f.box->drain();
  const auto original=f.target;if(mode==2)++f.target.generation;
  if(mode==4)check(f.w.health_capacity(f.target,1001,now+300));f.box->drain();
  const auto before=*f.w.resources(1);std::function<void()> publication;
  auto event=mode==1?2u:1u;bool ok=f.complete(skill,12,event,now,&publication);
  if(mode==1||mode==2||mode==4){
   // Party resurrection has no scalar destination lifetime; exercise source replacement instead.
   if(mode==2&&skill==304){check(ok);}
   else {check(!ok&&!publication&&f.w.resources(2)->health==0&&f.w.resources(1)->health==before.health);f.target=original;ok=f.complete(skill,12,1,mode==4?now+300:now,&publication);}
  }
  check(ok&&bool(publication));const bool reward=version&&mode!=3;const auto healed=before.health+(reward?5:0);
  check(f.w.resources(1)->health==(skill==791?(healed+1)/2:healed));check(f.w.resources(1)->energy==before.energy+(reward?1:0));
  check(f.w.resources(2)->health>0);publication();f.box->drain();publication();check(f.box->drain().empty());
  check(!f.complete(skill,12,1,mode==4?now+300:now));++manual;
 }
 for(unsigned skill:{305u,314u,1128u,791u,304u})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<5;++mode){
  context="paid skill="+std::to_string(skill)+" version="+std::to_string(version)+" mode="+std::to_string(mode);Fixture f;
  skills::casting::Runtime cast(1);skills::casting::Hooks h;unsigned commits=0;
  h.events=[&](const auto& packets){f.box->enqueue(packets);};h.resources=[&](auto id){return f.w.resources(id);};h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto,float,bool){return true;};h.resurrection_target_valid=[&](auto id,float range){return f.w.resurrection_target_valid(f.source,id,range);};
  h.before_spell_effect=[&](unsigned id,auto event,auto at){return f.w.before_spell_effect(f.source,id,event,at);};
  h.resurrection_completed=[&](auto source,auto target,unsigned id,unsigned rank,auto event,auto at){check(source==f.source);if(id!=304)check(target==f.target);++commits;skills::casting::EffectCompletion result;result.accepted=f.complete(id,rank,event,at,&result.publication);if(result.accepted&&id==791){result.visuals.push_back(*skills::particles::targeted_body_2022(target.actor,source.actor,791));result.visuals_before_publication=true;}return result;};
  skills::Policy policy;policy.primary=skill==791?8:3;policy.campaigns=255;policy.learned[skill]=1;skills::Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};ranks.fill(12);
  check(cast.start(100,{skill,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);const auto paid_energy=f.w.resources(1)->energy;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(200,h);if(mode==3)h.resurrection_completed={};if(mode==4)h.before_spell_effect={};
  f.w.tick(20000);f.w.set_vitals(1,{100,1000,f.w.resources(1)->energy,200},100);f.renewal(version,20000);const auto baseline=*f.w.resources(1);cast.update(20000,h);auto packets=f.box->drain();check(commits==(mode==0?1u:0u));
  if(mode==0){const auto health=baseline.health+(version?5:0);check(f.w.resources(1)->health==(skill==791?(health+1)/2:health)&&f.w.resources(2)->health>0);check(f.w.resources(1)->energy==baseline.energy+(version?1:0));
   const auto finish=std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1));check(finish!=packets.end());
   if(skill==791){const auto body=std::find(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(2,1,791));
    const auto sacrifice=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==14&&p[0]==162&&p[2]==55;});
    check(body!=packets.end()&&sacrifice!=packets.end()&&finish<body&&body<sacrifice);}
   if(version){const auto delta=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[2]==55;});check(delta<finish);}
  }else check(f.w.resources(2)->health==0&&f.w.resources(1)->health==baseline.health&&f.w.resources(1)->energy==baseline.energy);
  ++paid;
 }
 for(unsigned signet:{2u,1816u}){Fixture f;const auto before=*f.w.resources(1);std::function<void()> publication;
  check(!f.w.resurrect_target(f.source,f.target,*skills::effects::resurrection(signet,0),200,1,&publication));
  check(!publication&&f.w.resources(2)->health==0&&f.w.resources(1)->energy==before.energy);}
 std::cout<<manual<<" resurrection event/replay/expiry/late-clock/sacrifice cases and "<<paid<<" paid family completion/cancel/interrupt/hook-loss casts pass\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
