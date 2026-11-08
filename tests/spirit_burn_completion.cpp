#include "gw_elemental_completion.h"
#include "gw_quickening_zephyr_spawn.h"
#include <iostream>
#include <source_location>
#include <fstream>
#include <sstream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Spirit Burn completion line "+std::to_string(at.line()));
}
static std::vector<world::Instance::Npc> templates(unsigned level){
 std::vector<world::Instance::Npc> result;
 for(unsigned i=0;i<3;++i){
  world::Instance::Npc n;n.definition.id=400+i;n.definition.model_file=1234;
  n.definition.visual_adjustment={0,0,0,100};n.definition.level=std::uint8_t(level);
  n.x=float(i*40);n.y=0;
  combat::npcs::Definition d;d.binding.map=146;d.binding.definition=n.definition.id;
  d.binding.model_file=n.definition.model_file;d.binding.level=n.definition.level;
  d.relation=combat::npcs::Relation::allied_to_players;
  d.spawn_health=combat::Health{30,50};d.damageable=true;d.conditions_allowed=true;
  d.healing_allowed=false;d.healing_evidence="archived spirit mechanics";
  d.fleshy=false;d.fleshy_evidence="archived spirit mechanics";
  n.combat_definition=d;result.push_back(n);
 }
 return result;
}

