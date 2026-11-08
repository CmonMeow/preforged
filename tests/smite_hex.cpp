#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Smite Hex line "+std::to_string(at.line()));}
int main()try{
 check(smite_hex(0)->damage==10&&smite_hex(12)->damage==70&&smite_hex(15)->damage==85&&smite_hex(20)->damage==110);
 check(!smite_hex(21)&&!smite_hex(12,21)&&smite_hex(12)->radius==322);
 for(unsigned skill:{67u,302u})for(unsigned permission=0;permission<3;++permission){
  world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?0:100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
   d.relation=id==100?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
   d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;
   d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;d.healing_allowed=true;
   if(id==101){if(permission==1)d.healing_allowed=false;else if(permission==2)d.healing_allowed.reset();}
   d.healing_evidence="authored allied removal fixture";d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="authored normal sensitivity";
   n.combat_definition=d;npcs.push_back(n);}
  w.configure_npcs(npcs);auto box=std::make_shared<world::Outbox>();
  w.join(1,"Monk",50,0,box,0,{{165,0,1,0,0,0,3,0}},{},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),foe=*w.effect_scope(100),target=*w.effect_scope(101);
  check(w.configure_npc_spellbook(foe,{{{31,0},1248,"authored hostile cast"}}));
  check(w.begin_npc_phantasm(foe,target,12,100).has_value());w.tick(1100);
  check(w.status_snapshot(target,1100)->size()==1);
  check(w.stage_npc_shield_absorption(target,12,1100));
  const auto plan=skill==302?*smite_hex(12,12):*shatter_hex(12);
  box->drain();const bool ok=w.shatter_delusions(source,target,plan,20,1100);
  check(ok==(skill!=302||permission!=2));
  const auto gain=skill==302&&permission==0?38:0;
  check(w.npc_combat_snapshot(target)->health->current==500+gain);
  const auto status=w.status_snapshot(target,1100);check(status&&status->size()==(ok?1:2));
  check(w.npc_combat_snapshot(foe)->health->current==500-(ok?plan.damage:0));
  if(!ok)continue;
  const auto packets=box->drain();
  check(std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67;})==1);
  const auto feedback=particles::health_change_2022(target.actor,source.actor,38,1000);
  check(std::count(packets.begin(),packets.end(),feedback)==(gain?1:0));
  // No remaining hex: valid ally spell still grants DF, never repeats collateral.
  check(w.shatter_delusions(source,target,plan,20,1100));
  check(w.npc_combat_snapshot(target)->health->current==500+2*gain);
  check(w.npc_combat_snapshot(foe)->health->current==500-plan.damage);
  box->drain();w.tick(2100);
  check(w.npc_combat_snapshot(target)->health->current==500+2*gain);
  const auto later=box->drain();
  check(std::none_of(later.begin(),later.end(),[](const auto& p){return p.size()==10&&p[0]==67;}));
 }
 for(unsigned mode=0;mode<8;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();Packet profession{165,0};world::u32(profession,1);profession.push_back(3);profession.push_back(0);
  for(unsigned id=1;id<=5;++id){float x=id==1?0:id==2?100:id==3?422:id==4?422.1f:100;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,id==1?std::vector<Packet>{profession}:std::vector<Packet>{},{},100,id<=2?1:2);
   w.set_vitals(id,{id==2?500u:1000u,1000,100,100},100);check(w.player_holy_equipment(*w.effect_scope(id),mode==4&&id==3?std::nullopt:std::optional<unsigned>(mode==7&&id==3?6:0)));
  }
  const auto source=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);
  if(mode!=1&&mode!=2)check(w.degeneration_hex(foe,ally,*degeneration_hex(129,0),100));
  const auto plan=*smite_hex(12,12);auto bad=plan;bad.holy=false;check(!w.shatter_delusions(source,ally,bad,20,100));
  if(mode==3){auto stale=ally;++stale.generation;check(!w.shatter_delusions(source,stale,plan,20,100));continue;}
  check(!w.shatter_delusions(source,foe,plan,20,100));
  if(mode==4){check(!w.shatter_delusions(source,ally,plan,20,100));check(w.status_snapshot(ally,100)->size()==1&&w.resources(2)->health==500);continue;}
  const auto now=mode==2?20000u:100u;
  if(mode==5){check(w.wound_spell(foe,ally,*wound_spell(44,12),20,100));}
  if(mode==6){check(w.degeneration_hex(foe,ally,*degeneration_hex(31,0),100));}
  box->drain();check(w.shatter_delusions(source,ally,plan,20,now));
  check(w.resources(3)->health==((mode==1||mode==2)?1000u:mode==7?924u:930u));
  check(w.resources(4)->health==1000&&w.resources(5)->health==((mode==1||mode==2)?1000u:930u));
  if(mode==0)check(w.resources(2)->health==538&&w.status_snapshot(ally,now)->empty());
  if(mode==1)check(w.resources(2)->health==538);
  if(mode==6)check(w.status_snapshot(ally,now)->size()==1); // newest cover removed,old Defile modifies healing.
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::target_body_2022(2,302))==1);
 }
 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{1000,1000,100,100},100);check(w.player_holy_equipment(*w.effect_scope(id),0));}
  const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);
  check(w.degeneration_hex(foe,owner,*degeneration_hex(129,0),100));
  casting::Hooks h;std::vector<Packet> wire;h.events=[&](const auto& batch){wire.insert(wire.end(),batch.begin(),batch.end());};
  h.resources=[&](auto id){return w.resources(id);};h.consume_energy_at=[&](auto cost,auto now){return w.consume_energy(1,cost,owner,now);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.shatter_ally_valid=[&](auto id,float range,auto now){return w.valid_shatter_ally(owner,id,range,now);};
  h.shatter_delusions=[&](auto target,const auto& plan,auto now){return w.shatter_delusions(owner,target,plan,20,now);};
  Policy p;p.primary=3;p.campaigns=255;p.learned[302]=1;Slots bar{};bar[0]={302,0};attributes::Ranks ranks{};ranks[14]=12;
  casting::Runtime cast(1);check(cast.start(100,{302,0,2},bar,p,ranks,true,h)==casting::Reason::target);
  if(mode==4)h.shatter_delusions={};
  check(cast.start(100,{302,0,1},bar,p,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(500,h);if(mode==3)h.shatter_delusions=[](auto,const auto&,auto){return false;};
  cast.update(1100,h);check(w.resources(2)->health==(mode?1000u:930u));
  if(!mode){check(w.resources(1)->energy==95);check(cast.start(2000,{302,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);
   check(std::count(wire.begin(),wire.end(),casting::terminal(58,1))==1);}
 }
 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u,102u}){world::Instance::Npc n;n.id=n.definition.id=id;n.definition.visual_adjustment={0,0,0,100};n.x=id==100?100:id==101?422:422.1f;
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.spawn_health=combat::Health{id==100?500.:1000.,1000};
   d.relation=id==100?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;d.damageable=id!=100;
   d.healing_allowed=mode!=3;if(mode==4)d.healing_allowed.reset();d.healing_evidence="test";d.holy_traits.light_sensitive=mode==1;
   if(mode==2&&id==101)d.holy_traits.light_sensitive.reset();d.light_sensitive_evidence="test";n.combat_definition=d;npcs.push_back(n);
  }
  w.configure_npcs(npcs);Packet profession{165,0};world::u32(profession,1);profession.push_back(3);profession.push_back(0);
  w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{profession},{},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,1000,100,100},100);
  w.join(3,"Foe",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),ally=*w.effect_scope(2);
  check(w.shatter_delusions(source,*w.effect_scope(100),*smite_hex(12,12),20,100)==(mode!=4));
  check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==(mode>=3?500:538));
  if(mode==4)continue;
  check(w.degeneration_hex(*w.effect_scope(3),ally,*degeneration_hex(129,0),100));
  const auto ok=w.shatter_delusions(source,ally,*smite_hex(12,12),20,100);check(ok==(mode!=2));
  check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==(mode==2?1000:mode==1?860:930));
  check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==1000);
  check(w.status_snapshot(ally,100)->size()==(mode==2?1u:0u));
 }
 std::cout<<"PASS Smite Hex removal/Holy area/DF,range boundary,unknown-trait rollback,cover,expiry,cast,cancel and recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
