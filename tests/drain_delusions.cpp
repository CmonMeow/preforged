#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Drain Delusions line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope a,b;
 Fixture(unsigned energy=20,bool npc=false,bool known=true,unsigned source_energy=5){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;
   if(known){d.spawn_energy=combat::npcs::Energy{energy,30};d.energy_pips=0;}n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,source_energy,50},100);check(w.energy_capacity(1,5,0,100,{30,-2}));w.set_vitals(1,{500,1000,source_energy,50},100);a=*w.effect_scope(1);check(w.settle_resources(a,100));
  if(!npc){w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,1000,energy,30},100);check(w.energy_capacity(2,5,0,100,{10,-2}));w.set_vitals(2,{500,1000,energy,30},100);check(w.settle_resources(*w.effect_scope(2),100));}
  b=*w.effect_scope(npc?100:2);
 }
 void hex(){check(w.degeneration_hex(a,b,*degeneration_hex(19,12),100));}
};
int main()try{
 check(drain_delusions(0)->loss==1&&drain_delusions(12)->loss==4&&drain_delusions(20)->loss==6&&!drain_delusions(21));check(casting::Runtime::effect_supported(1337));
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned energy:{0u,1u,20u})for(bool hexed:{false,true}){
  Fixture f(energy,npc);if(hexed)f.hex();f.box->drain();const auto plan=*drain_delusions(rank);auto bad=plan;++bad.multiplier;check(!f.w.drain_delusions(f.a,f.b,bad,100));auto stale=f.b;++stale.generation;check(!f.w.drain_delusions(f.a,stale,plan,100));check(!f.w.drain_delusions(f.a,f.a,plan,100));
  check(f.w.drain_delusions(f.a,f.b,plan,100));check(f.w.resources(1)->energy==5+(hexed?std::min(energy,plan.loss)*4:0));check(f.w.resources(1)->health==500);
  if(!npc)check(f.w.resources(2)->energy==energy-(hexed?std::min(energy,plan.loss):0)&&f.w.status_snapshot(f.b,100)->empty());
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(f.b.actor,1,1337))==unsigned(hexed));
 }
 // NPC unknown Energy fails before removing its owned hex.
 {Fixture f(0,true,false);f.hex();check(!f.w.drain_delusions(f.a,f.b,*drain_delusions(12),100));check(f.w.npc_combat_snapshot(f.b)->effects&0x800);check(f.w.resources(1)->energy==5);}
 // Newest Mesmer selection ignores a newer Necromancer hex.
 {Fixture f;f.hex();check(f.w.degeneration_hex(f.a,f.b,*degeneration_hex(129,12),100));check(f.w.drain_delusions(f.a,f.b,*drain_delusions(12),100));const auto left=*f.w.status_snapshot(f.b,100);check(left.size()==1&&left[0].skill==129);}
 // Native Phantom Pain's end condition runs through the normal player path.
 {Fixture f;check(f.w.wound_spell(f.a,f.b,*wound_spell(44,12),20,100));check(f.w.drain_delusions(f.a,f.b,*drain_delusions(12),100));check(f.w.has_condition(f.b,combat::conditions::Kind::deep_wound,100));}
 for(unsigned at:{225u,475u}){Fixture f(2);check(f.w.energy_capacity(2,5,0,100,{10,2}));f.hex();check(f.w.drain_delusions(f.a,f.b,*drain_delusions(12),at));
  check(f.w.resources(2)->energy==0);check(f.w.activate_overcast_energy(f.a,231,0,1,at));
  check(overcast::precise_current(*f.w.overcast_energy_snapshot(f.a))==(at==225?4100000:4500000)); // 5+(2+1/6)*4;5+2.5*4
 }
 {Fixture f(20,false,true,39);f.hex();check(f.w.activate_overcast_energy(f.a,231,0,1,100));check(f.w.drain_delusions(f.a,f.b,*drain_delusions(12),100));check(overcast::precise_current(*f.w.overcast_energy_snapshot(f.a))==12000000&&f.w.resources(2)->energy==16);}
 {Fixture f(4,false,true,0);f.hex();check(f.w.energy_capacity(1,5,0,100,{10,-2}));check(f.w.drain_delusions(f.a,f.b,*drain_delusions(12),100));check(f.w.resources(1)->energy==0);check(f.w.energy_capacity(1,5,0,100,{30,-2}));check(f.w.resources(1)->energy==16);}
 for(bool npc:{false,true})for(bool remove_wrack:{false,true}){Fixture f(1,npc);if(remove_wrack)f.hex();check(f.w.apply_mind_wrack(f.a,f.b,12,false,remove_wrack?101:100));if(!remove_wrack)check(f.w.degeneration_hex(f.a,f.b,*degeneration_hex(19,12),101));
  check(f.w.drain_delusions(f.a,f.b,*drain_delusions(12),101));const auto health=npc?f.w.npc_combat_snapshot(f.b)->health->current:double(f.w.resources(2)->health);check(health==(remove_wrack?500:417));check(f.w.resources(1)->energy==9);
 }
 {Fixture f(4);f.hex();check(f.w.settle_resources(f.b,200));check(!f.w.drain_delusions(f.a,f.b,*drain_delusions(12),100));check(f.w.status_snapshot(f.b,200)->size()==1&&f.w.resources(1)->energy==5);}
 // NPC's own fractional pool is drained precisely without a second pool.
 {combat::npcs::Definition d;d.binding.map=146;d.binding.actor=2;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=true;d.spawn_energy=combat::npcs::Energy{2,30};d.energy_pips=4;
  combat::npcs::Lifetime life{1,1,1,146,2};combat::npcs::LiveActor live(d,life);check(live.advance_energy(life,100));const auto lost=live.drain_energy_units(life,4,225);check(lost&&*lost==650000&&live.precise_energy_units(life)==0);}
 for(unsigned mode=0;mode<5;++mode){Fixture f;f.hex();casting::Runtime cast(1);casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return f.w.resources(id);};h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.a,at);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return f.w.projectile_target_valid(f.a,id,range,100);};h.drain_delusions=[&](auto target,const auto& plan,auto at){return f.w.drain_delusions(f.a,target,plan,at);};
  Policy p;p.primary=5;p.campaigns=255;p.learned[1337]=1;Slots bar{};bar[0]={1337,0};attributes::Ranks ranks{};ranks[3]=12;if(mode==4)h.drain_delusions={};check(cast.start(100,{1337,0,2},bar,p,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(200,h);if(mode==3){f.w.leave(2);f.w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{500,1000,20,30},100);}
  cast.update(349,h);check(f.w.resources(1)->energy==0);cast.update(350,h);check(f.w.resources(1)->energy==(mode?0u:16u));if(!mode)check(cast.start(12000,{1337,0,2},bar,p,ranks,true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Drain Delusions player/NPC removal,actual/fractional transfers,cap/debt/Overcast,Mind Wrack,end conditions,cast control/lifetime\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
