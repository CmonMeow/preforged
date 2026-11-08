#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Barrage line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{395u,2144u})for(unsigned rank=0;rank<=20;++rank){const auto plan=combat::weapon::attack_skill(skill,rank);check(plan&&plan->bonus==effects::interpolate(skill==395?5:1,skill==395?20:10,rank)&&plan->required_type==5&&plan->arrows==1);}
 for(unsigned skill:{395u,2144u})for(unsigned preparation:{0u,431u,432u})for(unsigned mode=0;mode<4;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=930;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.physical_armor=60;d.physical_armor_evidence="test";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id=1;id<=10;++id){const float x=id==1?0:id==9?1200:id==10?950:900+(id-2)*20;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==10?1:2);
   w.set_vitals(id,{1000,1000,30,30},100);if(id!=1&&(mode!=2||id!=4))check(w.physical_armor(*w.effect_scope(id),60));}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  combat::weapon::Owned bow{12,1,{20,20,1},12,false,false,5};check(w.ordinary_weapon(source,bow,20));
  check(w.status_mutation(source,200,3,0,100,15100,true));
  if(preparation)check(w.status_mutation(source,preparation,1,12,100,24100,true));
  if(mode==1){w.configure_attack_miss_draws([]{return .5;});check(w.apply_condition(target,source,combat::conditions::Kind::blind,100,5000));}
  if(mode==3){w.configure_attack_block_draws([]{return 0.;});check(w.status_mutation(*w.effect_scope(3),448,2,12,100,100+*effects::escape(448,12),true));}
  check(w.prepare_diversion_skill_use(source,{skill,0},1,1000));std::vector<world::Packet> launch_packets;
  check(w.completed_bow_attack(source,target,bow,*combat::weapon::attack_skill(skill,12),1,1000,skill==395?1000:2000,0,&launch_packets).has_value()==(mode!=2));
  if(mode==2)check(w.abort_diversion_skill_use(source,1)&&launch_packets.empty());
  else{check(launch_packets.size()==(skill==395?6u:4u));std::set<std::uint32_t> tokens;
   for(const auto& packet:launch_packets){check(packet.size()==29&&packet[0]==163&&packet[1]==128&&packet[20]==143&&packet.back()==1);
    const auto key=std::uint32_t(packet[24])|(std::uint32_t(packet[25])<<8)|(std::uint32_t(packet[26])<<16)|(std::uint32_t(packet[27])<<24);check(key&&tokens.insert(key).second);}
   w.skill_event(1,launch_packets,1000);
  }
  const auto statuses=*w.status_snapshot(source,1000);
  check(std::count_if(statuses.begin(),statuses.end(),[](const auto& effect){return effect.kind==effects::Category::preparation;})==unsigned(mode==2&&preparation));
  check(std::count_if(statuses.begin(),statuses.end(),[](const auto& effect){return effect.kind==effects::Category::glyph;})==unsigned(skill==395||mode==2));
  for(unsigned id=2;id<=10;++id)check(w.resources(id)->health==1000); // No instant area damage.
  w.tick(1500);const auto outcomes=w.take_attack_outcomes(source);check(outcomes.size()==(mode==2?0u:skill==395?6u:4u));
  std::set<unsigned> actors;std::set<std::uint64_t> events;
  for(const auto& result:outcomes){actors.insert(result.target.actor);events.insert(result.event);
   check(result.outcome==(mode==1?combat::weapon::AttackOutcome::miss:mode==3&&result.target.actor==3?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));}
  if(mode!=2)check(actors==(skill==395?std::set<unsigned>{2,3,4,5,6,100}:std::set<unsigned>{2,3,4,100})&&events.size()==(skill==395?6u:4u));
  for(unsigned id=2;id<=10;++id){const bool hit=mode!=1&&mode!=2&&id<=(skill==395?6u:4u)&&!(mode==3&&id==3);check(w.resources(id)->health==(hit?(skill==395?963u:972u):1000u));}
  check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==(mode==1||mode==2?1000.:skill==395?963.:972.));
  w.tick(1600);check(w.take_attack_outcomes(source).empty());
 }
 for(unsigned skill:{395u,2144u})for(unsigned mode=0;mode<3;++mode){warrior_test::Fixture f(skill,false);auto& w=f.world;Slots bar{};bar[0]={skill,0};check(w.configure_adrenaline(f.source,bar,100));
  casting::Runtime runtime(1);casting::Hooks h;std::vector<world::Packet> wire;unsigned releases=0;std::uint64_t event_time=1000;
  h.events=[&](const auto& batch){wire.insert(wire.end(),batch.begin(),batch.end());w.skill_event(1,batch,event_time);};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](unsigned amount,auto at){check(amount==5);return w.consume_energy(1,amount,f.source,at);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.weapon_snapshot=[&](){return std::optional{f.weapon};};h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,skill);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){return w.melee_attack(f.source,target,weapon,plan,at);};
  h.completed_bow_attack=[&](auto target,const auto& weapon,const auto& plan,auto event,auto at,auto base,auto existing,auto& visuals){++releases;return w.completed_bow_attack(f.source,target,weapon,plan,event,at,base,existing,&visuals);};
  h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};h.cast_slot_activity=[&](auto slot,auto begins,auto ends){check(w.cast_activity(f.source,slot,begins,ends));};
  h.control_epoch=[&](){return w.control_epoch(f.source);};h.control_event_time=[&](auto after){return w.control_event_time(f.source,after);};h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(f.source);};
  h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};h.current_skillbar=[&]()->std::optional<Slots>{return bar;};h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(f.source,slot,at);};
  h.prepare_skill_use=[&](auto slot,auto event,auto at){return w.prepare_diversion_skill_use(f.source,slot,event,at);};
  h.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool instant){return w.finalize_diversion_skill_use(f.source,event,at,base,existing,instant);};
  h.abort_skill_use=[&](auto event){return w.abort_diversion_skill_use(f.source,event);};h.cooldown_completion_source=[&](auto event){return w.cooldown_completion_stamp(f.source,event);};
  Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[skill]=1;attributes::Ranks ranks{};ranks[25]=12;
  if(mode==2)h.completed_bow_attack={};
  check(runtime.start(1000,{skill,0,f.target.actor},bar,policy,ranks,true,h)==(mode==2?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==2){check(w.resources(1)->energy==30);continue;}
  check(w.resources(1)->energy==25);if(mode==1)runtime.cancel(h);
  event_time=1500;runtime.update(1500,h);check(releases==unsigned(mode==0));w.tick(1600);check(f.health()==(mode==0?(skill==395?963:972):1000));
  if(mode==0){
   const auto launch=std::find_if(wire.begin(),wire.end(),[](const auto& packet){return packet.size()==29&&packet[0]==163&&packet[1]==128;});
   check(launch!=wire.end()&&std::find(wire.begin(),wire.end(),combat::weapon::finish_2022(1,true))<launch);
   check(std::find(wire.begin(),wire.end(),casting::terminal(46,1))==wire.end());
   const auto snapshot=*runtime.cooldown_source_snapshot(f.source,1600,h);check(snapshot.timers.size()==1&&snapshot.timers[0].until==(skill==395?2500u:3500u)&&snapshot.timers[0].origin);}
 }
 // Deferred publication: old tick and forged/mistimed packets cannot open
 // the flight; a late valid publication restarts the complete wire duration.
 {warrior_test::Fixture f(395,false);auto& w=f.world;Slots bar{};bar[0]={395,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.prepare_diversion_skill_use(f.source,{395,0},1,1000));std::vector<world::Packet> visuals;
  check(w.completed_bow_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(395,12),1,1000,1000,0,&visuals).has_value());
  check(visuals.size()==1&&visuals[0].size()==29&&visuals[0][0]==163&&visuals[0][1]==128&&visuals[0].back()==1);
  w.tick(2000);check(f.health()==1000&&w.take_attack_outcomes(f.source).empty());
  f.box->drain();auto forged=visuals;forged[0][24]^=2;w.skill_event(1,forged,2000);check(f.box->drain().empty());w.tick(2100);check(f.health()==1000);
  f.box->drain();w.skill_event(1,visuals,999);check(f.box->drain().empty());w.tick(2200);check(f.health()==1000);
  f.box->drain();bool sampled=false;w.skill_event_live(1,visuals,[&]{const auto queued=f.box->drain();check(std::find(queued.begin(),queued.end(),visuals[0])!=queued.end());sampled=true;return 4000;});
  check(sampled);w.tick(4000);check(f.health()==1000);w.tick(4100);check(f.health()==963);
  check(w.take_attack_outcomes(f.source).size()==1);f.box->drain();w.skill_event(1,visuals,5000);check(f.box->drain().empty());w.tick(5100);check(f.health()==963&&w.take_attack_outcomes(f.source).empty());
 }
 // A retired recipient must not leave a ghost launch in a later completion batch.
 {warrior_test::Fixture f(395,false);auto& w=f.world;Slots bar{};bar[0]={395,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.prepare_diversion_skill_use(f.source,{395,0},1,1000));std::vector<world::Packet> visuals;
  check(w.completed_bow_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(395,12),1,1000,1000,0,&visuals).has_value());
  w.leave(f.target.actor);w.tick(2000);f.box->drain();visuals.push_back(casting::terminal(46,1));
  w.skill_event_live(1,visuals,[]{return 3000;});const auto queued=f.box->drain();
  check(queued==std::vector<world::Packet>{casting::terminal(46,1)});
 }
 {projectiles::SynchronizationTokens tokens(UINT32_MAX);const std::vector<std::uint32_t> live{1,2,UINT32_MAX};
  const auto ids=tokens.reserve(3,live);check(ids==std::vector<std::uint32_t>{3,4,5});
  check(!tokens.reserve(0,{})&&!tokens.reserve(4097,{}));check(tokens.reserve(1,{})==std::vector<std::uint32_t>{6});
 }
 std::cout<<"PASS Barrage mixed six-recipient bow release,preparation removal,independent hit/miss/block,atomic unknown armor and Runtime payment/cancellation/source\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
