#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Skull World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned mode=0;mode<9;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;
  if(mode!=3)check(w.cast_activity(f.target,mode==1?1:281,100,3000));
  if(mode==2){check(w.cast_activity(f.target,281,0,0));check(w.attack_activity(f.target,3000,100));}
  if(mode==5)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==6){check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});}
  if(mode==7||mode==8)check(w.status_mutation(f.target,mode==7?16:17,123,0,100,10100,true));
  const auto epoch=w.control_epoch(f.target);f.box->drain();const auto at=mode==4?3000u:1000u;
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(329,0),at));
  const bool hit=mode!=5&&mode!=6,spell=mode==0||mode>=7;
  check(w.has_condition(f.target,combat::conditions::Kind::dazed,at)==(hit&&spell));
  check((w.control_epoch(f.target)!=epoch)==(mode==0||mode==1||mode==2));
  if(mode==7||mode==8)check(w.casting_skill(f.target,at)==281);
  if(mode==8)check(w.resources(2)->energy==20);
  const auto wire=f.box->drain();const auto body=*particles::targeted_body_2022(2,1,329);check(std::count(wire.begin(),wire.end(),body)==unsigned(hit));
  if(spell&&hit){check(w.has_condition(f.target,combat::conditions::Kind::dazed,15999));check(!w.has_condition(f.target,combat::conditions::Kind::dazed,16000));}
 }
 for(unsigned mode=0;mode<7;++mode){warrior_test::Fixture f(340,true,1000,true,mode!=4,1,mode==6?std::nullopt:std::optional(combat::npcs::Energy{20,20}));auto& w=f.world;
  const auto token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,100,3000,Slot{static_cast<std::uint16_t>(mode==1?1:281),0});check(token.has_value());
  if(mode==2||mode==3||mode==6)check(w.npc_interrupt_stance(f.target,mode==2?16:17,0,10000,100));
  if(mode==5)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(329,0),1000)==(mode!=6));
  check(w.has_condition(f.target,combat::conditions::Kind::dazed,1000)==(mode==0||mode==2||mode==3));
  check(w.complete_npc_action(*token,3000)==(mode==2||mode==3||mode==5||mode==6));
  if(mode==3)check(w.npc_energy_snapshot(f.target)->current==10);if(mode==6)check(f.health()==1000);
 }
 for(unsigned mode=0;mode<3;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={329,0};bar[1]={340,0};check(w.configure_adrenaline(f.source,bar,100));
  for(unsigned i=0;i<9;++i)check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200+i));
  check(w.adrenaline(f.source)->front().units==220);check(w.cast_activity(f.target,281,300,3000));casting::Hooks h;unsigned commits=0;
  h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy_at=[&](auto n,auto at){check(n==0);return w.consume_energy(1,n,f.source,at);};
  h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(f.source,slot,at);};h.reconcile_adrenaline=[&](auto at){check(w.reconcile_adrenaline(f.source,at));};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.weapon_snapshot=[&](){return std::optional(f.weapon);};h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,329);};
  h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++commits;return w.melee_attack(f.source,target,gear,plan,at);};h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(f.source);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};
  Policy p;p.primary=1;p.campaigns=255;p.learned[329]=p.learned[340]=1;attributes::Ranks ranks{};casting::Runtime runtime(1);
  check(runtime.start(1000,{329,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);check(w.adrenaline(f.source)->front().units==0&&w.resources(1)->energy==30);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1200,h);runtime.update(1499,h);check(!commits);runtime.update(1500,h);
  check(commits==unsigned(mode==0)&&w.has_condition(f.target,combat::conditions::Kind::dazed,1500)==(mode==0));
 }
 std::cout<<"PASS Skull player/NPC spell-only15s Dazed and action interrupt,Mantra single charge/drain,immune/miss/block/expiry/unknown Energy,body576 and Runtime220-unit lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