struct Fixture {
 std::unique_ptr<world::Instance> storage=std::make_unique<world::Instance>();world::Instance& w=*storage;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope source,target;
 Fixture(std::optional<bool> witness={},double health=100){
  // Static population belongs to instance setup, before any player is joined.
  // Actor IDs stay1/2/100; all scopes are obtained after authoritative setup.
  if(witness){
   auto n=templates(1).front();n.id=n.definition.id=100;n.x=100;
   auto& d=*n.combat_definition;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={health,100};
   if(*witness){d.creature_type="spirit";d.creature_type_evidence="synthetic census witness";}
   w.configure_npcs({n});
  }
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{500,500,50,100},100);w.set_vitals(2,{500,500,50,100},100);
  source=*w.effect_scope(1);target=*w.effect_scope(2);check(w.lightning_armor(target,60));
 }
};
int main(int argc,char** argv)try{
 bool direct_events=false;std::string trace_path;
 for(int i=1;i<argc;++i){if(std::string(argv[i])=="--direct-events")direct_events=true;else{check(trace_path.empty());trace_path=argv[i];}}
 std::ofstream trace;if(!trace_path.empty()){trace.open(trace_path,std::ios::binary|std::ios::trunc);check(bool(trace));trace<<"{\"direct_events\":"<<(direct_events?"true":"false")<<",\"cases\":[";}

 // The census must accept an untouched living NPC's absent Health clock.
 // Advancing such an actor intentionally leaves the unstarted timeline absent.
 {auto d=*templates(1).front().combat_definition;d.binding.actor=400;
  combat::npcs::LiveActor untouched(d,{1,1,1,146,400});const auto life=untouched.lifetime();
  check(!untouched.health_clock(life));check(!untouched.advance_conditions(life,200));
  check(!untouched.health_clock(life)&&untouched.actor(life)->health->current>0);
 }

 // Real registry lifetimes, direct completion before tick and after tick agree.
 for(bool qz:{false,true})for(bool expired:{false,true})for(bool tick:{false,true}){
  Fixture f;std::vector<world::Packet> packets;std::uint64_t deadline;
  if(qz){const auto p=*quickening_zephyr(0);deadline=1000+p.lifetime;
   const auto spirit=f.w.spawn_quickening_zephyr_spirit(f.source,p,*world::quickening_zephyr_candidate_template(146,p),1000,packets);
   check(bool(spirit));check(f.w.publish_quickening_zephyr_spirit(*spirit,1000));
  }else{const auto p=*spirits::signet_plan(0);deadline=1000+p.lifetime;
   check(bool(f.w.spawn_controlled_spirits(f.source,p,templates(p.level),1000,packets)));}
  const auto at=deadline-(expired?0:1);if(tick)f.w.tick(at);
  check(f.w.elemental(f.source,f.target,*elemental(919,15),20,at));
  check(f.w.has_condition(f.target,combat::conditions::Kind::burning,at)==!expired);
 }
 // A live known witness dominates an unknown neighbor; an expired one cannot.
 for(bool expired:{false,true}){Fixture f(false);std::vector<world::Packet> packets;
  const auto p=*spirits::signet_plan(0);check(bool(f.w.spawn_controlled_spirits(f.source,p,templates(p.level),1000,packets)));
  const auto at=1000+p.lifetime-(expired?0:1);
  check(f.w.elemental(f.source,f.target,*elemental(919,15),20,at)==!expired);
  check(f.w.has_condition(f.target,combat::conditions::Kind::burning,at)==!expired);
  if(expired)check(f.w.resources(2)->health==500);
 }
 // A stationary spirit dies from Burning even without the periodic tick.
 for(bool tick:{false,true}){Fixture f(true,1);const auto witness=*f.w.effect_scope(100);
  check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::burning,100,5000));
  check(f.w.condition_transfer(f.source,witness,*condition_transfer(154),100));
  if(tick)f.w.tick(1100);
  check(f.w.elemental(f.source,f.target,*elemental(919,15),20,1100));
  check(!f.w.has_condition(f.target,combat::conditions::Kind::burning,1100));
  const auto dead=f.w.npc_combat_snapshot(witness);check(!dead||!dead->health||dead->health->current<=0);
 }
 // Paid Runtime uses the same admitted callback as gw_auth; no legacy fallback.
 for(unsigned mode=0;mode<3;++mode){Fixture f(mode?std::optional<bool>{mode==1}:std::nullopt);
  casting::Hooks h;std::uint64_t publication_clock=500;unsigned commits=0,admissions=0;std::uint64_t admitted=0;std::function<void()> copied;
  h.prepare_completion_publication=[&](auto owner,auto at){return f.w.prepare_completion_publication(owner,at);};
  h.events=[&](const auto& packets){if(direct_events)f.box->enqueue(packets);else f.w.skill_event_live(1,packets,[&]{return publication_clock;});};
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.projectile_target_valid=[](auto id,float){return id==2;};
  h.consume_energy_at=[&](auto cost,auto at){check(cost==5);return f.w.consume_energy(1,cost,f.source,at);};
  h.before_spell_effect=[&](auto skill,auto event,auto at){++admissions;admitted=event;return f.w.before_spell_effect(f.source,skill,event,at);};
  unsigned current_level=0,level_reads=0;
  auto consumer=casting::make_admitted_elemental_completion(f.w,[&]{return std::optional{f.source};},[&]{++level_reads;return current_level;});
  current_level=20; // A hook created at another level must read the completion level.
  h.elemental_completed=[&](auto target,const auto& p,auto event,auto at){++commits;auto result=consumer(target,p,event,at);if(result)copied=result->publication;return result;};
  h.elemental=[](auto,const auto&,auto)->casting::EffectCompletion{throw std::runtime_error("legacy callback used");};
  Policy policy;policy.primary=8;policy.campaigns=255;policy.learned[919]=1;Slots bar{};bar[0]={919,0};attributes::Ranks ranks{};ranks[34]=15;
  casting::Runtime cast(1);f.box->drain();check(cast.start(500,{919,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
  publication_clock=1499;cast.update(1499,h);check(commits==0);publication_clock=1500;cast.update(1500,h);check(commits==1&&admissions==1&&admitted&&level_reads==1);
  check(f.w.resources(1)->energy==45);check((f.w.resources(2)->health<500)==(mode!=2));
  check(f.w.has_condition(f.target,combat::conditions::Kind::burning,1500)==(mode==1));
  auto packets=f.box->drain();
  if(trace){if(mode)trace<<',';trace<<"{\"mode\":"<<mode<<",\"caster\":1,\"target\":2,\"skill\":919,\"accepted\":"<<(mode!=2?"true":"false")<<",\"health\":"<<f.w.resources(2)->health<<",\"burning\":"<<(mode==1?"true":"false")<<",\"packets\":[";
   bool comma=false;constexpr char digits[]="0123456789abcdef";
   for(const auto& packet:packets){if(comma)trace<<',';comma=true;trace<<"{\"hex\":\"";for(auto b:packet)trace<<digits[b>>4]<<digits[b&15];trace<<"\"}";}
   trace<<"]}";
  }
  const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  const auto body=std::find(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,919));
  const auto damage=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,919));
  if(mode!=2){
   const bool ordered=finish!=packets.end()&&body!=packets.end()&&damage!=packets.end()&&finish<body&&body<damage;
   if(!ordered){
    std::ostringstream diagnostic;diagnostic<<"{\"failed\":true,\"direct_events\":"<<(direct_events?"true":"false")<<",\"mode\":"<<mode<<",\"finish_index\":"<<std::distance(packets.begin(),finish)<<",\"body_index\":"<<std::distance(packets.begin(),body)<<",\"damage_index\":"<<std::distance(packets.begin(),damage)<<",\"packet_count\":"<<packets.size()<<",\"packets\":[";
    bool comma=false;constexpr char digits[]="0123456789abcdef";
    for(const auto& packet:packets){if(comma)diagnostic<<',';comma=true;diagnostic<<"{\"hex\":\"";for(auto byte:packet)diagnostic<<digits[byte>>4]<<digits[byte&15];diagnostic<<"\"}";}
    diagnostic<<"]}\n";std::cerr<<diagnostic.str();
    if(!trace_path.empty()){std::ofstream failure(trace_path+".failure-mode"+std::to_string(mode)+".json",std::ios::binary);failure<<diagnostic.str();failure.flush();check(bool(failure));}
    if(trace)trace.flush();
   }
   check(ordered);
  }
  else check(body==packets.end()&&damage==packets.end()&&!copied);
  if(mode==1){const auto status=std::find(packets.begin(),packets.end(),emotes::effects_2022(2,2));check(status!=packets.end()&&body<status);}
  if(copied){auto replay=copied;copied();replay();}cast.update(1500,h);check(commits==1&&f.box->drain().empty());
  check(!consumer(f.target,*elemental(919,15),admitted,1500));
 }
 if(trace){trace<<"]}\n";trace.flush();check(bool(trace));}
 std::cout<<"PASS Spirit Burn lifetime, stationary death, admitted production consumer, publication and replay\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
