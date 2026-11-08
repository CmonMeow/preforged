#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Disenchantment line "+std::to_string(at.line()));}
int main()try{
 check(enchantment_removal(882)->lose_all_energy&&enchantment_removal(882)->count==1);
 for(unsigned mode=0;mode<8;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;w.set_vitals(1,{mode==6?50u:500u,1000,41,50},100);
  if(mode!=0)check(w.status_mutation(f.target,180,99,12,100,60100,true));
  if(mode==2)check(w.status_mutation(f.target,38,100,12,100,46100,true));
  if(mode==3)check(w.activate_overcast_energy(f.source,231,0,1,1000));
  if(mode==4||mode==6||mode==7)check(w.apply_mind_wrack(f.target,f.source,12,false,100));
  if(mode==5){w.set_vitals(1,{500,1000,0,30},100);check(w.energy_capacity(1,1,0,100,{0,-2}));}
  const auto at=mode==3?1501u:100u;
  auto stale=f.target;++stale.generation;check(!w.enchantment_removal(f.source,stale,*enchantment_removal(882),20,at));
  if(mode==7){check(w.settle_resources(f.target,200));check(!w.enchantment_removal(f.source,f.target,*enchantment_removal(882),20,100)&&w.resources(1)->energy==41);continue;}
  f.box->drain();check(w.enchantment_removal(f.source,f.target,*enchantment_removal(882),20,at));check(w.resources(1)->energy==0);
  check(w.status_snapshot(f.target,at)->size()==(mode==2?1u:0u));
  if(mode==3)check(overcast::precise_current(*w.overcast_energy_snapshot(f.source))==0);
  if(mode==6)check(w.resources(1)->health==0);
  if(mode==4)check(w.resources(1)->health==417&&w.status_snapshot(f.source,at)->empty());
  if(mode==5){check(w.restore_energy(1,10,f.source,100));check(w.resources(1)->energy==0);} // loss does not erase debt
  const auto wire=f.box->drain();check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,976))==1);
  check(std::count(wire.begin(),wire.end(),*particles::targeted_body_2022(2,1,882))==1);
 }
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;w.set_vitals(1,{500,1000,41,50},100);
  check(w.status_mutation(f.target,180,99,12,100,60100,true));casting::Runtime cast(1);casting::Hooks h;
  h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy_at=[&](auto cost,auto at){check(!cost);return w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(f.source,id,range,100);};
  h.enchantment_removal=[&](auto target,const auto& plan,auto at){return w.enchantment_removal(f.source,target,plan,20,at);};
  Policy p;p.primary=5;p.campaigns=255;p.learned[882]=1;Slots bar{};bar[0]={882,0};
  if(mode==4)h.enchantment_removal={};
  check(cast.start(100,{882,0,2},bar,p,{},true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(w.resources(1)->energy==41);if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(500,h);
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,30,30},100);}
  cast.update(1099,h);check(w.resources(1)->energy==41);cast.update(1100,h);check(w.resources(1)->energy==(mode?41u:0u));
  if(!mode)check(cast.start(2000,{882,0,2},bar,p,{},true,h)==casting::Reason::recharge);
 }
 {warrior_test::Fixture f(340,true);f.world.set_vitals(1,{1000,1000,41,50},100);
  check(f.world.enchantment_removal(f.source,f.target,*enchantment_removal(882),20,100)&&f.world.resources(1)->energy==0);
 }
 for(unsigned removal:{882u,68u,69u,141u,143u,766u,877u,955u,936u,1347u})for(bool lethal:{false,true}){world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{lethal?20.:400.,500};d.damageable=true;d.conditions_allowed=!(removal==955&&lethal);
  d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  auto npc_box=std::make_shared<world::Outbox>();w.join(1,"Mesmer",100,0,npc_box,0,{}, {},100,1);w.set_vitals(1,{500,1000,20,50},100);
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);check(w.elemental_armor(owner,60,60));
  check(w.energy_capacity(1,5,0,100,{30,-2}));check(w.observe_npc_overcast(target,2,1000,"fixture"));
  combat::ai::FlarePolicy ai;ai.source=target;ai.level=20;ai.fire_magic=12;ai.speed=300;ai.acquire=1000;ai.follow=ai.leash=1500;ai.flight={100,32};ai.evidence="fixture";
  namespace e=combat::ai::elemental;std::vector<e::Reviewed> book{{{214,0},e::Plan{*water_projectile(214,12)},"fixture"}};
  check(w.configure_npc_elemental_ai(ai,book,4,1000));w.tick(1000);const auto state=w.npc_elemental_ai_snapshot(target);check(state&&state->pending);
  const auto end=state->pending->token.completes;w.tick(end);check(w.npc_combat_snapshot(target)->effects&0x80);
  const auto npc_before=w.npc_combat_snapshot(target)->health->current;
  const auto before=*w.resources(1);
  if(removal==143){check(w.enchantment_removal(owner,target,*enchantment_removal(143,0),20,end));check(w.npc_combat_snapshot(target)->effects&0x80);check(w.resources(1)->health==before.health);}
  if(removal==877&&lethal){check(w.status_mutation(owner,180,100,12,end,end+60000,true));check(w.status_mutation(owner,241,101,12,end,end+60000,true));check(w.enchantment_removal(owner,target,*enchantment_removal(877),20,end));check(w.npc_combat_snapshot(target)->effects&0x80);continue;}
  if(removal==766&&lethal){check(w.enchantment_removal(owner,target,*enchantment_removal(766),20,end));check(w.npc_combat_snapshot(target)->effects&0x80);continue;}
  if(removal==1347)check(w.degeneration_hex(owner,target,*degeneration_hex(19,12),end));
  EnchantmentRemovalOutcome outcome;npc_box->drain();check(w.enchantment_removal(owner,target,*enchantment_removal(removal,12),20,end,&outcome));check(outcome.target_hexed==(removal==1347));
  const auto output=npc_box->drain();check(std::count(output.begin(),output.end(),(removal==955||removal==936||removal==1347)?combat::detail::integer(21,1,skills::find_skill(removal)->caster_body_resource):*particles::targeted_body_2022(100,1,removal))==1);
  check(w.resources(1)->energy==(removal==882?0u:removal==68?std::min(before.max_energy,before.energy+15):before.energy));
  check(w.resources(1)->health==(removal==68?std::min(before.max_health,before.health+104):removal==143?before.health+unsigned(std::min(53.,npc_before+1)):before.health));
  const auto health=w.npc_combat_snapshot(target)->health->current;
  check(std::abs(health-std::max(0.,npc_before-(removal==69?83:removal==143?53:0)))<0.000001);check(!(w.npc_combat_snapshot(target)->effects&0x80));
  check(w.has_condition(target,combat::conditions::Kind::bleeding,end)==(removal==955&&!lethal));
  w.tick(end+500);check(std::abs(w.npc_combat_snapshot(target)->health->current-(health-(removal==955&&!lethal?3.:0.)))<0.000001);
 }
 std::cout<<"PASS Disenchantment completion-time all Energy loss,covered enchant removal,cap/debt/Overcast/Mind Wrack,cancel/lifetime and owned NPC regeneration removal\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
