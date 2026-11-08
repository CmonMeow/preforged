#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <array>
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;using combat::weapon::AttackInterruptDisables;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Self shot line "+std::to_string(at.line())+context);}
// Bow skills use the existing2500-unit/s policy. Source-190,target60
// gives an exact100ms flight; the outer tick time is not the impact clock.
struct ShotFixture:warrior_test::Fixture {
 ShotFixture(unsigned id,bool npc,unsigned health=1000):warrior_test::Fixture(id,npc,health){
  world.leave(1);world.join(1,"Bow caster",-190,0,box,0,{}, {},100,1);
  world.set_vitals(1,{1000,1000,30,30},100);source=*world.effect_scope(1);
  check(world.energy_capacity(1,1,0,100,{10,-2}));check(world.ordinary_weapon(source,weapon,20));
  check(world.resources(1)->energy==30&&world.resources(1)->max_energy==30);
  const auto flight=projectiles::intercept({2500,32},-190,0,60,0,0,0);check(flight&&flight->flight_ms==100);
 }
};
Slots bar_for(unsigned id){Slots b{};b[0]={static_cast<std::uint16_t>(id),0};b[1]={403,0};b[2]={403,1};b[3]={451,0};b[4]={static_cast<std::uint16_t>(id),1};return b;}
int main()try{
 constexpr std::array<unsigned,21> marauder{10,12,13,15,17,18,20,22,23,25,27,28,30,32,33,35,37,38,40,42,43};
 constexpr std::array<unsigned,21> disable{5,5,5,5,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,2,2};
 for(unsigned r=0;r<=20;++r){const auto m=combat::weapon::bow_self_disable(908,r),f=combat::weapon::bow_self_disable(909,r,1);check(m&&f&&m->bonus==marauder[r]&&m->duration==10000&&f->bonus==10+r&&f->duration==disable[r]*1000);
  auto p=*combat::weapon::attack_skill(909,r,0,1);check(p.copy==1&&combat::weapon::valid(p));++p.bonus;check(!combat::weapon::valid(p));}
 check(!combat::weapon::bow_self_disable(909,21)&&!combat::weapon::bow_self_disable(909,0,65536));
 {const Scope owner{1,1,1},foreign{1,1,2};CooldownBarAuthority authority(owner);const auto bar=*authority.publish(owner,bar_for(909),100);AttackInterruptDisables ledger(owner);
  const auto plan=*combat::weapon::bow_self_disable(909,12,1);const auto slots=combat::weapon::bow_self_disabled_slots(plan,bar.bar);check(slots&&*slots==std::vector<Slot>{{909,0},{403,0},{403,1}});
  auto stamp=*authority.order(owner,200);auto next=ledger.propose_self_hit(owner,plan,bar,stamp,9,200);check(next&&*next->disabled_until(owner,{909,0},200)==3200&&*next->disabled_until(owner,{909,1},200)==0);
  check(!ledger.propose_self_hit(foreign,plan,bar,stamp,9,200));check(!next->propose_self_hit(owner,plan,bar,*authority.order(owner,200),9,200));
  auto earlier_release=next->propose_self_hit(owner,plan,bar,*authority.order(owner,200),3,200);check(earlier_release.has_value());
  const auto reset=earlier_release->propose_reset_slots(owner,{{403,1}},200);check(reset&&*reset->disabled_until(owner,{403,1},200)==0&&*reset->disabled_until(owner,{403,0},200)==3200);
  check(!reset->propose_self_hit(owner,plan,bar,*authority.order(owner,200),3,200));
  auto future=bar;future.published.at=300;check(!ledger.propose_self_hit(owner,plan,future,stamp,8,200));
  const auto interrupt=ledger.propose(owner,*combat::weapon::interrupt_disable_plan(340),{owner,{403,0},1,100,400,200,200},200);check(interrupt.has_value());
  const auto combined=interrupt->propose_self_hit(owner,plan,bar,*authority.order(owner,200),7,200);check(combined&&*combined->disabled_until(owner,{403,0},200)==20200);
 }
 for(unsigned id:{908u,909u})for(bool npc:{false,true})for(unsigned mode=0;mode<8;++mode){
  context=" World id="+std::to_string(id)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  ShotFixture f(id,npc,mode==3?1:1000);auto& w=f.world;auto bar=bar_for(id);if(mode!=4)check(w.configure_adrenaline(f.source,bar,mode==7?2000:100));
  if(mode==1)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,3000));
  if(mode==2){w.configure_attack_block_draws([]{return 0.;});if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));}
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(id,12,0),1000));
  if(mode==5){bar[3]={452,1};check(w.configure_adrenaline(f.source,bar,1050));} // current impact bar, exactcopy
  if(mode==6)check(w.kill(f.source,1050));w.tick(1100);
  if(mode==7){
   check(f.health()==1000&&w.take_attack_outcomes(f.source).empty());
   const Slot selected=id==908?bar[3]:bar[1];check(w.skill_disabled_slot(f.source,selected,1100)==0);
   // World ticks have100ms cadence. Process1900 so the1999 probe
   // cannot push the next real tick past the intended2000 frontier.
   w.tick(1900);w.tick(1999);check(f.health()==1000&&w.take_attack_outcomes(f.source).empty());
   w.tick(2000);if(f.health()!=1000-(id==908?50:42)){
    const auto diagnostic=w.take_attack_outcomes(f.source);
    throw std::runtime_error("Deferred health"+context+" health="+std::to_string(f.health())+" outcomes="+std::to_string(diagnostic.size())+" outcome="+(diagnostic.empty()?"none":std::to_string(unsigned(diagnostic[0].outcome)))+" at="+(diagnostic.empty()?"none":std::to_string(diagnostic[0].at))+" failures="+std::to_string(w.cooldown_order_failures(f.source).value_or(999)));
   }
   check(w.skill_disabled_slot(f.source,selected,2000)==2000+(id==908?10000u:3000u));
   const auto outcome=w.take_attack_outcomes(f.source);check(outcome.size()==1&&outcome[0].at==2000&&outcome[0].outcome==combat::weapon::AttackOutcome::hit);
   w.tick(2000);w.tick(2100);check(w.take_attack_outcomes(f.source).empty()&&f.health()==1000-(id==908?50:42));continue;
  }
  const bool hit=mode==0||mode==3||mode==5;const unsigned damage=id==908?50:42;check(f.health()==(hit?(mode==3?0:1000-damage):(mode==3?1:1000)));
  const auto selected=id==908?bar[3]:bar[1];const auto actual_deadline=w.skill_disabled_slot(f.source,selected,1100);
  const auto expected_deadline=hit?1100+(id==908?10000u:3000u):0u;
  if(actual_deadline.value_or(0)!=expected_deadline)throw std::runtime_error("Self shot deadline"+context+" actual="+std::to_string(actual_deadline.value_or(0))+" expected="+std::to_string(expected_deadline));
  if(hit){check(w.skill_disabled_slot(f.source,{static_cast<std::uint16_t>(id),0},1100)==0);if(id==909)check(w.skill_disabled_slot(f.source,{909,1},1100)==4100);}
 }
 for(unsigned id:{908u,909u})for(bool npc:{false,true})for(unsigned mode=0;mode<4;++mode){
  context=" Runtime id="+std::to_string(id)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  ShotFixture f(id,npc);auto& w=f.world;auto bar=bar_for(id);check(w.configure_adrenaline(f.source,bar,100));casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet> wire;
  h.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};h.resources=[&](auto actor){return w.resources(actor);};h.target_scope=[&](auto actor){return w.effect_scope(actor);};h.current_skillbar=[&](){return std::optional(bar);};
  h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(f.source,slot,at);};h.consume_energy_at=[&](auto n,auto at){return w.consume_energy(1,n,f.source,at);};
  h.weapon_snapshot=[&](){return std::optional(f.weapon);};h.melee_target=[&](auto actor,auto at){return w.melee_target(f.source,actor,at,id);};
  h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){check(plan.copy==1);return w.melee_attack(f.source,target,gear,plan,at);};
  h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};
  Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[id]=2;policy.learned[403]=2;policy.learned[451]=1;attributes::Ranks ranks{};ranks[25]=12;
  check(runtime.start(1000,{id,1,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1200,h);if(mode==3)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,1200,3000));runtime.update(1500,h);w.tick(1600);runtime.update(1600,h);
  const bool hit=mode==0;check(f.health()==1000-(hit?(id==908?50:42):0));
  check(std::count(wire.begin(),wire.end(),casting::activation(1,f.target.actor,id,true))==1);
  check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,id==908?1058:1062))==unsigned(mode==0||mode==3));
  check(!particles::target_body_2022(f.target.actor,id));
  check(std::count(wire.begin(),wire.end(),casting::terminal(46,1))==0);
  check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,2077))==0);
  check(w.skill_disabled_slot(f.source,id==908?Slot{451,0}:Slot{909,0},1600).value_or(0)==(hit?1600+(id==908?10000u:3000u):0u));
  if(hit){const auto slot=id==908?Slot{451,0}:Slot{403,1};check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,slot,id==908?10:3))==1);}
 }
 for(unsigned id:{908u,909u}){
  ShotFixture f(id,false);auto& w=f.world;const auto bar=bar_for(id);check(w.configure_adrenaline(f.source,bar,100));
  const auto prior=*w.disable_reset_revision(f.source);check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(id,12,0),1000));w.tick(1100);
  const Slot selected=id==908?Slot{451,0}:Slot{403,1};check(!w.reset_skill_disables(f.source,{selected},prior,1100));
  const auto actual=*w.disable_reset_revision(f.source);check(actual!=prior&&w.reset_skill_disables(f.source,{selected},actual,1100));
  check(w.skill_disabled_slot(f.source,selected,1100)==0);w.tick(1200);check(w.skill_disabled_slot(f.source,selected,1200)==0);
 }
 {ShotFixture f(909,false);auto& w=f.world;check(w.configure_adrenaline(f.source,bar_for(909),100));check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(909,12,0),1000));
  w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},1050,1);w.set_vitals(1,{1000,1000,30,30},100);const auto replacement=*w.effect_scope(1);check(replacement!=f.source);check(w.configure_adrenaline(replacement,bar_for(909),1050));w.tick(1100);
  check(w.skill_disabled_slot(replacement,{403,0},1100)==0&&f.health()==1000);
 }
 // A real temporary-copy restoration wins its own mutex transaction just
 // before impact. The arrow observes the restored bar, not its release bar.
 for(unsigned id:{908u,909u}){
  ShotFixture f(id,false);auto& w=f.world;auto before=bar_for(id),after=before;after[2]={75,0};check(w.configure_adrenaline(f.source,before,100));
  TemporarySkillRestoration change{{2,{75,0},{403,1},100,20100,1,20000,EchoRestorationKind::independent_disable},true};
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(id,12,0),20000));
  check(w.restore_temporary_skill_slots(f.source,{before,after},{change},20100).has_value());w.tick(20100);
  check(f.health()==1000-(id==908?50:42));check(w.skill_disabled_slot(f.source,{403,1},20100)==0);
  check(w.skill_disabled_slot(f.source,{75,0},20100)==40100); // independent restoration barrier remains longer
  check(w.skill_disabled_slot(f.source,id==908?Slot{451,0}:Slot{403,0},20100)==20100+(id==908?10000u:3000u));
  check(w.configure_adrenaline(f.source,before,20200));check(w.skill_disabled_slot(f.source,{403,1},20200)==0);
 }
 // The deferred self-disable frontier must not block another arrow's
 // earlier impact or cancel the deferred shot when that boundary is visited.
 {context=" unrelated Called Shot boundary";ShotFixture f(909,false);auto& w=f.world;check(w.configure_adrenaline(f.source,bar_for(909),2000));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(909,12,0),1000));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(403,0,0),1020));
  w.tick(1120);check(f.health()==980);auto first=w.take_attack_outcomes(f.source);check(first.size()==1&&first[0].at==1054);
  check(w.skill_disabled_slot(f.source,{403,0},1120)==0);w.tick(2000);check(f.health()==938);
  auto second=w.take_attack_outcomes(f.source);check(second.size()==1&&second[0].at==2000);check(w.skill_disabled_slot(f.source,{403,0},2000)==5000);
 }
 std::cout<<"PASS self-disable shots native ranks,exactcopy/currentbar,playerNPC hit/miss/block/lethal/unknownbar,ledger ordering/replay/reset/barriers and Runtime cost/body/UI\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
