// Detached integration consumers: actual maintained248 + production controller motion.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Controller maintainer line "+std::to_string(at.line()));
}
world::Instance::Npc npc(unsigned id,float x,unsigned energy=100){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.primary=3;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding={146,id,id,0,0,0,0,20};d.relation=combat::npcs::Relation::hostile_to_players;d.packed_allegiance=3;
 d.spawn_health=combat::Health{1000,1000};d.spawn_energy=combat::npcs::Energy{energy,100};d.energy_pips=4;
 d.physical_armor=60;d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic maintained controller integration";
 d.holy_traits={false,false};d.light_sensitive_evidence=d.summoned_evidence=d.healing_evidence;n.combat_definition=d;return n;
}
std::uint32_t u32(const world::Packet& p,std::size_t at){return std::uint32_t(p[at])|std::uint32_t(p[at+1])<<8|std::uint32_t(p[at+2])<<16|std::uint32_t(p[at+3])<<24;}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope caster,recipient,moving;bool moving_recipient;
 explicit Fixture(unsigned kind,bool recipient_moves,bool lease=true,bool zero=false):moving_recipient(recipient_moves){
  std::vector<world::Instance::Npc> bodies{npc(100,0,zero?1:100),npc(101,100)};
  if(zero)for(unsigned id=102;id<=104;++id)bodies.push_back(npc(id,float(100+(id-101)*20)));
  w.configure_npcs(bodies);w.join(1,"Remote authored foe",9000,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto enemy=*w.effect_scope(1);caster=*w.effect_scope(100);recipient=*w.effect_scope(101);moving=recipient_moves?recipient:caster;
  check(w.physical_armor(enemy,0)&&w.elemental_armor(enemy,0,0));
  combat::ai::FlarePolicy p;p.source=moving;p.target=enemy;p.level=20;p.speed=440;p.acquire=p.follow=p.leash=10000;
  p.flight={1000,32};p.evidence="synthetic maintained controller production route";
  if(kind==0)check(w.configure_npc_flare_ai(p,100));
  if(kind==1){check(w.configure_npc_elemental_ai(p,{{{214,0},*water_projectile(214,0),p.evidence}},4,100));check(w.observe_npc_overcast(moving,0,100,p.evidence));}
  if(kind==2){combat::ai::ordinary::Reviewed a{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},p.evidence};check(w.configure_npc_hostile_ordinary(p,a,100));}
  if(lease){check(w.stage_retribution(caster,recipient,*retribution(12),100));
   if(zero){for(unsigned id=102;id<=104;++id)check(w.stage_retribution(caster,*w.effect_scope(id),*retribution(0),100));
    check(w.stage_retribution(caster,caster,*retribution(0),100));}}
  w.tick(100);check(w.npc_pursuit_movement_snapshot(moving).has_value());box->drain();
 }
 double materialize(std::uint64_t at){w.tick(at);double x=moving_recipient?100:0;
  for(const auto& p:box->drain())if(p.size()==16&&p[0]==44&&p[1]==0&&u32(p,2)==moving.actor)x=std::bit_cast<float>(u32(p,6));return x;}
};
std::uint64_t independent_crossing(unsigned kind,bool recipient){
 const auto outside=[&](std::uint64_t at){Fixture oracle(kind,recipient,false);const auto x=oracle.materialize(at);return recipient?x>5000.:x-100.>5000.;};
 check(!outside(100)&&outside(15000));std::uint64_t low=100,high=15000;
 while(high-low>1){const auto middle=low+(high-low)/2;if(outside(middle))high=middle;else low=middle;}return high;
}
int main()try{
 for(unsigned kind=0;kind<3;++kind)for(bool recipient_moves:{false,true}){
  const auto crossing=independent_crossing(kind,recipient_moves);
  for(bool dense:{false,true}){Fixture f(kind,recipient_moves);const auto entry=f.w.status_snapshot(f.recipient,100)->front();const auto end=*mending_end_wire(f.caster,f.recipient,entry);
   if(dense)for(std::uint64_t at=200;at<crossing-1;at+=100){f.w.tick(at);check(f.w.maintained_enchantment_count(f.caster,at)==1);f.box->drain();}
   f.w.tick(crossing-1);check(f.w.maintained_enchantment_count(f.caster,crossing-1)==1);f.box->drain();
   f.w.tick(crossing);check(f.w.maintained_enchantment_count(f.caster,crossing)==0&&f.w.status_snapshot(f.recipient,crossing)->empty());
   const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),end.effect)==1);check(std::count(packets.begin(),packets.end(),end.upkeep)==0);
   f.w.tick(crossing+100);const auto repeated=f.box->drain();check(std::count(repeated.begin(),repeated.end(),end.effect)==0);
  }
 }
 // Actual controller motion must not reorder the newest-first first-zero
 // release:1Energy with5upkeep over4base pips reaches zero exactly at3100.
 for(unsigned kind=0;kind<3;++kind)for(bool dense:{false,true}){Fixture f(kind,false,true,true);
  check(f.w.maintained_enchantment_count(f.caster,100)==5);
  const auto self=f.w.status_snapshot(f.caster,100);check(self&&self->size()==1);const auto end=*mending_end_wire(f.caster,f.caster,self->front());
  if(dense)for(std::uint64_t at=200;at<3099;at+=100){f.w.tick(at);f.box->drain();}
  f.w.tick(3099);check(f.w.maintained_enchantment_count(f.caster,3099)==5);f.box->drain();
  f.w.tick(3100);check(f.w.maintained_enchantment_count(f.caster,3100)==4&&f.w.npc_energy_snapshot(f.caster)->current==0);
  check(f.w.status_snapshot(f.caster,3100)->empty());const auto packets=f.box->drain();
  check(std::count(packets.begin(),packets.end(),end.effect)==1&&std::count(packets.begin(),packets.end(),end.upkeep)==0);
  f.w.tick(3200);check(f.w.maintained_enchantment_count(f.caster,3200)==4);
 }
 std::cout<<"PASS moving Flare/elemental/ordinary NPC maintainers+recipients: exact range, sparse/dense, newest-first zero, no NPC private upkeep\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
