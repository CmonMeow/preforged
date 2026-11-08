#include "gw_wound_spell_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Accumulated Pain completion line "+std::to_string(at.line())+" "+context);
}
struct Fixture {
 world::Instance w;bool npc;Scope source,target;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool is_npc,unsigned version,unsigned hexes,bool conditions=true,unsigned health=480):npc(is_npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),480};
   d.damageable=true;d.conditions_allowed=conditions;n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u}){w.join(id,"Wound actor",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{id==1?500u:health,id==1?1000u:480u,50,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
  if(version){const auto plan=*ether_renewal(version,0);check(w.status_mutation(source,version,80,0,100,100+plan.duration,true));}
  if(hexes)check(w.degeneration_hex(source,target,*degeneration_hex(129,0),100));
  if(hexes>=2)check(w.degeneration_hex(source,target,*degeneration_hex(785,0),100));box->drain();
 }
 double health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 unsigned maximum(){return npc?w.npc_combat_snapshot(target)->health->maximum:w.resources(2)->max_health;}
 bool wound(std::uint64_t at){return w.has_condition(target,combat::conditions::Kind::deep_wound,at);}
 auto completion(){return casting::make_wound_spell_completion(w,[this]{return w.effect_scope(1);},20);}
};
int main()try{unsigned direct=0,paid=0,edges=0;
 for(unsigned skill:{1052u,3184u})for(unsigned rank=0;rank<=20;++rank)for(bool npc:{false,true})
  for(unsigned count:{0u,1u,2u})for(unsigned version:{0u,181u,2860u}){
   context="direct skill="+std::to_string(skill)+" rank="+std::to_string(rank)+" npc="+std::to_string(npc)+" hexes="+std::to_string(count)+" renewal="+std::to_string(version);
   Fixture f(npc,version,count);const auto plan=*wound_spell(skill,rank);check(plan.damage==unsigned(std::lround(skill==1052?15.+4.*rank:10.+25.*rank/15.))&&plan.wound_duration==(5u+rank)*1000u);check(f.w.before_spell_effect(f.source,skill,1,200));
   auto complete=f.completion();auto result=complete(f.target,plan,1,200);check(result.accepted&&bool(result.publication));
   check(f.health()==480-plan.damage-(count>=2?96:0)&&f.maximum()==(count>=2?384:480));
   check(f.wound(200)==(count>=2));check(f.w.resources(1)->health==500+(version?5:0)&&f.w.resources(1)->energy==50+(version?1:0));
   result.publication();f.box->drain();check(!complete(f.target,plan,1,200).accepted&&f.box->drain().empty());++direct;
  }
 for(bool npc:{false,true})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<12;++mode){
  context="paid npc="+std::to_string(npc)+" renewal="+std::to_string(version)+" mode="+std::to_string(mode);
  Fixture f(npc,version,2);casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0,before_health=0,before_energy=0;std::uint64_t admitted=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};h.target_valid=[](auto,float,bool){return true;};h.projectile_target_valid=[](auto,float){return true;};
  h.before_spell_effect=[&](auto used,auto event,auto at){f.w.tick(at);admitted=event;before_health=f.w.resources(1)->health;before_energy=f.w.resources(1)->energy;return f.w.before_spell_effect(f.source,used,event,at);};
  auto native=f.completion();h.wound_spell_completed=[&](auto target,const auto& plan,auto event,auto at){check(event==admitted&&event);check(plan.skill==(mode==11?3184:1052));++calls;return native(target,plan,event,at);};
  h.wound_spell=[](auto,const auto&,auto)->bool{throw std::runtime_error("admitted wound completion downgraded");};
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[1052]=1;policy.pvp_match=mode==11;
  Slots bar{};bar[0]={1052,0};attributes::Ranks ranks{};
  if(mode==9)h.before_spell_effect={};
  if(mode==10){h.wound_spell_completed={};h.wound_spell=[&](auto target,const auto& plan,auto at){++calls;return f.w.wound_spell(f.source,target,plan,20,at);};}
  check(runtime.start(100,{1052,0,f.target.actor},bar,policy,ranks,true,h)==(mode==9?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(101,h);if(mode==3)h.wound_spell_completed={};if(mode==4)h.before_spell_effect={};
  if(mode==5)h.target_scope={};if(mode==6)h.projectile_target_valid={};
  if(mode==7){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},200,1);f.w.set_vitals(1,{500,1000,40,100},100);}
  if(mode==8)h.target_scope=[&](auto id){auto scope=f.w.effect_scope(id);if(scope&&id==f.target.actor)++scope->generation;return scope;};
  if(mode==10)h.wound_spell_completed=[](auto,const auto&,auto,auto)->casting::EffectCompletion{throw std::runtime_error("legacy admission silently upgraded");};
  const auto at=100+std::uint64_t(std::llround(find_skill(1052)->activation*1000));f.box->drain();runtime.update(at,h);const auto packets=f.box->drain();runtime.update(at,h);
  const bool normal=mode==0||mode==11,legacy=mode==10;check(calls==unsigned(normal||legacy));
  if(normal||legacy){const auto plan=*wound_spell(mode==11?3184:1052,0);check(f.health()==480-plan.damage-96&&f.maximum()==384);
   if(normal){check(f.w.resources(1)->health==before_health+(version?5:0)&&f.w.resources(1)->energy==before_energy+(version?1:0));
    const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish!=packets.end());
    check(std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{1052,0},mode==11?15:12))!=packets.end());
    const auto damage=std::find_if(packets.begin(),packets.end(),[&](const auto& p){return p.size()==18&&p[0]==162&&p[2]==55&&p[6]==f.target.actor;});
    check(damage!=packets.end()&&finish<damage);
    combat::Packet target_body{159,0};combat::detail::put(target_body,20);combat::detail::put(target_body,f.target.actor);combat::detail::put(target_body,1);combat::detail::put(target_body,1671);
    const auto body=std::find(packets.begin(),packets.end(),target_body);check(body!=packets.end()&&finish<body&&body<damage&&std::count(packets.begin(),packets.end(),target_body)==1);
    if(version){const auto reward=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162&&p[2]==55&&p[6]==1;});check(reward!=packets.end()&&reward<finish);}
   }else check(f.w.resources(1)->health==500);
  }else check(f.health()==480&&f.w.resources(1)->health==500);++paid;
 }
 for(unsigned skill:{1052u,3184u})for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){
  context="refusal/retry skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  Fixture f(npc,181,2);check(f.w.before_spell_effect(f.source,skill,1,200));auto plan=*wound_spell(skill,0);auto source=f.source,target=f.target;
  if(mode==0)++plan.damage;if(mode==1)++source.generation;if(mode==2)++target.generation;if(mode==3)f.w.tick(300);
  std::vector<combat::Packet> visuals;std::function<void()> publication;f.box->drain();
  check(!f.w.complete_wound_spell(source,target,plan,20,mode==4?2:1,200,&visuals,&publication)&&visuals.empty()&&!publication);
  check(f.health()==480&&f.w.resources(1)->health==500&&f.w.resources(1)->energy==50);
  check(f.w.complete_wound_spell(f.source,f.target,*wound_spell(skill,0),20,1,mode==3?300:200,&visuals,&publication));++edges;
 }
 for(unsigned skill:{1052u,3184u}){Fixture f(true,0,2,false);check(f.w.wound_spell(f.source,f.target,*wound_spell(skill,12),20,200));
  check(f.health()==480-wound_spell(skill,12)->damage&&f.maximum()==480&&!f.wound(200));++edges;}

 for(unsigned skill:{1052u,3184u})for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u}){
  context="expiry skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" rank="+std::to_string(rank);
  Fixture f(npc,0,2);const auto plan=*wound_spell(skill,rank);check(f.w.wound_spell(f.source,f.target,plan,20,200));
  const auto deadline=200+plan.wound_duration;f.w.tick(deadline-100);check(f.wound(deadline-100)&&f.maximum()==384);
  f.w.tick(deadline);check(!f.wound(deadline)&&f.maximum()==480);++edges;
 }
 for(unsigned skill:{1052u,3184u})for(bool npc:{false,true}){
  context="lethal skill="+std::to_string(skill)+" npc="+std::to_string(npc);Fixture f(npc,181,2,true,5);
  check(f.w.before_spell_effect(f.source,skill,1,200));auto result=f.completion()(f.target,*wound_spell(skill,0),1,200);
  check(result.accepted&&f.health()==0&&!f.wound(200)&&f.maximum()==480&&f.w.resources(1)->health==505);++edges;
 }
 for(bool npc:{false,true})for(unsigned skill:{1052u,3184u}){
  context="reward death npc="+std::to_string(npc);Fixture f(npc,181,2);f.w.set_vitals(1,{50,1000,50,100},100);
  check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,60000));
  check(f.w.before_spell_effect(f.source,skill,1,200));auto result=f.completion()(f.target,*wound_spell(skill,0),1,200);
  check(result.accepted&&f.w.resources(1)->health==0&&f.w.resources(1)->energy==50&&f.health()==480&&!f.wound(200)&&!result.publication);++edges;
 }
 for(bool npc:{false,true})for(unsigned skill:{1052u,3184u}){
  context="public wound wrapper event forwarding";Fixture f(npc,181,2);
  check(f.w.before_spell_effect(f.source,skill,1,200));
  std::vector<combat::Packet> visuals;std::function<void()> publication;
  check(f.w.wound_spell(f.source,f.target,*wound_spell(skill,12),20,200,&visuals,&publication,1));
  check(f.w.resources(1)->health==505&&f.w.resources(1)->energy==51&&bool(publication));
  publication();check(!f.w.wound_spell(f.source,f.target,*wound_spell(skill,12),20,200,&visuals,&publication,1));
 }
 std::cout<<"PASS "<<direct<<" all-rank player/NPC/hex-count/Renewal, "<<paid<<" paid routing/alias/cancel/lifetime/retained choice and "<<edges<<" refusal/retry/condition-immunity cases;4 public wrapper event/replay cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
