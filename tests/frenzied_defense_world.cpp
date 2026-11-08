#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include "gw_shadow_damage.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Frenzied Defense World line "+std::to_string(at.line()));
}
int main()try{
 // Every attack uses the normal World block ledger; unblocked strike damage
 // doubles, while replacing/removing/expiring the stance removes both parts.
 for(unsigned mode=0;mode<7;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;unsigned draws=0;
  w.configure_attack_block_draws([&]{++draws;return mode==0?.749:.75;});
  check(w.status_mutation(f.target,1700,123,0,100,8100,true));
  if(mode==2)check(w.status_mutation(f.target,371,124,0,200,8200,true));
  if(mode==3)check(w.status_mutation(f.target,1700,123,0,200,{},false));
  if(mode==5)check(w.status_mutation(f.target,137,124,0,100,8100,true));
  if(mode==6)check(w.protective_enchantment(f.target,f.target,*regeneration::reviewed(299,12),0,200));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),mode==4?8100:300));
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1);
  check(f.health()==unsigned(mode==0?1000:mode==1||mode==5?960:mode==6?975:980));
  check(outcomes[0].outcome==(mode==0?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));
  check(draws==unsigned(mode==0||mode==1||mode==6?1:0));
 }
 // Player spell path has the same multiplier, before Protective Spirit's cap;
 // life stealing in a combined shadow spell retains its separate channel.
 for(bool cap:{false,true}){
  warrior_test::Fixture f(340,false);auto& w=f.world;
  check(w.lightning_armor(f.target,60));check(w.status_mutation(f.target,1700,123,0,100,8100,true));
  if(cap)check(w.complete_protective_spirit(f.target,f.target,*protective_spirit(12),0,100));
  const auto plan=*lightning(232,12);check(w.lightning(f.source,f.target,plan,20,200));
  const auto base=*combat::amount({combat::Kind::lightning,plan.damage,20,232,plan.penetration},60);
  check(f.health()==std::ceil(1000-std::min(2*base,cap?100.:1000000.)));
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;const auto plan=*shadow_damage(102,12);
  check(w.status_mutation(f.target,1700,123,0,100,8100,true));
  check(w.shadow_damage(f.source,f.target,plan,20,200));check(f.health()==1000-2*plan.damage-plan.steal_above_half);
 }
 // Current stance at projectile impact controls both block and vulnerability.
 for(unsigned mode=0;mode<3;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Wand",0,0,box,0,{}, {},100,1);w.join(2,"Target",100,0,box,0,{}, {},100,2);
  w.set_vitals(1,{480,480,20,20},100);w.set_vitals(2,{480,480,20,20},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(w.ordinary_weapon(source,wand,20)&&w.combat_armor(target,60)&&w.configure_projectiles({100,32}));
  check(w.status_mutation(target,1700,123,0,100,8100,true));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([&]{return mode==0?.749:.75;});
  check(w.ordinary_attack(source,2,100));w.tick(600);check(w.resources(2)->health==480);
  if(mode==2)check(w.status_mutation(target,1700,123,0,1000,{},false));
  w.tick(1600);check(w.resources(2)->health==unsigned(mode==0?480:mode==1?440:460));
 }
 // Authored hostile NPC ordinary and spell controllers use the same player
 // protection services; no NPC source stats are inferred from the capture.
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
  d.spawn_energy=combat::npcs::Energy{mode<2?0u:30u,30};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Target",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(100),target=*w.effect_scope(1);check(w.elemental_armor(target,60,60));
  if(mode!=3)check(w.status_mutation(target,1700,123,0,1000,9000,true));
  combat::ai::FlarePolicy policy;policy.source=source;policy.level=20;policy.fire_magic=12;policy.speed=300;
  policy.acquire=2000;policy.follow=2500;policy.leash=2500;policy.flight={10000,32};policy.evidence="authored Frenzied Defense NPC fixture";
  check(w.configure_npc_flare_ai(policy,1000));
  if(mode<2){combat::ai::ordinary::Reviewed p{10,10,0,5,true,combat::ai::ordinary::Delivery::ranged,500,1500,1000,{100,32},"authored fixed fire wand"};
   check(w.configure_npc_ordinary_ai(source,p,1000));w.configure_attack_block_draws([&]{return mode==0?.749:.75;});
   w.tick(1000);w.tick(1500);w.tick(2500);check(w.resources(1)->health==unsigned(mode==0?1000:980));
  }else{
   w.tick(1000);const auto pending=w.npc_flare_ai_snapshot(source)->cast;check(pending.has_value());
   w.tick(pending->completes);w.tick(pending->completes+100);
   const auto plan=*elemental(194,12);const auto damage=*combat::amount({combat::Kind::fire,plan.damage,20,194},60);
   check(w.resources(1)->health==std::ceil(1000-damage*(mode==2?2:1)));
  }
 }
 // Full normal Runtime instant path: cost, self target, native body, status,
 // recharge and exact expiry. Generic production hooks already serve1700.
 for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;casting::Runtime runtime(1);casting::Hooks h;
  unsigned spent=0;std::vector<items::Packet> packets;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.consume_energy_at=[&](auto n,auto at){spent+=n;return w.consume_energy(1,n,f.source,at);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return w.status_mutation(f.source,skill,effect,rank,now,until,active);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[1700]=1;Slots bar{};bar[0]={1700,0};attributes::Ranks ranks{};
  if(mode==1)h.effect_snapshot={};if(mode==2)h.status_mutation={};
  if(mode==3)w.set_vitals(1,{1000,1000,4,30},100);
  const auto result=runtime.start(1000,{1700,0,2},bar,p,ranks,true,h);
  if(mode){check(result==(mode==3?casting::Reason::energy:casting::Reason::unsupported));check(w.status_snapshot(f.source,1000)->empty());continue;}
  check(result==casting::Reason::accepted&&spent==5&&w.resources(1)->energy==25);
  check(std::count(packets.begin(),packets.end(),casting::generic(48,1,1700))==1);
  check(std::count(packets.begin(),packets.end(),casting::generic(21,1,1666))==1);
  const auto entries=*w.status_snapshot(f.source,1000);check(entries.size()==1&&entries[0].skill==1700&&entries[0].expires==9000);
  check(w.attack_duration(f.source,500,1000)==500); // No Frenzy-style haste.
  check(runtime.start(1001,{1700,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);
  runtime.update(8999,h);check(w.status_snapshot(f.source,8999)->size()==1);
  runtime.update(9000,h);check(w.status_snapshot(f.source,9000)->empty());
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;auto stale=f.target;++stale.generation;
  check(!w.status_mutation(stale,1700,123,0,100,8100,true));
  check(!w.status_mutation(f.target,1700,123,21,100,8100,true));
  check(!w.status_mutation(f.target,1700,123,0,100,8101,true));
  check(w.status_mutation(f.target,1700,123,0,100,8100,true));check(w.kill(f.target,200));check(w.status_snapshot(f.target,200)->empty());
 }
 std::cout<<"PASS Frenzied Defense normal Runtime, block/hit outcomes, doubling/cap/reduction, shadow life-steal separation, projectile impact, replacement/expiry/death and invalid scope guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
