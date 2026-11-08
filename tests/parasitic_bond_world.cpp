#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Parasitic Bond world regression line "+std::to_string(at.line()));}
void setup(world::Instance& w,unsigned source=1,std::vector<std::shared_ptr<world::Outbox>>* queues=nullptr){
 if(queues)queues->resize(4);
 for(unsigned id:{1u,2u,3u}){auto box=std::make_shared<world::Outbox>();if(queues)(*queues)[id]=box;w.join(id,"Actor",float(id*100),0,box,0,{}, {},100,id==source?1:2);w.set_vitals(id,{id==source?100u:500u,1000,100,100},100);}
}
double last_health(const std::vector<world::Packet>& packets,unsigned actor){
 for(auto it=packets.rbegin();it!=packets.rend();++it){const auto& p=*it;
  if(p.size()!=14||p[0]!=161||p[2]!=34)continue;
  unsigned id=0,bits=0;for(unsigned i=0;i<4;++i){id|=unsigned(p[6+i])<<(8*i);bits|=unsigned(p[10+i])<<(8*i);}
  if(id==actor)return std::bit_cast<float>(bits)*1000.;
 }throw std::runtime_error("missing authoritative Health publication");
}
int main()try{
 using namespace skills;
 for(unsigned source:{1u,2u})for(unsigned rank:{0u,12u,20u}){
  world::Instance w;setup(w,source);const auto a=*w.effect_scope(source),b=*w.effect_scope(source==1?2:1);
  check(w.apply_parasitic_bond(a,b,rank,100));w.tick(20100);
  check(w.resources(source)->health==130+6*rank&&w.resources(b.actor)->health==460);
  w.tick(40100);check(w.resources(source)->health==130+6*rank&&w.resources(b.actor)->health==460);
 }
 for(unsigned source:{1u,2u})for(unsigned cadence:{100u,40000u}){
  world::Instance w;std::vector<std::shared_ptr<world::Outbox>> queues;setup(w,source,&queues);const auto a=*w.effect_scope(source),b=*w.effect_scope(source==1?2:1);
  w.set_vitals(source,{220,1000,100,100},100);
  check(w.apply_condition(b,a,combat::conditions::Kind::bleeding,100,40000));
  check(w.apply_parasitic_bond(a,b,0,100));
  for(std::uint64_t now=100+cadence;now<=40100;now+=cadence)w.tick(now);
  const auto packets=queues[source]->drain();
  check(std::abs(last_health(packets,source)-10)<.001&&std::abs(last_health(packets,b.actor)-460)<.001);
  // Existing unsigned ceil projection can differ by1 due to sub-ulp drift;
  // authoritative fraction above must still match across tick cadences.
  check(w.resources(source)->health>=10&&w.resources(source)->health<=11);

 }
 for(unsigned source:{1u,2u})for(unsigned cadence:{100u,40000u}){
  world::Instance w;std::vector<std::shared_ptr<world::Outbox>> queues;setup(w,source,&queues);
  const auto a=*w.effect_scope(source),b=*w.effect_scope(source==1?2:1),c=*w.effect_scope(3);
  check(w.apply_condition(b,a,combat::conditions::Kind::bleeding,100,40000));
  check(w.apply_parasitic_bond(a,b,0,100)&&w.apply_parasitic_bond(a,c,12,1100));
  const auto before=*w.control_epoch(a);
  for(std::uint64_t now=1100+cadence;now<40100;now+=cadence)w.tick(now);
  w.tick(40100);
  check(w.resources(source)->health==0&&w.resources(b.actor)->health==467&&w.resources(3)->health==469);
  check(w.control_event_time(a,before)==16767);
  const auto packets=queues[b.actor]->drain();
  check(std::abs(last_health(packets,b.actor)-466.666)<.003&&std::abs(last_health(packets,3)-468.666)<.003);
  check(w.status_snapshot(b,40100)->empty()&&w.status_snapshot(c,40100)->empty());
 }
 for(unsigned dying:{1u,2u})for(unsigned cadence:{100u,40000u}){
  world::Instance w;std::vector<std::shared_ptr<world::Outbox>> queues;setup(w,dying,&queues);
  const auto a=*w.effect_scope(dying),b=*w.effect_scope(dying==1?2:1);
  check(w.apply_condition(b,a,combat::conditions::Kind::bleeding,100,40000));
  check(w.apply_parasitic_bond(a,b,0,100)&&w.apply_parasitic_bond(b,a,12,1100));
  for(std::uint64_t now=1100+cadence;now<40100;now+=cadence)w.tick(now);
  w.tick(40100);
  check(w.resources(a.actor)->health==0&&w.resources(b.actor)->health==577);
  check(std::abs(last_health(queues[b.actor]->drain(),b.actor)-576.5)<.003);
 }
 for(unsigned source:{1u,2u})for(unsigned cadence:{100u,40000u}){
  world::Instance w;setup(w,source);const auto a=*w.effect_scope(source),b=*w.effect_scope(source==1?2:1);
  w.set_vitals(b.actor,{10,1000,100,100},100);
  check(w.apply_condition(b,a,combat::conditions::Kind::bleeding,100,40000)&&w.apply_parasitic_bond(a,b,0,100));
  const auto before=*w.control_epoch(a),victim_before=*w.control_epoch(b);
  for(std::uint64_t now=100+cadence;now<=40100;now+=cadence)w.tick(now);
  check(w.resources(a.actor)->health==0&&w.resources(b.actor)->health==0);
  check(w.control_event_time(b,victim_before)==5100&&w.control_event_time(a,before)==21767);
 }
 for(unsigned operation:{0u,1u,2u,3u}){
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  check(w.apply_parasitic_bond(a,b,12,100));auto entry=w.status_snapshot(b,100)->front();
  if(operation==0)check(w.status_mutation(b,99,entry.instance,12,1100,{},false));
  if(operation==1)check(w.kill(b,1100));
  if(operation==2)check(w.kill(a,1100));
  if(operation==3)w.leave(1);
  if(operation<2)check(w.resources(1)->health==202);
  w.tick(40100);
  if(operation<2)check(w.resources(1)->health==202);
  if(operation==2)check(w.resources(1)->health==0&&w.resources(2)->health==498);
  if(operation==3)check(w.resources(2)->health==500);
 }
 {
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  check(w.apply_parasitic_bond(a,b,0,100)&&w.apply_parasitic_bond(a,b,12,1100));
  w.tick(20100);check(w.resources(1)->health==100);w.tick(21100);check(w.resources(1)->health==202&&w.resources(2)->health==458);
 }
 {
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2),c=*w.effect_scope(3);
  check(w.degeneration_hex(b,a,*effects::degeneration_hex(129,12),100)&&w.apply_parasitic_bond(a,b,12,100));
  w.tick(20100);check(w.resources(1)->health==168);
  auto stale=b;++stale.generation;check(!w.apply_parasitic_bond(a,stale,0,20100)&&!w.apply_parasitic_bond(a,a,0,20100));
 }
 {
  // The first return settles lethal Deep Wound debt. Its caster-death callback
  // must cancel the later bond before the dispatcher reaches that deadline.
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2),c=*w.effect_scope(3);
  w.set_vitals(1,{50,1000,100,100},100);
  check(w.apply_condition(b,a,combat::conditions::Kind::deep_wound,100,60000));
  check(w.resources(1)->health==1&&w.apply_parasitic_bond(a,b,0,100)&&w.apply_parasitic_bond(a,c,12,1100));
  w.tick(40100);check(w.resources(1)->health==0&&w.resources(3)->health==462);
  check(w.status_snapshot(c,40100)->empty());
 }
 {
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  check(w.apply_parasitic_bond(a,b,12,100));w.leave(2);
  w.join(2,"Replacement",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(2,{500,1000,100,100},100);w.tick(40100);
  check(w.resources(1)->health==100&&w.resources(2)->health==500);
 }
 for(unsigned rank:{0u,12u,20u})for(bool npc_target:{false,true})for(unsigned mode=0;mode<6;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=200;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=true;d.spawn_health=combat::Health{mode==5?4.:500.,500};n.combat_definition=d;w.configure_npcs({n});std::vector<std::shared_ptr<world::Outbox>> queues;setup(w,1,&queues);
  const auto caster=*w.effect_scope(1),recipient=*w.effect_scope(npc_target?100:2);unsigned commits=0;bool replaced=false;
  if(mode==5&&!npc_target)w.set_vitals(2,{4,1000,100,100},100);
  Slots bar{};bar[0]={99,0};Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[99]=1;attributes::Ranks ranks{};ranks[7]=rank;
  std::vector<Packet> emitted;casting::Hooks h;h.events=[&](const auto& packets){emitted.insert(emitted.end(),packets.begin(),packets.end());w.skill_event(1,packets);};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(1,cost,caster,at);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==recipient.actor&&range==1248;};
  h.target_scope=[&](unsigned id){auto scope=w.effect_scope(id);if(replaced&&scope)++scope->generation;return scope;};
  if(mode!=4)h.degeneration_hex=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;std::vector<Packet> visuals;const bool applied=w.degeneration_hex(caster,target,plan,at,20,&visuals);
   const auto early=queues[1]->drain();const auto body=*particles::targeted_body_2022(target.actor,1,99);
   check(std::count(early.begin(),early.end(),body)==0);return {applied,std::move(visuals)};
  };
  casting::Runtime cast(1);check(cast.start(1000,{99,0,recipient.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(w.resources(1)->energy==(mode==4?100u:95u));
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1500,h);if(mode==3)replaced=true;
  cast.update(1999,h);check(!commits);cast.update(2000,h);cast.update(2000,h);check(commits==unsigned(mode==0||mode==5));
  const Packet caster_body{158,0,21,0,0,0,1,0,0,0,183,0,0,0};
  check(std::count(emitted.begin(),emitted.end(),caster_body)==unsigned(mode==0||mode==5));
  if(mode==0||mode==5){
   const auto finish=std::find(emitted.begin(),emitted.end(),casting::terminal(58,1));
   const auto body=std::find(emitted.begin(),emitted.end(),caster_body);
   const auto effect=std::find(emitted.begin(),emitted.end(),*particles::targeted_body_2022(recipient.actor,1,99));
   check(finish<body&&body<effect&&effect!=emitted.end());
  }
  const auto target_body=*particles::targeted_body_2022(recipient.actor,caster.actor,99);
  const auto published=queues[1]->drain();check(std::count(published.begin(),published.end(),target_body)==unsigned(mode==0||mode==5));
  const auto plan=*effects::degeneration_hex(99,rank);auto forged=plan;++forged.pips;check(!w.degeneration_hex(caster,recipient,forged,2000));
  check(!w.degeneration_hex(caster,recipient,plan,2000,0));
  w.tick(22000);
  check(w.resources(1)->health==100+((mode==0||mode==5)?30+6*rank:0));
  const auto expected=mode==0?460u:mode==5?0u:500u;
  if(npc_target)check(w.npc_combat_snapshot(recipient)->health->current==expected);
  else check(w.resources(2)->health==expected);
  w.tick(42000);check(w.resources(1)->health==100+((mode==0||mode==5)?30+6*rank:0));
 }
 for(unsigned id:{99u,103u,31u,53u,137u,123u,150u,101u,35u,129u,108u,142u})for(bool npc_target:{false,true})for(bool reject:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=200;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=true;d.spawn_health=combat::Health{500,500};n.combat_definition=d;w.configure_npcs({n});
  std::vector<std::shared_ptr<world::Outbox>> queues;setup(w,1,&queues);const auto source=*w.effect_scope(1),target=*w.effect_scope(npc_target?100:2);
  const auto* row=find_skill(id);Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};Policy policy;policy.primary=row->profession;policy.campaigns=255;policy.learned[id]=1;attributes::Ranks ranks{};ranks[row->attribute]=12;
  std::vector<Packet> emitted;casting::Hooks hooks;hooks.resources=[&](unsigned actor){return w.resources(actor);};hooks.events=[&](const auto& packets){emitted.insert(emitted.end(),packets.begin(),packets.end());w.skill_event(1,packets);};
  hooks.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(1,cost,source,at);};hooks.target_scope=[&](unsigned actor){return w.effect_scope(actor);};hooks.projectile_target_valid=[&](unsigned actor,float range){return actor==target.actor&&range==1248;};
  hooks.activation_multiplier=[&](unsigned skill,auto at){return w.activation_multiplier(source,skill,at);};
  hooks.health_sacrifice=[&](unsigned skill,auto at){return w.health_sacrifice(source,skill,at);};
  const auto target_body=*particles::targeted_body_2022(target.actor,1,id);
  hooks.degeneration_hex=[&](auto recipient,const auto& plan,auto at)->casting::EffectCompletion{
   if(reject)return {false,{target_body}};
   std::vector<Packet> visuals;const bool applied=w.degeneration_hex(source,recipient,plan,at,20,&visuals);
   const auto early=queues[1]->drain();check(std::find(early.begin(),early.end(),target_body)==early.end());return {applied,std::move(visuals)};
  };
  casting::Runtime cast(1);const auto admission=cast.start(1000,{id,0,target.actor},bar,policy,ranks,true,hooks);if(admission!=casting::Reason::accepted)std::cerr<<"hex="<<id<<" npc="<<npc_target<<" reason="<<casting::reason_text(admission)<<"\n";check(admission==casting::Reason::accepted);
  cast.update(1000+static_cast<std::uint64_t>(std::llround(row->activation*1000)),hooks);
  const auto finish=std::find(emitted.begin(),emitted.end(),casting::terminal(58,1));const auto target_effect=std::find(emitted.begin(),emitted.end(),target_body);
  if(reject)check(finish==emitted.end()&&target_effect==emitted.end());
  else {
   if(!(finish<target_effect&&target_effect!=emitted.end()))throw std::runtime_error("hex order id="+std::to_string(id)+" npc="+std::to_string(npc_target)+" finish="+std::to_string(finish!=emitted.end())+" target="+std::to_string(target_effect!=emitted.end()));
   if(const auto body=casting::success_body_effect(1,id)){const auto caster=std::find(emitted.begin(),emitted.end(),*body);check(finish<caster&&caster<target_effect);}
  }
 }
 std::cout<<"PASS player world Parasitic Bond expiry/removal/death/renewal and sparse/dense Health order;actual Runtime binding verified\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
