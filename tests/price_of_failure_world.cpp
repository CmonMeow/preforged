#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Price of Failure world regression line "+std::to_string(at.line()));}
void setup(world::Instance& w){
 w.configure_attack_miss_draws([]{return .5;});
 for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id==1?0:100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}
 check(w.combat_armor(*w.effect_scope(2),60)&&w.configure_projectiles({100,32}));
}
int main()try{
 for(unsigned rank:{0u,12u,20u})for(unsigned mode:{0u,1u,2u,3u,4u,5u}){
  world::Instance w;setup(w);const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);
  check(w.apply_price_of_failure(caster,attacker,rank,100));
  if(mode==0||mode==5)check(w.apply_condition(target,attacker,combat::conditions::Kind::blind,100,5000));
  if(mode==2){const auto entry=w.status_snapshot(attacker,100)->front();check(w.status_mutation(attacker,103,entry.instance,rank,200,{},false));}
  if(mode==3){w.leave(3);w.join(3,"Replacement caster",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{500,500,100,100},100);}
  if(mode==2||mode==3||mode==4)check(w.apply_condition(target,attacker,combat::conditions::Kind::blind,300,5000));
  if(mode==4)check(w.apply_price_of_failure(caster,attacker,0,300));
  if(mode==5)check(w.kill(caster,300));
  combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};check(w.ordinary_weapon(attacker,wand,20)&&w.ordinary_attack(attacker,2,400));
  w.tick(900);w.tick(2000);w.cancel_ordinary_attack(attacker);
  const unsigned damage=((mode==0||mode==5)?1+3*rank:mode==4?1:0);
  check(w.resources(1)->health==500-damage&&w.resources(2)->health==(mode==1?480u:500u));
  const auto outcomes=w.take_attack_outcomes(attacker);check(outcomes.size()==1);
  w.tick(2100);check(w.resources(1)->health==500-damage&&w.take_attack_outcomes(attacker).empty());
 }
 {
  world::Instance w;setup(w);const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);
  check(w.apply_price_of_failure(caster,attacker,12,100));w.set_vitals(1,{10,500,100,100},100);
  check(w.apply_condition(target,attacker,combat::conditions::Kind::blind,100,5000));combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(w.ordinary_weapon(attacker,wand,20)&&w.ordinary_attack(attacker,2,100));w.tick(600);const auto before=*w.control_epoch(attacker);w.tick(1600);check(w.resources(1)->health==0&&w.control_event_time(attacker,before)==1600);
  const auto outcome=w.take_attack_outcomes(attacker);check(outcome.size()==1&&outcome[0].failed_hit());w.tick(1700);check(w.resources(1)->health==0);
 }
 {
  world::Instance w;setup(w);const auto attacker=*w.effect_scope(1),caster=*w.effect_scope(3);
  check(w.apply_price_of_failure(caster,attacker,12,100));combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(w.ordinary_weapon(attacker,wand,20)&&w.ordinary_attack(attacker,2,100));w.tick(600);w.move(2,500,0,700);w.tick(1600);w.cancel_ordinary_attack(attacker);
  check(w.resources(1)->health==463&&w.resources(2)->health==500);
 }
 {
  world::Instance w;setup(w);const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);
  check(w.apply_price_of_failure(caster,attacker,12,100)&&w.status_mutation(attacker,346,900,0,100,8100,true));
  check(w.apply_condition(target,attacker,combat::conditions::Kind::blind,100,5000));combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(w.ordinary_weapon(attacker,wand,20)&&w.ordinary_attack(attacker,2,100));w.tick(600);w.tick(1600);w.cancel_ordinary_attack(attacker);
  check(w.resources(1)->health==426&&w.resources(2)->health==500); //37 reactive damage doubled by Frenzy.
 }
 {
  world::Instance w;setup(w);const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);
  check(w.apply_price_of_failure(caster,attacker,12,100));w.tick(30100);
  check(w.apply_condition(target,attacker,combat::conditions::Kind::blind,30200,5000));combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(w.ordinary_weapon(attacker,wand,20)&&w.ordinary_attack(attacker,2,30200));w.tick(30700);w.tick(31800);w.cancel_ordinary_attack(attacker);
  check(w.resources(1)->health==500&&w.resources(2)->health==500);
 }
 {
  world::Instance w;const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");
  auto population=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/price_of_failure_world.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);w.configure_npcs(population.for_map(146));setup(w);
  const auto target=*w.effect_scope(100),caster=*w.effect_scope(3);
  check(w.apply_price_of_failure(caster,target,12,100)&&w.apply_price_of_failure(caster,target,0,1100));
  w.tick(30100);check((w.npc_combat_snapshot(target)->effects&0x800)&&w.npc_combat_snapshot(target)->health->current==500);
  w.tick(31100);check(!(w.npc_combat_snapshot(target)->effects&0x800)&&w.npc_combat_snapshot(target)->health->current==500);
 }
 {
  // Actual world resolver thresholds,including the combined Blind chance.
  for(bool blind:{false,true})for(bool below:{false,true}){
   world::Instance w;setup(w);const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);
   const double threshold=blind?.925:.25;w.configure_attack_miss_draws([=]{return threshold-(below?.000001:0);});
   check(w.apply_price_of_failure(caster,attacker,12,100));
   if(blind)check(w.apply_condition(target,attacker,combat::conditions::Kind::blind,100,5000));
   combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};check(w.ordinary_weapon(attacker,wand,20)&&w.ordinary_attack(attacker,2,100));
   w.tick(600);w.tick(1600);w.cancel_ordinary_attack(attacker);
   check(w.resources(1)->health==(below?463u:500u)&&w.resources(2)->health==(below?500u:480u));
  }
 }
 {
  // Replay a seeded server stream through real projectile impacts,not a
  // helper-only probability test. No Blind is present:the25percent hex alone
  // must produce both hits and misses,and repeated seed reproduces outcomes.
  std::vector<bool> first;
  for(unsigned run=0;run<2;++run){
   world::Instance w;setup(w);w.configure_attack_miss_draws({});
   const auto attacker=*w.effect_scope(1),target=*w.effect_scope(2),caster=*w.effect_scope(3);
   w.set_vitals(2,{100000,100000,100,100},100);combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};check(w.ordinary_weapon(attacker,wand,20));
   unsigned misses=0;
   for(unsigned i=0;i<200;++i){const std::uint64_t start=100+2200*i;
    check(w.apply_price_of_failure(caster,attacker,0,start)&&w.ordinary_attack(attacker,2,start));w.tick(start+500);w.tick(start+1600);w.cancel_ordinary_attack(attacker);
    const auto events=w.take_attack_outcomes(attacker);check(events.size()==1);
    check(events[0].outcome==combat::weapon::AttackOutcome::hit||events[0].outcome==combat::weapon::AttackOutcome::miss);
    const bool missed=events[0].outcome==combat::weapon::AttackOutcome::miss;misses+=missed;
    if(!run)first.push_back(missed);else check(first[i]==missed);
   }
   check(misses>=30&&misses<=70&&w.resources(1)->health==500-misses);
   check(w.resources(2)->health==100000-20*(200-misses));
  }
 }
 {
  for(double invalid:std::array<double,2>{NAN,1.}){
   world::Instance w;setup(w);const auto attacker=*w.effect_scope(1),caster=*w.effect_scope(3);
   w.configure_attack_miss_draws([=]{return invalid;});check(w.apply_price_of_failure(caster,attacker,12,100));
   combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};check(w.ordinary_weapon(attacker,wand,20)&&w.ordinary_attack(attacker,2,100));
   w.tick(600);w.tick(1600);w.cancel_ordinary_attack(attacker);const auto events=w.take_attack_outcomes(attacker);
   check(events.size()==1&&events[0].outcome==combat::weapon::AttackOutcome::cancelled&&w.resources(1)->health==500&&w.resources(2)->health==500);
  }
 }
 {
  world::Instance w;setup(w);const auto attacker=*w.effect_scope(1),caster=*w.effect_scope(3),target=*w.effect_scope(2);
  check(!w.apply_price_of_failure(attacker,attacker,12,100)&&!w.apply_price_of_failure(target,caster,12,100));
  auto stale=attacker;++stale.generation;check(!w.apply_price_of_failure(caster,stale,12,100));
  check(!w.apply_price_of_failure(caster,attacker,21,100));
 }
 for(unsigned rank:{0u,12u,20u})for(bool npc_target:{false,true})for(unsigned mode=0;mode<5;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=true;d.spawn_health=combat::Health{500,500};d.earth_armor=60;n.combat_definition=d;w.configure_npcs({n});setup(w);
  const auto caster=*w.effect_scope(3),recipient=*w.effect_scope(npc_target?100:1);unsigned commits=0;bool replaced=false;
  skills::Slots bar{};bar[0]={103,0};skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[103]=1;attributes::Ranks ranks{};ranks[7]=rank;
  skills::casting::Hooks h;h.events=[](const auto&){};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(3,cost,caster,at);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==recipient.actor&&range==1248;};
  h.target_scope=[&](unsigned id){auto scope=w.effect_scope(id);if(replaced&&scope)++scope->generation;return scope;};
  if(mode!=4)h.degeneration_hex=[&](auto target,const auto& plan,auto at){++commits;return w.degeneration_hex(caster,target,plan,at,20);};
  skills::casting::Runtime cast(3);check(cast.start(1000,{103,0,recipient.actor},bar,policy,ranks,true,h)==(mode==4?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
  check(w.resources(3)->energy==(mode==4?100u:85u));
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1500,h);if(mode==3)replaced=true;
  cast.update(2999,h);check(!commits);cast.update(3000,h);cast.update(3000,h);check(commits==unsigned(mode==0));
  if(npc_target)check(bool(w.npc_combat_snapshot(recipient)->effects&0x800)==(mode==0));
  else {
   const auto status=w.status_snapshot(recipient,3000);check(status.has_value());
   check(std::any_of(status->begin(),status->end(),[](const auto& entry){return entry.skill==103;})==(mode==0));
   if(mode==0){
    w.configure_attack_miss_draws([]{return 0.;});combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
    check(w.ordinary_weapon(recipient,wand,20)&&w.ordinary_attack(recipient,2,3500));w.tick(4000);w.tick(5100);w.cancel_ordinary_attack(recipient);
    check(w.resources(1)->health==500-(1+3*rank)&&w.resources(2)->health==500);
   }
  }
 }
 std::cout<<"PASS staged Price of Failure projectile miss/dodge damage,rank,renewal,removal,lethal Health and caster lifetime\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
