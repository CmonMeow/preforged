#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Demonic Flesh World line "+std::to_string(at.line()));}
struct Fixture {
 world::Instance w;effects::Scope owner,target,npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(){
  std::vector<world::Instance::Npc> actors;
  for(unsigned id=100;id<=100u;++id){world::Instance::Npc n;n.id=n.definition.id=id;n.x=10;n.definition.visual_adjustment={0,0,0,static_cast<unsigned char>(id)};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=true;
   d.spawn_health=combat::Health{500,500};n.combat_definition=d;actors.push_back(n);}
  w.configure_npcs(actors);
  for(unsigned id=1;id<=5;++id){const float x=id==1?0:id==2?100:id==3?166:id==4?166.1f:20;
   w.join(id,"Player",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);w.set_vitals(id,{1000,1000,100,100},100);}
  owner=*w.effect_scope(1);target=*w.effect_scope(2);npc=*w.effect_scope(100);box->drain();
 }
 void apply(unsigned rank=12,unsigned effect=1,std::uint64_t at=1000){check(w.demonic_flesh(owner,effect,*effects::demonic_flesh(rank),at));}
 bool use(std::uint64_t event=1,std::uint64_t at=2000,bool success=true){return w.demonic_flesh_skill_used({owner,target,194,event,at,success,false},20);}
};
int main()try{
 for(bool reversal:{false,true}){Fixture f;f.apply();const auto collateral=*f.w.effect_scope(3);
  check(f.w.status_mutation(collateral,1399,80,12,100,6100,true));
  if(reversal)check(f.w.status_mutation(collateral,307,81,12,100,8100,true));
  check(f.use(1,2000));check(f.w.resources(3)->health==(reversal?1000u:988u));
  check(f.use(2,2100));check(f.w.resources(3)->health==(reversal?993u:981u));
  check(!f.use(2,2100));f.w.tick(2100);check(f.w.resources(3)->health==(reversal?993u:981u));
 }

 {Fixture f;f.apply();check(f.use());check(f.w.resources(2)->health==1000&&f.w.resources(3)->health==983&&f.w.resources(4)->health==1000&&f.w.resources(5)->health==1000);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==483&&!f.use());check(f.use(2,2100));check(f.w.resources(3)->health==966);
  check(!f.w.status_mutation(f.owner,130,3,12,2200,56000,true));
  check(f.w.status_mutation(f.owner,130,1,12,2200,{},false));check(f.use(3,2300)&&f.w.resources(3)->health==966);
 }
 {Fixture f;f.apply();check(f.use(1,2000,false)&&f.w.resources(3)->health==1000);
  check(f.w.demonic_flesh_skill_used({f.owner,*f.w.effect_scope(5),194,2,2100,true,true},20)&&f.w.resources(3)->health==1000); // Caller cannot forge allegiance.
  check(!f.w.demonic_flesh_skill_used({f.owner,f.target,0,3,2200,true,true},20));
 }
 {Fixture f;f.apply(0);f.apply(12,2,1500);check(f.use());check(f.w.resources(3)->health==983);
  check(!f.w.demonic_flesh(f.owner,2,*effects::demonic_flesh(0),2100));
  f.w.tick(55500);check(f.use(2,55500)&&f.w.resources(3)->health==983); // Exact expiry,not a replacement's old deadline.
 }
 {Fixture f;f.apply();check(f.use());check(f.w.kill(f.owner,2200));check(f.w.resurrect(f.owner,2300));f.apply(12,2,2400);check(!f.use(1,2500));}
 {Fixture f;f.apply();const auto receipt=f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);check(receipt&&receipt->id);
  auto forged=*receipt;++forged.event;check(!f.w.commit_demonic_flesh_use(forged));
  check(f.w.cancel_demonic_flesh_use(*receipt)&&!f.w.commit_demonic_flesh_use(*receipt));
  check(!f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20));
  f.w.tick(3000);check(f.w.resources(3)->health==1000&&f.w.npc_combat_snapshot(f.npc)->health->current==500);}
 {Fixture f;f.apply();const auto receipt=*f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);
  // Target replacement and enchantment replacement cannot reinterpret a use
  // that already selected its collateral before the primary effect.
  f.w.leave(2);f.w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},2100,2);f.w.set_vitals(2,{1000,1000,100,100},100);
  f.apply(0,2,2100);f.w.move(3,600,0,2100);
  check(f.w.commit_demonic_flesh_use(receipt)&&f.w.commit_demonic_flesh_use(receipt));
  check(!f.w.cancel_demonic_flesh_use(receipt));f.w.tick(2200);
  check(f.w.resources(3)->health==983&&f.w.resources(2)->health==1000&&f.w.npc_combat_snapshot(f.npc)->health->current==483);
  check(!f.w.commit_demonic_flesh_use(receipt));f.w.tick(2300);check(f.w.resources(3)->health==983);}
 {Fixture f;f.apply();const auto receipt=*f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);
  check(f.w.settle_resources(*f.w.effect_scope(3),5000));check(f.w.settle_resources(f.owner,2100));
  check(f.w.commit_demonic_flesh_use(receipt));f.w.tick(2000);
  check(f.w.resources(3)->health==1000&&f.w.npc_combat_snapshot(f.npc)->health->current==483);
  f.w.tick(5000);check(f.w.resources(3)->health==983&&f.w.npc_combat_snapshot(f.npc)->health->current==483);
  f.w.tick(5100);check(f.w.resources(3)->health==983);}
 {Fixture f;f.apply();const auto receipt=*f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);
  f.w.leave(3);f.w.join(3,"New collateral",166,0,std::make_shared<world::Outbox>(),0,{}, {},2100,2);f.w.set_vitals(3,{1000,1000,100,100},100);
  check(f.w.commit_demonic_flesh_use(receipt));f.w.tick(2200);check(f.w.resources(3)->health==1000&&f.w.npc_combat_snapshot(f.npc)->health->current==483);}
 {Fixture f;f.apply();const auto receipt=*f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);
  check(f.w.kill(f.owner,2100)&&f.w.commit_demonic_flesh_use(receipt));f.w.tick(2200);check(f.w.resources(3)->health==983);}
 for(bool committed_before_death:{false,true}){Fixture f;f.apply();const auto recipient=*f.w.effect_scope(3);
  const auto receipt=*f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);
  if(committed_before_death)check(f.w.commit_demonic_flesh_use(receipt));
  check(f.w.kill(recipient,2100)&&f.w.resurrect(recipient,2200));check(f.w.effect_scope(3)==recipient);
  const auto revived_health=f.w.resources(3)->health;
  if(!committed_before_death)check(f.w.commit_demonic_flesh_use(receipt));
  f.w.tick(2300);check(f.w.resources(3)->health==revived_health&&f.w.npc_combat_snapshot(f.npc)->health->current==483);
  f.w.tick(2400);check(f.w.resources(3)->health==revived_health&&f.w.npc_combat_snapshot(f.npc)->health->current==483);
 }
 {Fixture f;f.apply();const auto receipt=*f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);
  f.w.tick(60000);check(f.w.commit_demonic_flesh_use(receipt));f.w.tick(60100);check(f.w.resources(3)->health==983);}
 {Fixture f;f.apply();const auto receipt=*f.w.prepare_demonic_flesh_use({f.owner,f.target,194,1,2000},20);
  check(f.w.commit_demonic_flesh_use(receipt));f.w.leave(1);f.w.tick(2200);check(f.w.resources(3)->health==1000);}
 {Fixture f;f.apply();std::vector<effects::DemonicFleshReservation> reservations;
  for(unsigned event=1;event<=4096;++event){const auto receipt=f.w.prepare_demonic_flesh_use({f.owner,f.target,194,event,2000},20);check(receipt&&receipt->id);reservations.push_back(*receipt);}
  check(!f.w.prepare_demonic_flesh_use({f.owner,f.target,194,4097,2000},20));
  const auto unenchanted=*f.w.effect_scope(5);
  const auto no_plan=f.w.prepare_demonic_flesh_use({unenchanted,f.target,194,1,2000},20);check(no_plan&&!no_plan->id&&f.w.commit_demonic_flesh_use(*no_plan));
  // The selected foe is in spell range, but all collateral is outside the
  // new caster's adjacent radius. A full queue cannot block this ordinary use.
  f.w.join(6,"Distant caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},1000,1);f.w.set_vitals(6,{1000,1000,100,100},100);
  const auto distant=*f.w.effect_scope(6);
  check(f.w.demonic_flesh(distant,1,*effects::demonic_flesh(12),1000));
  const auto no_collateral=f.w.prepare_demonic_flesh_use({distant,f.target,194,1,2000},20);
  check(no_collateral&&!no_collateral->id&&f.w.commit_demonic_flesh_use(*no_collateral));
  check(!f.w.prepare_demonic_flesh_use({f.owner,f.target,194,4097,2000},20));
  check(f.w.cancel_demonic_flesh_use(reservations.back()));reservations.pop_back();
  const auto next=f.w.prepare_demonic_flesh_use({f.owner,f.target,194,4097,2000},20);check(next&&f.w.cancel_demonic_flesh_use(*next));
  for(const auto& receipt:reservations)check(f.w.cancel_demonic_flesh_use(receipt));}
 for(unsigned mode=0;mode<7;++mode){Fixture f;casting::Runtime runtime(1);casting::Hooks h;unsigned casts=0,uses=0;std::uint64_t use_at=0;bool reserved_proc=false;
  check(f.w.energy_capacity(1,6,0,100,{80,-2}));
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){f.w.skill_event(1,p);};
  h.consume_energy_at=[&](auto amount,auto at){return f.w.consume_energy(1,amount,f.owner,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto,float,bool){return true;};h.projectile_target_valid=[](auto,float){return true;};
  h.elemental=[&](auto,const auto&,auto)->casting::EffectCompletion{
   if(mode==5||mode==6)check(reserved_proc); // Reservation must precede the successful primary callback.
   if(mode==5){check(f.w.settle_resources(f.owner,4001));check(f.w.settle_resources(*f.w.effect_scope(3),5000));}
   if(mode==6){f.w.leave(2);f.w.join(2,"New Target",100,0,std::make_shared<world::Outbox>(),0,{}, {},4001,2);f.w.set_vitals(2,{1000,1000,100,100},100);}
   return {mode!=4,{}};
  };
  if(mode!=3)h.demonic_flesh=[&](auto effect,const auto& plan,auto at)->casting::EffectCompletion{++casts;std::vector<world::Packet> out;const bool ok=f.w.demonic_flesh(f.owner,effect,plan,at,&out);return {ok,std::move(out)};};
  h.prepare_demonic_flesh_use=[&](const auto& use){use_at=use.at;auto receipt=f.w.prepare_demonic_flesh_use(use,20);reserved_proc=receipt&&receipt->id;return receipt;};
  h.commit_demonic_flesh_use=[&](const auto& receipt){++uses;const bool ok=f.w.commit_demonic_flesh_use(receipt);f.w.tick(use_at);return ok;};
  h.cancel_demonic_flesh_use=[&](const auto& receipt){return f.w.cancel_demonic_flesh_use(receipt);};
  Slots bar{};bar[0]={130,0};bar[1]={194,0};Policy policy;policy.primary=4;policy.secondary=6;policy.campaigns=255;policy.learned[130]=policy.learned[194]=1;attributes::Ranks ranks{};ranks[4]=12;
  check(runtime.start(1000,{130,0,2},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==3){check(f.w.resources(1)->energy==100);continue;}
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1200,h);runtime.update(2000,h);
  check(casts==unsigned(mode==0||mode>=4));if(mode==1||mode==2)continue;
  const auto packets=f.box->drain();const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  const auto first=std::find(packets.begin(),packets.end(),casting::generic(21,1,239)),second=std::find(packets.begin(),packets.end(),casting::generic(21,1,240));check(finish<first&&first<second);
  check(f.w.resources(3)->health==1000&&f.w.resources(1)->energy==95);
  const auto before=uses;check(runtime.start(3000,{194,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);runtime.update(4000,h);
  check(f.w.resources(3)->health==((mode==4||mode==5)?1000:983)&&uses==before+unsigned(mode!=4));
  if(mode==5||mode==6){
   check(f.w.npc_combat_snapshot(f.npc)->health->current==483);
   f.w.tick(5000);check(f.w.resources(3)->health==983&&f.w.resources(2)->health==1000&&f.w.npc_combat_snapshot(f.npc)->health->current==483);
   f.w.tick(5100);check(f.w.resources(3)->health==983&&f.w.npc_combat_snapshot(f.npc)->health->current==483);
  }
 }
 std::cout<<"PASS Demonic Flesh owned lifecycle,mixed caster-adjacent collateral,excluded selected/allies,replay,failed-use refusal,expiry/replacement/death and Runtime/native layer order\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
