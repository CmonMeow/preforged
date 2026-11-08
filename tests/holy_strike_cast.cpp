#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Holy Strike line "+std::to_string(at.line())+context);
}
int main()try{
 for(unsigned rank:{0u,12u,20u})for(bool light:{false,true})for(unsigned mode=0;mode<10;++mode){
  context=" rank="+std::to_string(rank)+" light="+std::to_string(light)+" mode="+std::to_string(mode);
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=mode==7?145.f:100.f;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{mode==9?5.:1000.,1000};d.damageable=true;d.conditions_allowed=true;
  d.knockdown_allowed=true;d.knockdown_evidence="authored fixture";
  if(mode!=6){d.holy_traits.light_sensitive=light;d.light_sensitive_evidence="authored holy trait";}
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  check(w.energy_capacity(1,1,0,100,{80,-2}));const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);
  const auto plan=*effects::holy_damage(312,rank);check(plan.range==144&&plan.damage==10+3*rank&&plan.knocked_down_bonus==plan.damage);
  const auto slices=effects::holy_strikes(plan,{light,{}},20,true);check(slices&&slices->size()==2&&slices->front().amount==plan.damage*(light?2:1));
  Slots bar{};bar[0]={312,0};Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[312]=1;attributes::Ranks ranks{};ranks[14]=rank;
  casting::Hooks h;std::vector<world::Packet> packets;bool stale=false;h.resources=[&](unsigned id){return w.resources(id);};
  h.target_scope=[&](unsigned id){if(stale&&id==100)return std::optional<effects::Scope>{};return w.effect_scope(id);};
  h.projectile_target_valid=[](unsigned id,float range){return id==100&&range==144;};
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.consume_energy_at=[&](unsigned cost,auto now){check(cost==5);return w.consume_energy(1,cost,owner,now);};
  h.holy_damage=[&](auto selected,const auto& value,auto now)->casting::EffectCompletion{box->drain();std::vector<world::Packet> visuals;
   const bool ok=w.holy_damage(owner,selected,value,20,now,&visuals);const auto early=box->drain();
   for(const auto& visual:visuals)check(std::find(early.begin(),early.end(),visual)==early.end());return {ok,std::move(visuals)};};
  using K=combat::npcs::control::Knockdown;
  if(mode==1||mode==2)check(w.knockdown_npc(target,mode==1?2000:500,1000)==K::accepted);
  casting::Runtime runtime(1);check(runtime.start(1000,{312,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted&&w.resources(1)->energy==95);
  if(mode==3)check(w.knockdown_npc(target,2000,1500)==K::accepted);
  if(mode==4)runtime.cancel(h);if(mode==5)runtime.interrupt(1100,h);if(mode==8)stale=true;
  runtime.update(1749,h);check(w.npc_combat_snapshot(target)->health->current==(mode==9?5:1000));runtime.update(1750,h);
  const bool success=mode<4||mode==9;const bool down=mode==1||mode==3;
  const double damage=plan.damage*(light?2:1)*(down?2:1);const double expected=mode==9?0:success?1000-damage:1000;
  check(w.npc_combat_snapshot(target)->health->current==expected);
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==unsigned(success));
  const auto body=*particles::targeted_body_2022(100,1,312);check(std::count(packets.begin(),packets.end(),body)==unsigned(success));
  if(success)check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),body));
  runtime.update(1750,h);check(w.npc_combat_snapshot(target)->health->current==expected);
 }
 auto forged=*effects::holy_damage(312);++forged.knocked_down_bonus;check(!effects::holy_strikes(forged,{false,{}},20,true));
 check(!effects::holy_damage(312,21)&&!effects::holy_strikes(*effects::holy_damage(312),{},20,false));
 std::cout<<"PASS Holy Strike ranks,known holy traits,touch range,completion-time knockdown/expiry,two slices,lethal first hit,cost,cancel/interrupt,stale identity,deferred556 and no replay\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
