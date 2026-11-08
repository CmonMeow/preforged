#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Blurred Vision line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source;
 Fixture(unsigned unknown=0,unsigned primary_health=1000){
  std::vector<world::Instance::Npc> population;
  for(unsigned id=100;id<=106;++id){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==101?266:id==102?267:100;n.plane=id==104?1:0;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.relation=id==103?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(id==106?10:id==100?primary_health:1000),1000};d.damageable=id!=103;d.conditions_allowed=id!=103&&id!=105;
   if(id!=unknown&&id!=102){d.lightning_armor=80;d.lightning_armor_evidence="test";}n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id=1;id<=5;++id){
   const float x=id==1?0:id==2?100:id==3?266:id==4?267:110;
   w.join(id,"Player",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:id==3?3:2);
   w.set_vitals(id,{1000,1000,100,100},100);
   check(w.lightning_armor(*w.effect_scope(id),id==unknown?std::nullopt:std::optional<int>{80}));
  }
  source=*w.effect_scope(1);box->drain();
 }
 skills::effects::Scope scope(unsigned id){return *w.effect_scope(id);}
 double health(unsigned id){return id>=100?w.npc_combat_snapshot(scope(id))->health->current:double(w.resources(id)->health);}
};
int main()try{
 using namespace skills;using namespace effects;using combat::weapon::AttackOutcome;
 check(*particles::targeted_body_2022(100,1,235)==world::Packet{159,0,20,0,0,0,100,0,0,0,1,0,0,0,159,1,0,0});
 for(unsigned rank=0;rank<=20;++rank){const auto p=control_hex(235,rank);check(p&&p->duration==1000ull*interpolate(4,10,rank)&&p->radius==166&&p->miss_percent==50);}
 for(bool npc:{false,true}){
  Fixture f;const auto target=f.scope(npc?100:2);std::vector<world::Packet> out;
  check(f.w.degeneration_hex(f.source,target,*degeneration_hex(235,12),1000,20,&out));
  for(unsigned id:{2u,3u,100u,101u,105u,106u}){
   check(f.health(id)==(id==106?10:1000));const bool hex=id>=100?bool(f.w.npc_combat_snapshot(f.scope(id))->effects&0x800):!f.w.status_snapshot(f.scope(id),1000)->empty();check(hex);
   check(std::count(out.begin(),out.end(),*particles::targeted_body_2022(id,1,235))==1);
  }
  f.w.tick(10000);check(f.w.status_snapshot(f.scope(2),10000)->empty()&&!(f.w.npc_combat_snapshot(f.scope(100))->effects&0x800));
 }
 for(bool blind:{false,true})for(bool price:{false,true})for(double roll:{0.,.499999,.5,.624999,.625,.949999,.95,.962499,.9625,.9999}){
  world::Instance w;w.configure_attack_miss_draws([&]{return roll;});check(w.configure_projectiles({100,32}));
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);check(w.combat_armor(target,60));
  check(w.degeneration_hex(caster,attacker,*degeneration_hex(235,12),100,20));
  if(blind)check(w.apply_condition(caster,attacker,combat::conditions::Kind::blind,100,10000));
  if(price)check(w.apply_price_of_failure(caster,attacker,12,100));
  combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};check(w.ordinary_weapon(attacker,wand,20)&&w.ordinary_attack(attacker,2,400));w.tick(900);w.tick(2000);w.cancel_ordinary_attack(attacker);
  const double threshold=blind?(price?.9625:.95):(price?.625:.5);const bool missed=roll<threshold;
  check(w.resources(2)->health==(missed?500u:480u));check(w.resources(1)->health==500-(price&&missed?37:0));
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==1&&events[0].outcome==(missed?AttackOutcome::miss:AttackOutcome::hit));
 }
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;w.configure_attack_miss_draws([]{return 0.;});check(w.configure_projectiles({100,32}));
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
  const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);check(w.combat_armor(target,60));check(w.degeneration_hex(caster,attacker,*degeneration_hex(235,0),100,20));
  if(mode==1)check(w.remove_effects(attacker,1,*removal(301,0,0),200));
  if(mode==3){w.leave(3);w.join(3,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{500,500,100,100},100);}
  if(mode==4)check(w.degeneration_hex(caster,attacker,*degeneration_hex(235,20),200,20));
  if(mode==5)check(w.kill(caster,200));
  const std::uint64_t at=mode==2?4200:400;combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};check(w.ordinary_weapon(attacker,wand,20)&&w.ordinary_attack(attacker,2,at));w.tick(at+500);w.tick(at+1600);w.cancel_ordinary_attack(attacker);
  check(w.resources(2)->health==((mode==0||mode==4||mode==5)?500u:480u));
 }
 for(bool player_target:{false,true})for(double roll:{.499999,.5}){
  const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/blurred_vision.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);
  auto enemies=population.for_map(146);auto guardian=enemies.front();guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  world::Instance w;w.configure_npcs({guardian});w.configure_attack_miss_draws([&]{return roll;});
  w.join(2,"Owner",100,0,std::make_shared<world::Outbox>());w.set_vitals(2,{100,500,100,100},100);
  const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100);check(w.physical_armor(owner,60));
  check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);
  check(w.degeneration_hex(owner,attacker,*degeneration_hex(235,12),100,20));
  combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=10;strike.location_armor=player_target?60:*guardian.combat_definition->physical_armor;strike.base_roll=strike.miss_roll=.5;
  const auto hit=*combat::weapon::resolve(strike);
  if(player_target)w.damage_quest_player(attacker,owner,128,1000,strike,1000);
  else w.damage_quest_guardian(attacker,guard,{combat::Kind::armor_ignoring,10,strike.level,0},128,1000,strike,1000);
  const bool missed=roll<.5;const auto events=w.take_attack_outcomes(attacker);check(events.size()==1&&events[0].outcome==(missed?AttackOutcome::miss:AttackOutcome::hit));
  check(w.resources(2)->health==std::ceil(100-(player_target&&!missed?hit.damage:0)));
  check(w.npc_combat_snapshot(guard)->health->current==500-(!player_target&&!missed?hit.damage:0));
 }
 {
  Fixture f;check(f.w.hex_breaker(f.scope(2),*hex_breaker(12),20,500));
  check(f.w.degeneration_hex(f.source,f.scope(2),*degeneration_hex(235,12),1000,20));
  check(f.w.status_snapshot(f.scope(2),1000)->empty()&&!f.w.status_snapshot(f.scope(3),1000)->empty());
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  Fixture f;const auto selected=f.scope(npc?100:2);casting::Hooks h;unsigned commits=0;bool stale=false;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& p){f.w.skill_event(1,p);};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==selected.actor)return std::optional<Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==selected.actor&&range==1248;};
  if(mode!=4)h.degeneration_hex=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{*particles::targeted_body_2022(100,1,235)}};
   std::vector<world::Packet> out;const bool ok=f.w.degeneration_hex(f.source,target,plan,at,20,&out);return {ok,std::move(out)};
  };
  Slots bar{};bar[0]={235,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[235]=1;attributes::Ranks ranks{};ranks[11]=12;
  casting::Runtime cast(1);check(cast.start(1000,{235,0,selected.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==(mode==4?100u:90u));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(commits==unsigned(mode==0||mode==5));
  const auto packets=f.box->drain();const auto particle=*particles::targeted_body_2022(100,1,235);check(std::count(packets.begin(),packets.end(),particle)==unsigned(mode==0));
  if(mode==0)check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),particle));
 }
 std::cout<<"PASS Blurred Vision mixed adjacent hexes,real projectile miss thresholds/stacking,removal/expiry/lifetimes and twelve actual casts\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
