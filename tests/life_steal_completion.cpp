#include "gw_life_steal_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Life-steal production adapter line "+std::to_string(at.line())+" "+context);
}
std::optional<combat::Packet> target_visual(unsigned target,unsigned skill){
 // Pinned native target resources; touch skills have no target particle.
 const unsigned resource=skill==153?276:skill==902?170:skill==1076?1178:skill==2189?1818:0;
 if(!resource)return {};combat::Packet packet{159,0};
 for(unsigned value:{20u,target,1u,resource})combat::detail::put(packet,value);return packet;
}
struct Fixture {
 world::Instance w;Scope source,target;bool npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 explicit Fixture(bool is_npc):npc(is_npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,1000};
   d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==1?100u:500u,1000,50,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
 double target_health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
};
int main()try{unsigned direct=0,paid=0;
 for(unsigned skill:{2189u,1076u,902u,153u,156u,1077u})for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u}){
  context="direct skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" rank="+std::to_string(rank);
  Fixture f(npc);auto complete=casting::make_life_steal_completion(f.w,[&]{return std::optional{f.source};});
  const auto plan=*life_steal(skill,rank);const auto result=complete(f.target,plan,100);check(result.accepted);
  check(f.w.resources(1)->health==100+plan.amount&&f.target_health()==500-plan.amount);
  const auto visual=target_visual(f.target.actor,skill);
  check(result.visuals==(visual?std::vector<combat::Packet>{*visual}:std::vector<combat::Packet>{}));++direct;
 }
 for(unsigned skill:{2189u,1076u,902u,153u,156u,1077u})for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  context="paid skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  Fixture f(npc);auto native=casting::make_life_steal_completion(f.w,[&]{return std::optional{f.source};});
  casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0;bool stale_target=false;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){auto scope=f.w.effect_scope(id);if(scope&&stale_target&&id==f.target.actor)++scope->generation;return scope;};
  h.events=[&](const auto& packets){f.box->enqueue(packets);};h.target_valid=[&](auto id,float,bool){return id==f.target.actor;};
  h.projectile_target_valid=[&](auto id,float){return id==f.target.actor;};h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.life_steal=[](auto,const auto&,auto)->bool{throw std::runtime_error("production life-steal callback downgraded");};
  h.complete_life_steal=[&](auto target,const auto& plan,auto at){++calls;return native(target,plan,at);};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;policy.pvp_match=mode%2;
  Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
  check(runtime.start(100,{skill,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(101,h);if(mode==3)h.complete_life_steal={};
  if(mode==4){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},200,1);f.w.set_vitals(1,{100,1000,40,100},100);}
  if(mode==5){if(npc)stale_target=true;else{f.w.leave(2);f.w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},200,2);f.w.set_vitals(2,{500,1000,50,100},100);}}
  if(mode==6)h.projectile_target_valid={};
  const auto at=100+std::uint64_t(std::llround(find_skill(skill)->activation*1000));f.box->drain();runtime.update(at,h);
  const auto packets=f.box->drain();runtime.update(at+100,h);check(calls==unsigned(mode==0));
  if(mode==0){const auto plan=*life_steal(skill,0);check(f.w.resources(1)->health==100+plan.amount&&f.target_health()==500-plan.amount);
   const auto visual=target_visual(f.target.actor,skill);
   if(visual){const auto body=std::find(packets.begin(),packets.end(),*visual);
    const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish<body&&body!=packets.end()&&std::count(packets.begin(),packets.end(),*visual)==1);}
   if(skill==1076||skill==156||skill==1077){combat::Packet caster{158,0};
    for(unsigned value:{21u,1u,skill==1076?1177u:280u})combat::detail::put(caster,value);
    const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1)),body=std::find(packets.begin(),packets.end(),caster);
    check(finish<body&&body!=packets.end()&&std::count(packets.begin(),packets.end(),caster)==1);}}
  else{check(f.w.resources(1)->health==100);if(mode!=5)check(f.target_health()==500);}
  ++paid;
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  context="invalid npc="+std::to_string(npc)+" mode="+std::to_string(mode);Fixture f(npc);std::optional<Scope> owner=f.source;
  auto complete=casting::make_life_steal_completion(f.w,[&]{return owner;});auto plan=*life_steal(2189,0);auto target=f.target;
  if(mode==0)owner.reset();if(mode==1)++owner->generation;if(mode==2)++target.generation;
  if(mode==3)++plan.amount;if(mode==4)plan={110,0,10,322};
  const auto result=complete(target,plan,100);check(!result.accepted&&result.visuals.empty());
  check(f.w.resources(1)->health==100&&f.target_health()==500);
 }
 for(bool npc:{false,true}){
  context="qualified Energy npc="+std::to_string(npc);Fixture f(npc);
  check(f.w.apply_condition(*f.w.effect_scope(2),f.source,combat::conditions::Kind::blind,100,10000));
  auto complete=casting::make_life_steal_completion(f.w,[&]{return std::optional{f.source};});
  check(complete(f.target,*life_steal(2189,0),100).accepted);check(f.w.resources(1)->energy==53);
 }
 for(unsigned skill:{153u,156u,1077u})for(bool npc:{false,true}){
  context="legacy retained skill="+std::to_string(skill)+" npc="+std::to_string(npc);Fixture f(npc);casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){f.box->enqueue(p);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.target_valid=[&](auto id,float,bool){return id==f.target.actor;};h.projectile_target_valid=[&](auto id,float){return id==f.target.actor;};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.life_steal=[&](auto target,const auto& plan,auto at){++calls;return f.w.life_steal(f.source,target,plan,at);};
  Policy p;p.primary=4;p.campaigns=255;p.learned[skill]=1;Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
  check(runtime.start(100,{skill,0,f.target.actor},bar,p,ranks,true,h)==casting::Reason::accepted);
  h.complete_life_steal=[](auto,const auto&,auto)->casting::EffectCompletion{throw std::runtime_error("legacy admission silently upgraded");};
  runtime.update(100+std::uint64_t(std::llround(find_skill(skill)->activation*1000)),h);check(calls==1);
 }
 std::cout<<"PASS "<<direct<<" production family adapter transfers and "<<paid<<" paid routing/visual/cancel/hook/lifetime cases;10 invalid refusals,2 conditional Energy and6 retained legacy cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
