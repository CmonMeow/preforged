#include "gw_npc_elemental_ai.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace elemental=combat::ai::elemental;
void check(bool yes,std::source_location at=std::source_location::current()){
 if(!yes)throw std::runtime_error("NPC elemental World line "+std::to_string(at.line()));
}
std::vector<elemental::Reviewed> loadout(std::initializer_list<unsigned> ids){
 std::vector<elemental::Reviewed> out;for(auto id:ids)out.push_back({{static_cast<std::uint16_t>(id),0},
  id==214?elemental::Plan{*skills::effects::water_projectile(214,12)}:elemental::Plan{*skills::effects::elemental(id,12)},
  "authored World fixture, canonical native offensive plan"});return out;
}
struct Fixture {
 world::Instance w;skills::effects::Scope source,first,second;world::Instance::Npc appearance;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool second_armor=true,unsigned source_health=500,float first_x=100){
  world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(source_health),source_health};
  d.damageable=true;d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;n.combat_definition=d;appearance=n;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,id==1?"Primary":"Secondary",id==1?first_x:200,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{1000,1000,0,100},100);if(id==1||second_armor)check(w.elemental_armor(*w.effect_scope(id),60,60));}
  source=*w.effect_scope(100);first=*w.effect_scope(1);second=*w.effect_scope(2);
 }
 combat::ai::FlarePolicy policy(){combat::ai::FlarePolicy p;p.source=source;p.level=20;p.fire_magic=12;p.speed=300;p.acquire=1000;p.follow=1500;p.leash=1500;
  p.flight={100,32};p.evidence="authored synthetic NPC movement/resource/loadout fixture";return p;}
 bool start(std::initializer_list<unsigned> ids={186,194,191},std::size_t capacity=4,std::uint64_t now=1000){return w.configure_npc_elemental_ai(policy(),loadout(ids),capacity,now);}
 bool zero_overcast(std::uint64_t now=1000){return w.observe_npc_overcast(source,0,now,"explicit authored zero-Overcast fixture");}
 void renew_population(std::uint64_t now){
  w.leave(1);w.leave(2);w.configure_npcs({appearance});source=*w.effect_scope(100);
  for(unsigned id:{1u,2u}){w.join(id,id==1?"Primary":"Secondary",float(id*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},now,id);
   w.set_vitals(id,{1000,1000,0,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  first=*w.effect_scope(1);second=*w.effect_scope(2);
 }
 std::uint64_t pending_end(){const auto state=w.npc_elemental_ai_snapshot(source);check(state&&state->pending);return state->pending->token.completes;}
 std::uint64_t begin(){w.tick(1000);return pending_end();}
 std::uint64_t launch(){const auto end=begin();w.tick(end);return end;}
 std::uint64_t arrival(std::uint64_t released){return released+projectiles::intercept(policy().flight,0,0,100,0,0,0)->flight_ms;}
 void tick(std::uint64_t until){for(std::uint64_t now=1000;now<=until;now+=100)w.tick(now);}
};
std::uint32_t value(const world::Packet& p,std::size_t at){return std::uint32_t(p[at])|(std::uint32_t(p[at+1])<<8)|(std::uint32_t(p[at+2])<<16)|(std::uint32_t(p[at+3])<<24);}
bool caster332(const world::Packet& p){return p.size()==14&&p[0]==158&&p[1]==0&&value(p,2)==21&&value(p,6)==100&&value(p,10)==332;}
bool impact(const world::Packet& p,unsigned target,unsigned resource){return p.size()==18&&p[0]==159&&p[1]==0&&value(p,2)==20&&value(p,6)==target&&value(p,10)==100&&value(p,14)==resource;}
int main()try{
 {Fixture f;f.w.leave(2);f.w.set_vitals(1,{1,1000,0,100},100);
  check(f.w.status_mutation(f.first,1399,80,12,100,6100,true));
  check(f.w.status_mutation(f.first,307,81,12,100,8100,true));
  check(f.start({191}));const auto end=f.begin();f.w.tick(end);
  check(f.w.resources(1)->health>0);
  check(f.w.has_condition(f.first,combat::conditions::Kind::burning,end));
 }

 {Fixture f(true,500,1000);f.w.leave(2);check(f.start({183}));f.box->drain();f.w.tick(1000);
  check(!f.w.npc_elemental_ai_snapshot(f.source)->pending&&f.w.npc_energy_snapshot(f.source)->current==50);
  std::uint64_t end=0;
  for(std::uint64_t now=1100;now<=4000;now+=100){f.w.tick(now);const auto state=*f.w.npc_elemental_ai_snapshot(f.source);
   if(state.pending){check(state.pending->use.slot.skill==183);end=state.pending->token.completes;break;}
   check(f.w.npc_energy_snapshot(f.source)->current==50);
  }
  check(end&&f.w.npc_energy_snapshot(f.source)->current==40);const auto starts=f.box->drain();
  check(std::any_of(starts.begin(),starts.end(),[](const auto& p){return p.size()==14&&p[0]==158&&value(p,2)==60&&value(p,10)==183;}));
  check(std::none_of(starts.begin(),starts.end(),[](const auto& p){return p.size()==18&&p[0]==159&&value(p,2)==60;}));
  f.w.tick(end);check(f.w.resources(1)->health==886&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
  const auto packets=f.box->drain();check(std::find(packets.begin(),packets.end(),combat::detail::integer(21,100,328))!=packets.end());
 }
 {Fixture f(true,500,1000);f.w.leave(2);check(f.start({183,194}));f.begin();
  check(f.w.npc_elemental_ai_snapshot(f.source)->pending->use.slot.skill==194&&f.w.npc_energy_snapshot(f.source)->current==45);}
 {Fixture f;check(f.start({183,194}));const auto end=f.launch();f.w.tick(end+750);
  check(f.w.npc_elemental_ai_snapshot(f.source)->pending->use.slot.skill==194&&f.w.npc_energy_snapshot(f.source)->current==35);}
 {Fixture f;check(f.start({183}));const auto end=f.begin();f.w.leave(1);
  // The original foe leaves; another foe inside the caster's radius is hit.
  f.w.leave(2);f.w.join(2,"Nearby",150,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);
  f.w.set_vitals(2,{1000,1000,0,100},100);check(f.w.elemental_armor(*f.w.effect_scope(2),60,60));
  f.w.tick(end);check(f.w.resources(2)->health==886&&!f.w.npc_elemental_ai_snapshot(f.source)->pending);
 }
 for(unsigned mode=0;mode<4;++mode){Fixture f(true,500,1600);f.w.leave(2);
  auto policy=f.policy();policy.acquire=2000;policy.follow=2500;policy.leash=mode==3?10:2000;
  check(f.w.configure_npc_elemental_ai(policy,loadout({194}),4,1000));f.w.tick(1000);f.w.tick(1100);
  const auto pursuing=*f.w.npc_elemental_ai_snapshot(f.source);
  check(!pursuing.pending);
  if(mode==3)check(!pursuing.engaged&&pursuing.returning); // clipped at the ten-unit home boundary
  else check(pursuing.engaged&&!pursuing.returning);
  f.w.join(2,"Alternative",300,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);
  f.w.set_vitals(2,{1000,1000,0,100},100);const auto other=*f.w.effect_scope(2);check(f.w.elemental_armor(other,60,60));
  if(mode==1)f.w.leave(1);else if(mode!=3)check(f.w.kill(f.first,1150));
  if(mode==2)check(f.w.settle_resources(other,4000));
  f.w.tick(1200);const auto ai=*f.w.npc_elemental_ai_snapshot(f.source);
  if(mode<2){check(ai.engaged&&!ai.returning&&ai.target==other&&ai.pending.has_value());
   check(f.w.npc_energy_snapshot(f.source)->current==45);}
  else {check(!ai.engaged&&!ai.pending&&f.w.npc_energy_snapshot(f.source)->current==50);
   if(mode==2)check(ai.returning);
   else {check(!ai.returning); // return reaches home before any reacquisition
    f.w.tick(1300);const auto reacquired=*f.w.npc_elemental_ai_snapshot(f.source);
    check(reacquired.engaged&&!reacquired.returning&&reacquired.target==other&&reacquired.pending.has_value());
    check(f.w.npc_energy_snapshot(f.source)->current==45);}}
 }
 {Fixture f;f.w.leave(2);check(f.w.settle_resources(f.first,4000)&&f.start({194}));f.w.tick(1000);
  check(!f.w.npc_elemental_ai_snapshot(f.source)->engaged&&f.w.npc_energy_snapshot(f.source)->current==50);}
 {Fixture f;check(f.start({194}));const auto end=f.launch();check(f.w.kill(f.first,end+100));
  f.w.tick(end+100);const auto switched=*f.w.npc_elemental_ai_snapshot(f.source);
  check(switched.engaged&&!switched.returning&&switched.target==f.second&&switched.flights==1);
  f.w.tick(f.arrival(end));check(f.w.resources(2)->health==1000); // old shot cannot adopt new target
  check(f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
 }
 for(unsigned skill:{183u,186u,194u,191u,214u})for(unsigned mode=0;mode<3;++mode){
  Fixture f(true,mode==2?10:500);check(f.w.spiteful_spirit(f.first,f.source,*skills::effects::spiteful_spirit(12),20,100));
  check(f.zero_overcast()&&f.start({skill}));const auto end=f.begin();
  if(mode==1)check(f.w.interrupt_skill(f.second,f.source,*skills::effects::interrupt_skill(5,12),20,1100));
  const auto before=f.w.npc_combat_snapshot(f.source)->health->current;f.box->drain();f.w.tick(end);
  check(f.w.npc_combat_snapshot(f.source)->health->current==(mode==1?before:mode==2?0.:471.));
  auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,100,121))==unsigned(mode!=1));
  f.w.tick(end);check(f.w.npc_combat_snapshot(f.source)->health->current==(mode==1?before:mode==2?0.:471.));
  if(mode!=1){
   const auto target_health=f.w.resources(1)->health;f.w.tick(end+100);
   check(f.w.npc_combat_snapshot(f.source)->health->current==(mode==2?0.:471.));
   check(f.w.resources(1)->health<=target_health);
   packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,100,121))==0);
   check(f.w.take_npc_deaths(f.first).size()==unsigned(mode==2));
   if(mode==2){f.w.tick(end+2000);check(f.w.resources(1)->health<1000);
    check(f.w.npc_combat_snapshot(f.source)->health->current==0&&f.w.take_npc_deaths(f.first).empty());}
  }
 }
 {Fixture f(true,500,1000);f.w.leave(2);check(f.zero_overcast()&&f.start({214}));const auto end=f.launch();
  f.w.tick(end+750);check(f.w.npc_elemental_ai_snapshot(f.source)->flights==1&&f.w.npc_elemental_ai_snapshot(f.source)->pending.has_value()&&f.w.npc_energy_snapshot(f.source)->current==40);
  f.w.tick(end+850);check(f.w.resources(1)->health==950&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0&&f.w.npc_elemental_ai_snapshot(f.source)->pending.has_value());
 }
 {Fixture f;check(f.zero_overcast()&&f.start({214}));check(f.w.status_mutation(f.first,8,1,15,1000,91000,true));
  const auto end=f.launch();f.w.tick(end+100);check(f.w.resources(1)->health==975&&f.w.resources(1)->energy==f.w.resources(2)->energy+2);
 }
 {Fixture f;check(f.zero_overcast()&&f.start({214}));const auto end=f.launch();check(f.w.settle_resources(f.first,end+200));f.w.tick(end+100);
   check(f.w.resources(1)->health==950&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);f.w.tick(end+200);check(f.w.resources(1)->health==950);
 }
 {Fixture f;check(f.zero_overcast()&&f.start({214}));const auto end=f.launch();
  check(f.w.damage_npc(f.second,f.source,{combat::Kind::armor_ignoring,1000,20,194},1248,end+1).killed);
  f.w.tick(end+100);check(f.w.resources(1)->health==950&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
 }
 {Fixture f;check(f.start({214}));f.w.tick(1000);check(!f.w.npc_elemental_ai_snapshot(f.source)->pending&&f.w.npc_energy_snapshot(f.source)->current==50);
  check(f.zero_overcast(1200));f.w.tick(1200);const auto end=f.pending_end();check(f.w.npc_energy_snapshot(f.source)->current==45);
  f.box->drain();f.w.tick(end);check(f.w.npc_elemental_ai_snapshot(f.source)->flights==1&&f.w.resources(1)->health==1000);
  const auto packets=f.box->drain();check(std::any_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==29&&p[0]==163&&value(p,2)==100&&value(p,20)==379;}));
  f.w.tick(end+100);check(f.w.resources(1)->health==950&&f.w.resources(2)->health==1000&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
  f.w.tick(end+100);check(f.w.resources(1)->health==950); // no replay
 }
 {Fixture f;check(f.start({214,194}));f.w.tick(1000);check(f.w.npc_elemental_ai_snapshot(f.source)->pending->use.slot.skill==194);}
 {Fixture f;check(f.zero_overcast());check(f.w.elemental_armor(f.first,60,{}));check(f.start({214,194}));f.w.tick(1000);
  check(f.w.npc_elemental_ai_snapshot(f.source)->pending->use.slot.skill==194); // missing cold armor falls back before payment
 }
 {Fixture f;check(f.zero_overcast()&&f.start({214}));const auto end=f.begin();
  check(f.w.damage_npc(f.second,f.source,{combat::Kind::armor_ignoring,100,20,194},1248,1100).reason==combat::Reason::accepted);
  check(f.w.observe_npc_overcast(f.source,2,1100,"authored mid-cast Overcast change"));check(!f.zero_overcast(1099));f.w.tick(end);
  check(!f.w.npc_elemental_ai_snapshot(f.source)->pending&&f.w.npc_elemental_ai_snapshot(f.source)->flights==1&&f.w.npc_energy_snapshot(f.source)->current==45);
  f.w.tick(end+100);check(f.w.resources(1)->health==950);f.w.leave(1);f.w.leave(2);
  f.w.tick(end+1000);check(f.w.npc_combat_snapshot(f.source)->health->current==406);
  f.w.tick(end+6000);check(f.w.npc_combat_snapshot(f.source)->health->current==430);
 }
 {Fixture f;check(f.zero_overcast()&&f.start({214}));const auto end=f.begin();check(f.zero_overcast(end+100));f.w.tick(end);
  check(f.w.npc_elemental_ai_snapshot(f.source)->pending.has_value());f.w.tick(end+100);check(f.w.npc_elemental_ai_snapshot(f.source)->flights==1);
 }
 {Fixture f;check(f.zero_overcast()&&f.start({214}));const auto end=f.launch();check(f.w.elemental_armor(f.first,60,{}));f.w.tick(end+100);
  check(f.w.npc_elemental_ai_snapshot(f.source)->flights==1&&f.w.resources(1)->health==1000);
  check(f.w.elemental_armor(f.first,60,60));f.w.tick(end+200);check(f.w.resources(1)->health==950&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
 }
 {Fixture f(true,40);check(f.w.observe_npc_overcast(f.source,2,1000,"authored positive Overcast")&&f.start({214}));const auto end=f.begin();
  check(f.w.backfire(f.second,f.source,*skills::effects::backfire(28,12),20,1100));f.box->drain();f.w.tick(end);
  check(f.w.npc_combat_snapshot(f.source)->health->current==0&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
  const auto packets=f.box->drain();check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==29&&p[0]==163;}));
  check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()>=8&&p[0]==65&&value(p,2)==100&&p[6]==214&&p[7]==0;}));
 }
 {Fixture f;check(f.zero_overcast()&&f.start({214}));const auto end=f.begin();check(f.w.kill(f.first,1200));f.w.tick(end);
  check(!f.w.npc_elemental_ai_snapshot(f.source)->pending&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0&&f.w.npc_energy_snapshot(f.source)->current==45);
  f.w.tick(end+100);f.w.tick(end+200);check(f.w.npc_elemental_ai_snapshot(f.source)->pending.has_value());
 }
 {Fixture f;check(f.zero_overcast()&&f.start({214}));const auto end=f.begin();
  check(f.w.interrupt_skill(f.second,f.source,*skills::effects::interrupt_skill(5,12),20,1100));f.w.tick(end);
  check(!f.w.npc_elemental_ai_snapshot(f.source)->pending&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
 }
 {Fixture f;check(f.zero_overcast()&&f.start({214}));const auto end=f.launch();const auto retired=f.source;f.renew_population(end+1);
  check(!f.w.observe_npc_overcast(retired,0,end+1,"retired authority")&&f.start({214},4,end+1));f.w.tick(end+100);
  check(f.w.resources(1)->health==1000&&!f.w.npc_elemental_ai_snapshot(f.source)->pending); // neither old shot nor old zero observation survives
 }
 for(const unsigned skill:{191u,194u}){Fixture f;check(f.start({skill}));const auto end=f.begin();
  const auto paid=f.w.npc_energy_snapshot(f.source)->current;
  check(f.w.backfire(f.second,f.source,*skills::effects::backfire(28,12),20,1100));
  check(f.w.kill(f.first,1200));f.box->drain();f.w.tick(end);
  check(!f.w.npc_elemental_ai_snapshot(f.source)->pending&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
  check(f.w.npc_energy_snapshot(f.source)->current==paid&&f.w.npc_combat_snapshot(f.source)->health->current==500);
  const auto packets=f.box->drain();
  check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==14&&p[0]==158&&value(p,2)==59&&value(p,6)==100;})==1);
  check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==14&&p[0]==158&&value(p,2)==58&&value(p,6)==100;}));
  f.w.tick(end+100);f.w.tick(end+200);check(f.w.npc_elemental_ai_snapshot(f.source)->pending.has_value()); // real 100ms return/acquire ticks, before recharge or aftercast
  check(f.w.npc_energy_snapshot(f.source)->current==paid-skills::find_skill(skill)->energy_cost());
 }
 {Fixture f;check(f.start({191}));const auto end=f.begin();check(f.w.settle_resources(f.first,end+100));
  f.w.tick(end);check(f.w.npc_elemental_ai_snapshot(f.source)->pending.has_value()&&f.w.npc_energy_snapshot(f.source)->current==40);
  check(f.w.kill(f.first,end+100));f.w.tick(end+100);check(!f.w.npc_elemental_ai_snapshot(f.source)->pending);
 }
 for(const bool replaced:{false,true}){Fixture f;check(f.start({191}));const auto end=f.begin();f.w.leave(1);
  if(replaced){f.w.join(1,"Replacement",100,0,f.box,0,{}, {},1200,3);f.w.set_vitals(1,{1000,1000,0,100},100);}
  f.w.tick(end);check(!f.w.npc_elemental_ai_snapshot(f.source)->pending&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
  check(f.w.npc_energy_snapshot(f.source)->current==40&&f.w.resources(2)->health==1000);
  if(replaced)check(f.w.resources(1)->health==1000);
 }
 for(const bool flight:{false,true}){Fixture f;check(f.w.configure_npc_flare_ai(f.policy(),1000));f.w.tick(1000);
  const auto end=f.w.npc_flare_ai_snapshot(f.source)->cast->completes;
  if(flight)f.w.tick(end);
  check(!f.w.configure_npc_flare_ai(f.policy(),flight?end:1100));
  const auto retired=f.source;const auto renewed=flight?end+1:1100;f.renew_population(renewed);
  check(f.w.configure_npc_flare_ai(f.policy(),renewed)&&!f.w.npc_flare_ai_snapshot(retired));
  f.w.tick(f.arrival(end));check(f.w.resources(1)->health==1000&&f.w.resources(2)->health==1000);
  const auto next=f.w.npc_flare_ai_snapshot(f.source)->cast->completes;f.w.tick(next);f.w.tick(f.arrival(next));
  check(f.w.resources(1)->health<1000);
 }
 {Fixture f;check(f.start());check(f.w.status_mutation(f.second,7,1,15,1000,91000,true));
  const auto released=f.launch(),arrives=f.arrival(released);
  check(f.w.npc_elemental_ai_snapshot(f.source)->flights==1&&f.w.npc_energy_snapshot(f.source)->current==40);
  const auto next=released+std::uint64_t(std::ceil(skills::find_skill(186)->aftercast*1000));
  check(next<arrives);f.w.tick(next);const auto overlap=*f.w.npc_elemental_ai_snapshot(f.source);
  check(overlap.flights==1&&overlap.pending&&overlap.pending->use.slot.skill==194&&f.w.npc_energy_snapshot(f.source)->current==35);
  const auto flare_end=overlap.pending->token.completes;
  f.w.tick(arrives);check(f.w.resources(1)->health<1000&&f.w.resources(2)->health>f.w.resources(1)->health&&f.w.resources(2)->health<1000);
  check(f.w.resources(2)->energy>f.w.resources(1)->energy);
  f.w.tick(flare_end);check(f.w.npc_elemental_ai_snapshot(f.source)->flights==1);
  const auto secondary=f.w.resources(2)->health;f.w.tick(f.arrival(flare_end));check(f.w.resources(2)->health==secondary);
  const auto wire=f.box->drain();check(std::any_of(wire.begin(),wire.end(),caster332));
  check(std::count_if(wire.begin(),wire.end(),[](const auto& p){return impact(p,1,344);})>=2);
  check(std::any_of(wire.begin(),wire.end(),[](const auto& p){return impact(p,2,344);}));
 }
 {Fixture f;check(f.start({191}));const auto end=f.launch();
  check(f.w.has_condition(f.first,combat::conditions::Kind::burning,end)&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
  const auto health=f.w.resources(1)->health;check(health<1000&&f.w.resources(2)->health==1000&&f.w.npc_energy_snapshot(f.source)->current==40);
  f.w.tick(end+1000);check(f.w.resources(1)->health<health);
  const auto wire=f.box->drain();check(std::any_of(wire.begin(),wire.end(),[](const auto& p){return impact(p,1,339);}));
 }
 {Fixture f(false);check(f.start({186}));check(f.w.status_mutation(f.first,7,1,15,1000,91000,true));
  const auto arrives=f.arrival(f.launch());f.w.tick(arrives);
  check(f.w.resources(1)->health==1000&&f.w.resources(2)->health==1000&&f.w.npc_elemental_ai_snapshot(f.source)->flights==1);
  check(f.w.resources(1)->energy==f.w.resources(2)->energy); // no Mantra reward before whole-group preflight
  check(f.w.elemental_armor(f.second,60,60));f.w.tick(arrives+100);
  check(f.w.resources(1)->health<1000&&f.w.resources(2)->health<1000&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
  check(f.w.resources(1)->energy>f.w.resources(2)->energy);
 }
 {Fixture f;check(f.start({186,194},1));const auto released=f.launch();
  f.w.tick(released+std::uint64_t(std::ceil(skills::find_skill(186)->aftercast*1000)));
  check(!f.w.npc_elemental_ai_snapshot(f.source)->pending&&f.w.npc_energy_snapshot(f.source)->current==40);
  f.w.tick(f.arrival(released));check(f.w.npc_elemental_ai_snapshot(f.source)->pending.has_value()&&f.w.npc_energy_snapshot(f.source)->current==35);
 }
 {Fixture f(true,40);check(f.start({186}));const auto end=f.begin();
  check(f.w.backfire(f.first,f.source,*skills::effects::backfire(28,12),20,1000+(end-1000)/2));
  f.w.tick(end);check(f.w.npc_combat_snapshot(f.source)->health->current==0&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
  f.w.tick(f.arrival(end));check(f.w.resources(1)->health==1000&&f.w.resources(2)->health==1000);
  const auto wire=f.box->drain();check(!std::any_of(wire.begin(),wire.end(),caster332));
 }
 {Fixture f;check(f.start({186}));const auto arrives=f.arrival(f.launch());check(f.w.settle_resources(f.second,arrives+1000));
  check(f.w.npc_elemental_ai_snapshot(f.source)->flights==0&&f.w.resources(1)->health<1000&&f.w.resources(2)->health<1000);
  const auto health=f.w.resources(1)->health;f.w.tick(arrives);check(f.w.npc_elemental_ai_snapshot(f.source)->flights==0&&f.w.resources(1)->health==health);
  f.w.tick(arrives+1000);check(f.w.npc_elemental_ai_snapshot(f.source)->flights==0&&f.w.resources(1)->health<1000&&f.w.resources(2)->health<1000);
 }
 {Fixture f;check(f.start({186}));const auto released=f.launch();
  f.w.join(3,"Far",10000,0,std::make_shared<world::Outbox>(),0,{}, {},100,3);f.w.set_vitals(3,{1000,1000,0,100},100);
  check(f.w.settle_resources(*f.w.effect_scope(3),f.arrival(released)+1000));
  f.w.tick(f.arrival(released));check(f.w.npc_elemental_ai_snapshot(f.source)->flights==0&&f.w.resources(1)->health<1000&&f.w.resources(2)->health<1000);
  check(f.w.resources(3)->health==1000); // queryable outside-radius future Health/Energy cannot block blast
 }
 {Fixture f;check(f.start({186}));const auto released=f.launch();
  check(f.w.damage_npc(f.first,f.source,{combat::Kind::armor_ignoring,1000,20,194},1248,released+100).killed);
  f.w.tick(f.arrival(released));check(f.w.resources(1)->health<1000&&f.w.resources(2)->health<1000);
 }
 {Fixture f;f.w.set_vitals(1,{1,1000,0,100},100);check(f.start({191}));const auto end=f.launch();
  check(f.w.resources(1)->health==0&&!f.w.has_condition(f.first,combat::conditions::Kind::burning,end));
 }
 {Fixture f;check(f.start({186}));const auto end=f.begin();
  check(f.w.kill(f.first,1000+(end-1000)/2));f.w.tick(end);
  check(f.w.npc_elemental_ai_snapshot(f.source)->flights==1);f.w.tick(f.arrival(end));
  check(f.w.resources(1)->health==0&&f.w.resources(2)->health<1000&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
 }
 {Fixture f;check(f.start({186}));const auto end=f.launch();check(f.w.kill(f.first,end+100));
  f.w.tick(f.arrival(end));check(f.w.resources(1)->health==0&&f.w.resources(2)->health<1000&&f.w.npc_elemental_ai_snapshot(f.source)->flights==0);
 }
 {Fixture f;check(f.start());check(!f.w.configure_npc_flare_ai(f.policy(),1000));}
 {Fixture f;check(f.w.configure_npc_flare_ai(f.policy(),1000));check(!f.start());}
 {Fixture f;check(f.start({186}));const auto released=f.launch();check(f.w.npc_elemental_ai_snapshot(f.source)->flights==1);
  const auto retired=f.source,old_target=f.first;const auto configured=released+100;f.renew_population(configured);check(f.source!=retired&&f.first!=old_target);
  auto wrong=f.policy();++wrong.level;check(!f.w.configure_npc_elemental_ai(wrong,loadout({194}),4,configured));
  check(f.start({194},4,configured)&&!f.w.npc_elemental_ai_snapshot(retired)&&!f.start({194},4,configured));
  f.w.tick(f.arrival(released));check(f.w.resources(1)->health==1000&&f.w.resources(2)->health==1000);
  const auto end=f.pending_end();f.w.tick(end);f.w.tick(f.arrival(end));check(f.w.resources(1)->health<1000&&f.w.resources(2)->health==1000);
 }
 {Fixture f;check(f.w.configure_npc_flare_ai(f.policy(),1000));f.w.tick(1000);
  const auto retired=f.source;f.renew_population(1100);
  check(f.start({191},4,1100)&&!f.w.npc_flare_ai_snapshot(retired));
  f.w.tick(2000);const auto end=f.pending_end();f.w.tick(end);check(f.w.has_condition(f.first,combat::conditions::Kind::burning,end));
 }
 std::cout<<"PASS autonomous multi-skill World payment,overlapping flights,Fireball mixed protection,Immolate registry,Burning/death,capacity,future clock and captured visuals\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
