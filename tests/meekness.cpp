#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Meekness line "+std::to_string(at.line()));
}
world::Instance::Npc make_meekness_npc(unsigned id,float x){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
 d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
 d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=0;n.combat_definition=d;return n;
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=effects::attack_slow_hex(1260,rank);
  check(p&&p->skill==1260&&p->pips==0&&p->slower_percent==50&&p->radius==322&&p->duration==std::uint64_t(effects::interpolate(5,30,rank))*1000);}
 check(!effects::attack_slow_hex(1260,21)&&casting::Runtime::effect_supported(1260));
 {world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.configure_npcs({make_meekness_npc(100,422),make_meekness_npc(101,422.1f)});
  for(unsigned id=1;id<=6;++id){const float x=id==1?0:id==2?100:id==3?422:id==4?422.1f:150;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==5?1:id);
   w.set_vitals(id,{1000,1000,100,100},100);}
  const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.kill(*w.effect_scope(6),100));
  const auto plan=*effects::attack_slow_hex(1260,0);auto forged=plan;forged.radius=252;
  check(!w.faintheartedness(a,b,forged,100)&&!w.faintheartedness(a,a,plan,100));
  check(w.faintheartedness(a,b,plan,100));
  for(unsigned id:{2u,3u})check(w.attack_duration(*w.effect_scope(id),1000,100)==1500);
  for(unsigned id:{1u,4u,5u})check(w.attack_duration(*w.effect_scope(id),1000,100)==1000);
  check(w.npc_combat_snapshot(*w.effect_scope(100))->effects&0x800);
  check(!(w.npc_combat_snapshot(*w.effect_scope(101))->effects&0x800));
  check(w.attack_duration(*w.effect_scope(100),1000,100)==1500&&w.attack_duration(*w.effect_scope(100),1000,100,true)==1500);
  const auto packets=box->drain();
  for(unsigned id:{2u,3u,100u})check(std::count(packets.begin(),packets.end(),particles::shadow_of_fear_target_2022(id,1))==1);
  check(w.attack_duration(b,1000,5099)==1500&&w.attack_duration(b,1000,5100)==1000);
  w.tick(5100);check(w.resources(2)->health==1000&&w.resources(3)->health==1000&&w.npc_combat_snapshot(*w.effect_scope(100))->health->current==1000);
  check(w.faintheartedness(a,b,*effects::attack_slow_hex(1260,12),5200));
  const auto entries=w.status_snapshot(b,5200);check(bool(entries));unsigned effect=0;for(const auto&e:*entries)if(e.skill==1260)effect=e.instance;check(effect);
  check(w.status_mutation(b,1260,effect,12,5300,std::nullopt,false)&&w.attack_duration(b,1000,5300)==1000);
 }
 // NPC cadence uses the same Registry-backed duration consumer as player
 // ordinary/skill attacks; zero-pip Meekness never creates degeneration.
 {auto n=make_meekness_npc(100,0);combat::npcs::LiveActor live(*n.combat_definition,{1,1,1,146,100});const auto life=live.lifetime();
  check(bool(live.apply_faintheartedness(life,*effects::attack_slow_hex(1260,0),1,1,100)));
  check(live.attack_duration(life,2000,100)==3000&&live.attack_duration(life,2000,5100)==2000);
  check(live.advance_conditions(life,5100)->after.current==1000);
 }
 for(bool npc_target:{false,true})for(unsigned mode=0;mode<9;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();const unsigned target=npc_target?100:2;
  if(npc_target)w.configure_npcs({make_meekness_npc(100,60)});
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{mode==8?10u:1000u,1000,100,100},100);
  if(!npc_target){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1);casting::Hooks h;unsigned commits=0,sacrifices=0;std::vector<world::Packet>sent;
  h.events=[&](const auto&p){sent.insert(sent.end(),p.begin(),p.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){check(cost==15);return w.consume_energy(1,cost,source,now);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return mode!=6&&id==target&&range==1248;};
  h.faintheartedness=[&](auto scope,const auto&plan,auto now){++commits;check(plan==*effects::attack_slow_hex(1260,12)&&now==2100);return w.faintheartedness(source,scope,plan,now);};
  h.health_sacrifice=[&](auto skill,auto now){++sacrifices;check(skill==1260);return w.health_sacrifice(source,skill,now);};
  if(mode==4)h.faintheartedness={};if(mode==5)h.health_sacrifice={};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[1260]=1;Slots slots{};slots[0]={1260,0};attributes::Ranks ranks{};ranks[7]=12;casting::Runtime runtime(1);
  const auto expected=(mode==4||mode==5)?casting::Reason::unsupported:mode==6?casting::Reason::target:casting::Reason::accepted;
  check(runtime.start(100,{1260,0,target},slots,policy,ranks,true,h)==expected);
  if(mode==4||mode==5||mode==6){check(w.resources(1)->energy==100&&!commits&&!sacrifices);continue;}
  check(w.resources(1)->energy==85);runtime.update(2099,h);check(!commits&&!sacrifices);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(2099,h);
  if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},2100,1);w.set_vitals(1,{1000,1000,100,100},100);}
  if(mode==7)h.target_scope=[&](auto id)->std::optional<effects::Scope>{auto scope=w.effect_scope(id);if(scope&&id==target)++scope->generation;return scope;};
  runtime.update(2100,h);
  if(mode==1||mode==2||mode==3||mode==7){check(!sacrifices);continue;}
  check(commits==1&&sacrifices==1&&w.resources(1)->health==(mode==8?0u:830u));
  check(std::count(sent.begin(),sent.end(),casting::generic(21,1,246))==1);
  check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{1260,0},15))==1);
  if(!npc_target)check(w.attack_duration(*w.effect_scope(target),1000,2100)==1500);
  if(mode==0)check(runtime.start(2850,{1260,0,target},slots,policy,ranks,true,h)==casting::Reason::recharge);
 }
 // Production-style deferred publication preserves both legacy family skills
 // and places native finish/caster feedback before target effects. Meekness's
 // actual sacrifice commits between the prefix and recipient publication.
 for(unsigned skill:{135u,136u,1260u})for(bool npc_target:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();const unsigned target=npc_target?100:2;
  if(npc_target)w.configure_npcs({make_meekness_npc(100,60)});
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc_target){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1);const auto plan=*effects::attack_slow_hex(skill,12);
  const auto target_fx=skill==135?*particles::targeted_body_2022(target,1,skill):particles::shadow_of_fear_target_2022(target,1);
  const auto finish=casting::terminal(58,1);std::vector<world::Packet> observed;
  casting::Hooks h;h.events=[&](const auto&packets){box->enqueue(packets);};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto cost,auto now){return w.consume_energy(1,cost,source,now);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.projectile_target_valid=[&](auto id,float range){return id==target&&range==1248;};
  h.faintheartedness=[&](auto scope,const auto&p,auto now){std::function<void()> publication;
   const bool accepted=w.faintheartedness(source,scope,p,now,&publication);return casting::EffectCompletion{accepted,{}, {},std::move(publication)};};
  h.health_sacrifice=[&](auto id,auto now){const auto prefix=box->drain();observed.insert(observed.end(),prefix.begin(),prefix.end());
   check(std::count(prefix.begin(),prefix.end(),finish)==1&&std::count(prefix.begin(),prefix.end(),casting::generic(21,1,246))==1);
   check(std::count(prefix.begin(),prefix.end(),target_fx)==0);return w.health_sacrifice(source,id,now);};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;Slots slots{};slots[0]={skill,0};attributes::Ranks ranks{};ranks[7]=12;casting::Runtime runtime(1);
  check(runtime.start(100,{skill,0,target},slots,policy,ranks,true,h)==casting::Reason::accepted);box->drain();
  runtime.update(skill==135?1100:2100,h);const auto tail=box->drain();observed.insert(observed.end(),tail.begin(),tail.end());
  check(std::count(observed.begin(),observed.end(),finish)==1&&std::count(observed.begin(),observed.end(),target_fx)==1);
  check(std::find(observed.begin(),observed.end(),finish)<std::find(observed.begin(),observed.end(),target_fx));
  if(skill!=135){const auto caster_fx=casting::generic(21,1,246);check(std::count(observed.begin(),observed.end(),caster_fx)==1);
   check(std::find(observed.begin(),observed.end(),finish)<std::find(observed.begin(),observed.end(),caster_fx));
   check(std::find(observed.begin(),observed.end(),caster_fx)<std::find(observed.begin(),observed.end(),target_fx));}
  check(w.resources(1)->health==(skill==1260?830u:1000u));
  check(w.attack_duration(*w.effect_scope(target),1000,skill==135?1100:2100)==1500);
 }
 std::cout<<"Meekness shared attack-slow family, player/NPC area, expiry/removal, native Runtime costs, guards and135/136/1260 completion chronology passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

