#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Vapor Blade line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;Scope source,target;bool npc;
 Fixture(bool is_npc=false,bool armor=true):npc(is_npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
   d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;
   if(armor){d.cold_armor=60;d.cold_armor_evidence="fixture";}n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Elementalist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{2000,2000,50,100},100);
  check(w.energy_capacity(1,6,0,100,{80,-2}));source=*w.effect_scope(1);check(w.elemental_armor(source,60,60));
  if(!npc){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,50,100},100);if(armor)check(w.elemental_armor(*w.effect_scope(2),{},60));}
  target=*w.effect_scope(npc?100:2);
 }
 double health()const{return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 void enchant(std::uint64_t end=60100,std::uint64_t now=100){check(w.status_mutation(target,180,99,12,now,end,true));}
};
int main()try{
 for(unsigned rank:{0u,12u,20u})for(bool enchanted:{false,true}){
  auto plan=*elemental(866,rank);combat::Actor actor;actor.id=2;actor.generation=1;actor.health=combat::Health{1000,1000};actor.damageable=true;actor.cold_armor=60;
  const auto result=elemental_target(plan,actor,{},20,1,100,false,1,true,enchanted);
  const double damage=(rank==0?15.:rank==12?111.:175.)*(enchanted?.5:1.);
  check(result.accepted&&result.hits.size()==1&&result.hits[0].damage==damage&&result.actor.health->current==1000-damage);
  check(!elemental_target(plan,actor,{},20,1,100).accepted);
  actor.incoming_reduction=10;const auto reduced=elemental_target(plan,actor,{},20,1,100,false,1,true,enchanted);
  check(reduced.accepted&&reduced.hits[0].damage==std::max(0.,damage-10));
 }
 for(unsigned rank:{0u,12u,20u})for(unsigned count=0;count<4;++count){Fixture f;const auto plan=*elemental(866,rank);
  if(count)f.enchant(count==3?200:60100);if(count==2)check(f.w.status_mutation(f.target,38,100,12,100,60100,true));
  auto bad=plan;bad.damage+=1;check(!f.w.elemental(f.source,f.target,bad,20,200));
  std::vector<Packet> visuals;check(f.w.elemental(f.source,f.target,plan,20,200,&visuals));
  check(f.health()==std::ceil(1000-plan.damage*((count==1||count==2)?.5:1.)));
  check(std::count(visuals.begin(),visuals.end(),*particles::targeted_body_2022(2,1,866))==1);
 }
 for(bool armor:{false,true}){Fixture f(true,armor);const auto plan=*elemental(866,12);
  check(f.w.elemental(f.source,f.target,plan,20,100)==armor);check(f.health()==(armor?889:1000));}
 // Real NPC-owned enchantment produced by an actual reviewed AI cast.
 {Fixture f(true);check(f.w.observe_npc_overcast(f.target,2,1000,"fixture"));
  combat::ai::FlarePolicy ai;ai.source=f.target;ai.level=20;ai.fire_magic=12;ai.speed=300;ai.acquire=1000;ai.follow=ai.leash=1500;ai.flight={100,32};ai.evidence="fixture";
  namespace e=combat::ai::elemental;std::vector<e::Reviewed> book{{{214,0},e::Plan{*water_projectile(214,12)},"fixture"}};
  check(f.w.configure_npc_elemental_ai(ai,book,4,1000));f.w.tick(1000);const auto state=f.w.npc_elemental_ai_snapshot(f.target);check(state&&state->pending);
  const auto end=state->pending->token.completes;f.w.tick(end);check(f.w.npc_combat_snapshot(f.target)->effects&0x80);
  const auto before=f.health();check(f.w.elemental(f.source,f.target,*elemental(866,12),20,end));check(f.health()==before-55.5);
 }
 for(bool pvp:{false,true})for(unsigned mode=0;mode<10;++mode){Fixture f;
  if(mode==9)f.w.set_vitals(1,{2000,2000,9,100},100);
  if(mode==1)f.enchant();if(mode==3)f.enchant(2000);
  Policy policy;policy.primary=6;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[866]=1;Slots bar{};bar[0]={866,0};attributes::Ranks ranks{};ranks[11]=12;
  casting::Hooks h;std::vector<Packet> packets;unsigned payments=0;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return f.w.resources(id);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[](auto id,float){return id==2;};
  if(mode!=6)h.consume_overcast_energy=[&](auto skill,auto cost,auto event,auto at){++payments;check(skill==866&&cost==10&&event);return f.w.activate_overcast_energy(f.source,skill,cost,event,at);};
  h.elemental=[&](auto target,const auto& plan,auto at){std::vector<Packet> visuals;const bool accepted=f.w.elemental(f.source,target,plan,20,at,&visuals);return casting::EffectCompletion{accepted,std::move(visuals)};};
  if(mode==8)h.elemental={};
  casting::Runtime cast(1);check(cast.start(500,{866,0,2},bar,policy,ranks,true,h)==(mode==6||mode==8?casting::Reason::unsupported:mode==9?casting::Reason::energy:casting::Reason::accepted));
  if(mode==6||mode==8){check(!payments&&f.w.resources(1)->energy==50);continue;}
  if(mode==9){check(payments==1&&f.w.resources(1)->energy==9&&!f.w.overcast_energy_snapshot(f.source));continue;}
  check(payments==1&&f.w.resources(1)->energy==40&&f.w.overcast_energy_snapshot(f.source)->curve.burden_units()==500000);
  if(mode==2)f.enchant(60100,1000);if(mode==4)cast.cancel(h,1000);if(mode==5)cast.interrupt(1000,h);
  if(mode==7){f.w.leave(2);f.w.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{1000,1000,50,100},100);check(f.w.elemental_armor(*f.w.effect_scope(2),{},60));}
  cast.update(2000,h);const bool success=mode<4;check(f.health()==(success?(mode==1||mode==2?945:889):1000));
  check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,999))==unsigned(success));
  check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,866))==unsigned(success));
  if(success){const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   const auto caster_body=std::find(packets.begin(),packets.end(),combat::detail::integer(21,1,999));
   const auto target_body=std::find(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,866));
   check(finish<caster_body&&caster_body<target_body);}
  if(success){check(cast.start(8999,{866,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);check(cast.start(9000,{866,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);}
 }
 std::cout<<"PASS Vapor Blade one cold hit,owned completion enchantment count,precise halves/protection,NPC armor/owned214,atomic Energy/Overcast,PvE/PvP lifecycle and999/998\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
