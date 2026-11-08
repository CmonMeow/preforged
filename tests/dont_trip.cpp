#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;namespace casting=skills::casting;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Don't Trip line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope owner,ally,other,foe,npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(float distance=60,bool member=true){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=distance;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;d.conditions_allowed=true;
  d.knockdown_allowed=true;d.knockdown_evidence="fixture";d.healing_allowed=true;d.healing_evidence="fixture";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Actor",id==2?distance:0,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==4?2:1);w.set_vitals(id,{500,1000,100,100},100);}
  owner=*w.effect_scope(1);ally=*w.effect_scope(2);other=*w.effect_scope(3);foe=*w.effect_scope(4);npc=*w.effect_scope(100);
  if(member)check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  check(w.set_npc_party_owner(npc,ally));
 }
 bool active(effects::Scope scope,unsigned now){const auto entries=w.status_snapshot(scope,now);if(!entries)return false;return std::any_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==2216;});}
};
int main()try{
 unsigned cases=0;
 for(unsigned rank=0;rank<=10;++rank)for(float distance:{0.f,1000.f,1000.01f})for(bool member:{false,true}){
  ++cases;Fixture f(distance,member);const auto plan=*effects::dont_trip(rank);check(plan.duration==1000ull*std::min(5u,3u+rank/2));
  std::function<void()> publication;f.box->drain();check(f.w.stage_dont_trip_party(f.owner,plan,100,&publication));
  check(f.box->drain().empty()&&bool(publication));publication();check(!f.box->drain().empty());publication();check(f.box->drain().empty());
  const bool included=member&&distance<=1000;
  check(f.active(f.owner,100)&&f.active(f.ally,100)==included&&f.active(f.npc,100)==included&&!f.active(f.other,100)&&!f.active(f.foe,100));
  const auto source_epoch=f.w.control_epoch(f.owner),ally_epoch=f.w.control_epoch(f.ally);
  check(f.w.gale(f.foe,f.owner,12,{},101)&&f.w.control_epoch(f.owner)==source_epoch);
  check(f.w.gale(f.foe,f.ally,12,{},101));check((f.w.control_epoch(f.ally)==ally_epoch)==included);
  const auto before=f.w.control_epoch(f.owner);f.w.tick(100+plan.duration);
  check(!f.active(f.owner,100+plan.duration)&&!f.active(f.npc,100+plan.duration));
  check(f.w.gale(f.foe,f.owner,12,{},100+plan.duration)&&f.w.control_epoch(f.owner)!=before);
 }
 // Exact NPC action consumer: temporary immunity wins over unknown intrinsic
 // metadata,and must preserve the pending action and its control epoch.
 for(unsigned rank=0;rank<=10;++rank)for(int intrinsic:{-1,0,1}){
  ++cases;combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;
  d.spawn_health={500,1000};d.damageable=true;if(intrinsic>=0){d.knockdown_allowed=bool(intrinsic);d.knockdown_evidence="fixture";}
  combat::npcs::Lifetime life{1,1,1,146,100};combat::npcs::LiveActor actor(d,life);const auto plan=*effects::dont_trip(rank);
  const auto token=actor.begin_action(life,combat::npcs::control::Action::attack,100,10000);check(token.has_value());const auto epoch=actor.control_epoch(life);
  const auto effect=actor.apply_party_defense_shout(life,effects::Scope{1,1,1},plan,7,100,true,1000);check(effect&&effect->accepted);
  using K=combat::npcs::control::Knockdown;check(actor.knock_down(life,101,0)==K::invalid);
  check(actor.knock_down(life,101)==K::immune&&actor.control_epoch(life)==epoch&&actor.active_action(life,101)==token);
  check(actor.knock_down(life,100+plan.duration)==(intrinsic<0?K::unknown:intrinsic?K::accepted:K::immune));
 }
 // Independent shout types coexist;renewing one doesn't erase the other.
 {
  ++cases;Fixture f;check(f.w.stage_shields_up_party(f.owner,*effects::shields_up(0),100));check(f.w.stage_dont_trip_party(f.owner,*effects::dont_trip(0),100));
  check(f.w.status_snapshot(f.owner,100)->size()==2&&f.w.status_snapshot(f.npc,100)->size()==2);
  check(f.w.stage_dont_trip_party(f.owner,*effects::dont_trip(4),200));check(f.w.status_snapshot(f.owner,200)->size()==2);
  f.w.tick(3100);check(f.active(f.owner,3100)&&f.active(f.npc,3100));f.w.tick(5100);check(f.active(f.owner,5100)&&f.active(f.npc,5100));
  check(f.w.shields_up_player_armor(f.owner,5100,effects::ShieldsUpDamage::projectile_attack)==0);f.w.tick(5200);check(!f.active(f.owner,5200)&&!f.active(f.npc,5200));
 }
 // A dead party owner doesn't remove a living NPC companion's membership.
 {
  ++cases;Fixture f;check(f.w.kill(f.ally,100));check(f.w.stage_dont_trip_party(f.owner,*effects::dont_trip(0),100));check(f.active(f.npc,100)&&!f.active(f.ally,100));
 }
 for(unsigned skill:{367u,2216u}){
  ++cases;Fixture f;check(f.w.gale(f.foe,f.owner,12,{},100));const auto epoch=f.w.control_epoch(f.owner);
  check(skill==367?f.w.stage_shields_up_party(f.owner,*effects::shields_up(0),101):f.w.stage_dont_trip_party(f.owner,*effects::dont_trip(0),101));
  check(f.w.control_epoch(f.owner)==epoch);check(f.w.gale(f.foe,f.owner,12,{},2200));check((f.w.control_epoch(f.owner)==epoch)==(skill==2216));
 }
 {
  ++cases;Fixture f;const auto old=f.owner;f.w.leave(1);f.w.join(1,"Recycled",0,0,f.box,0,{}, {},100,1);f.w.set_vitals(1,{500,1000,100,100},100);
  check(!f.w.stage_dont_trip_party(old,*effects::dont_trip(0),100));
  auto forged=*effects::dont_trip(0);++forged.duration;check(!f.w.stage_dont_trip_party(*f.w.effect_scope(1),forged,100));
 }
 for(unsigned rank:{0u,4u,10u})for(unsigned mode=0;mode<8;++mode){
  ++cases;Fixture f;casting::Runtime runtime(1);casting::Hooks h;unsigned payments=0,commits=0;bool stale=false;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.consume_energy_at=[&](auto cost,auto at){++payments;check(cost==5);const bool paid=f.w.consume_energy(1,cost,f.owner,at);if(mode==3&&paid)stale=true;return paid;};
  h.target_scope=[&](auto id){auto scope=f.w.effect_scope(id);if(scope&&stale)++scope->generation;return scope;};
  h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.deldrimor_rank=[&](){return std::optional(rank);};
  if(mode!=1)h.dont_trip=[&](auto source,const auto& plan,auto at){++commits;std::function<void()> publish;const bool ok=mode!=4&&f.w.stage_dont_trip_party(source,plan,at,&publish);return casting::EffectCompletion{ok,{}, {},std::move(publish)};};
  if(mode==2)f.w.set_vitals(1,{500,1000,4,100},100);if(mode==5)h.deldrimor_rank=[](){return std::optional(11u);};if(mode==7)check(f.w.gale(f.foe,f.owner,12,{},100));
  skills::Slots bar{};bar[0]={2216,0};skills::Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[2216]=1;policy.pvp_match=mode==6;attributes::Ranks ranks{};
  f.box->drain();const auto reason=runtime.start(200,{2216,0,4},bar,policy,ranks,true,h);
  if(mode==1||mode==5){check(reason==casting::Reason::unsupported&&payments==0&&commits==0);continue;}
  if(mode==2){check(reason==casting::Reason::energy&&payments==1&&commits==0);continue;}
  if(mode==6){check(reason==casting::Reason::pve_only&&payments==0&&commits==0);continue;}
  if(reason!=casting::Reason::accepted||runtime.active()||payments!=1||f.w.resources(1)->energy!=95)
   throw std::runtime_error("paid rank="+std::to_string(rank)+" mode="+std::to_string(mode)+" reason="+std::to_string(unsigned(reason))+" payments="+std::to_string(payments));
  if(mode==3||mode==4){check(commits==mode-3);continue;}
  check(commits==1&&f.active(f.owner,200));const auto packets=f.box->drain();
  const auto activation=std::find(packets.begin(),packets.end(),casting::generic(48,1,2216));
  const auto body=*casting::skill_success_body_effect(1,2216);const auto visual=std::find(packets.begin(),packets.end(),body);
  const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==65&&p[1]==0;});
  check(activation!=packets.end()&&visual!=packets.end()&&status!=packets.end()&&activation<visual&&visual<status);
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==0&&std::count(packets.begin(),packets.end(),casting::slot_event(228,1,{2216,0},20))==1);
  check(runtime.start(201,{2216,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
 }
 // An instant shout must not cancel or replace a spell already being cast.
 {
  ++cases;Fixture f;casting::Runtime runtime(1);casting::Hooks h;unsigned healed=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.owner,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.heal=[&](auto,unsigned,float,bool){++healed;return true;};h.deldrimor_rank=[](){return std::optional(4u);};
  h.dont_trip=[&](auto source,const auto& plan,auto at){std::function<void()> publish;const bool ok=f.w.stage_dont_trip_party(source,plan,at,&publish);return casting::EffectCompletion{ok,{}, {},std::move(publish)};};
  skills::Slots bar{};bar[0]={2216,0};bar[1]={281,0};skills::Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[2216]=policy.learned[281]=1;attributes::Ranks ranks{};
  check(runtime.start(100,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted&&runtime.active());
  check(runtime.start(200,{2216,0,4},bar,policy,ranks,true,h)==casting::Reason::accepted&&runtime.active()&&f.active(f.owner,200));
  check(healed==0);runtime.update(100+unsigned(skills::find_skill(281)->activation*1000),h);check(healed==1&&!runtime.active());
 }
 check(!effects::dont_trip(11));check(cases==129);std::cout<<"PASS "<<cases<<" Don't Trip party/range/player-NPC immunity/expiry/coexistence/renewal/knocked-down/dead-owner/paid-Runtime cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
