#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Flame Djinn line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope owner,foe,npc;
 Fixture(unsigned hp=500,int armor=60,float distance=60){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=-distance;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health={double(hp),1000};d.damageable=true;d.fire_armor=armor;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:id==2?distance:20,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{id==2?hp:1000u,1000,100,100},100);}
  owner=*w.effect_scope(1);foe=*w.effect_scope(2);npc=*w.effect_scope(100);check(w.elemental_armor(foe,armor,60));
 }
};
int main()try{
 unsigned cases=0;
 for(unsigned rank=0;rank<=20;++rank)for(int armor:{60,100})for(int rr:{-1,0,12})for(unsigned hp:{10u,500u}){
  Fixture f(hp,armor);const auto plan=*effects::flame_djinn_haste(rank);
  if(rr>=0){check(f.w.status_mutation(f.foe,307,900,rr,100,10000,true));check(f.w.stage_npc_reversal(f.npc,rr,100));}
  f.box->drain();const auto result=f.w.flame_djinn_haste(f.owner,plan,20,100);check(result&&*result);
  const auto damage=*combat::amount({combat::Kind::fire,double(plan.damage),20,1381},armor);
  const auto converted=rr<0?0.:std::min(damage,double(effects::reversal_fortune(rr)->maximum_healing));
  const auto expected=std::max(0.,std::min(1000.,hp+converted)-(damage-converted));
  check(std::abs(double(f.w.resources(2)->health)-expected)<1.01&&std::abs(f.w.npc_combat_snapshot(f.npc)->health->current-expected)<.001);
  check(f.w.resources(3)->health==1000);const auto status=*f.w.status_snapshot(f.owner,100);check(status.size()==1&&status[0].skill==1381&&status[0].expires==100+plan.duration);
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,1638))==1&&std::count(packets.begin(),packets.end(),combat::detail::integer(58,1,0))==1);
  check(f.w.take_npc_deaths(f.owner).size()==unsigned(expected<=0));++cases;
 }
 check(cases==252);
 for(unsigned mode=0;mode<6;++mode){
  Fixture f(500,60,167);const auto plan=*effects::flame_djinn_haste(12);
  check(f.w.flame_djinn_haste(f.owner,plan,20,100)==std::optional(false));
  auto status=*f.w.status_snapshot(f.owner,100);const auto effect=status[0].instance;
  f.w.move(1,10000,0,100);f.w.tick(600);check(std::abs(f.w.position(1,600).first-180)<.3);
  if(mode==0){f.w.tick(100+plan.duration+1000);check(std::abs(f.w.position(1,100+plan.duration+1000).first-(288*1.25*plan.duration/1000+288))<.5);}
  if(mode==1){check(f.w.status_mutation(f.owner,1381,effect,12,600,{},false));f.w.tick(1100);check(std::abs(f.w.position(1,1100).first-324)<.3);}
  if(mode==2){
   const auto renewed=f.w.flame_djinn_haste(f.owner,plan,20,600);check(renewed.has_value());
   check(f.w.status_snapshot(f.owner,600)->size()==1);f.w.tick(1100);
   if(std::abs(f.w.position(1,1100).first-360)>=.3)std::cerr<<"renewed position "<<f.w.position(1,1100).first<<'\n';
   check(std::abs(f.w.position(1,1100).first-360)<.3);
  }
  if(mode==3){check(f.w.kill(f.owner,600));check(f.w.status_snapshot(f.owner,600)->empty());}
  if(mode==4){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},600,1);f.w.set_vitals(1,{1000,1000,100,100},100);check(!f.w.flame_djinn_haste(f.owner,plan,20,600));}
  if(mode==5){auto forged=plan;++forged.damage;check(!f.w.flame_djinn_haste(f.owner,forged,20,600));}
  ++cases;
 }
 // One unknown recipient blocks the selected wave and the new enchantment.
 {Fixture f;f.w.join(4,"Unknown armor",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(4,{500,1000,100,100},100);
  check(!f.w.flame_djinn_haste(f.owner,*effects::flame_djinn_haste(12),20,100));check(f.w.resources(2)->health==500&&f.w.status_snapshot(f.owner,100)->empty());++cases;}
 // Normal paid Runtime: current self scope, start payment, cancellation,
 // interrupt, recycle, reaction death and conditional 10/20s recharge.
 for(bool hit:{false,true})for(unsigned mode=0;mode<7;++mode){
  Fixture f(500,60,hit?60:600);skills::casting::Runtime runtime(1);skills::casting::Hooks h;unsigned commits=0,paid=0;std::vector<world::Packet> sent;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){sent.insert(sent.end(),p.begin(),p.end());};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.consume_energy_at=[&](unsigned cost,auto at){paid+=cost;return f.w.consume_energy(1,cost,f.owner,at);};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return f.w.before_spell_effect(f.owner,skill,event,at);};
  h.flame_djinn_haste=[&](auto scope,const auto& plan,auto event,auto at){++commits;return f.w.flame_djinn_haste(scope,plan,20,at,nullptr,event);};
  if(mode==4)h.flame_djinn_haste={};
  if(mode==5){f.w.set_vitals(1,{1,1000,100,100},100);check(f.w.backfire(f.foe,f.owner,*effects::backfire(28,12),20,100));}
  if(mode==6)f.w.set_vitals(1,{1000,1000,0,100},100);
  skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1381]=1;skills::Slots bar{};bar[0]={1381,0};attributes::Ranks ranks{};ranks[10]=12;
  const auto started=runtime.start(100,{1381,0,2},bar,policy,ranks,true,h);
  check(started==(mode==4?skills::casting::Reason::unsupported:mode==6?skills::casting::Reason::energy:skills::casting::Reason::accepted));
  if(mode==4||mode==6){check(!commits);++cases;continue;}check(paid==10);runtime.update(849,h);check(!commits);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(849,h);
  if(mode==3){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},849,1);f.w.set_vitals(1,{1000,1000,100,100},100);}
  f.box->drain();runtime.update(850,h);check(!runtime.active()&&commits==unsigned(mode==0));
  if(mode==0){const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,1,1638))==1);check(std::count(sent.begin(),sent.end(),skills::casting::terminal(58,1))==0);
   check(std::count(sent.begin(),sent.end(),skills::casting::slot_event(228,1,{1381,0},hit?10:20))==1);}
  if(mode==5)check(f.w.resources(1)->health==0);++cases;
 }
 check(cases==273);
 // Same-speed renewal must preserve history for another movement family too.
 {Fixture f(500,60,600);check(f.w.armor_earth(f.owner,0,100));f.w.move(1,10000,0,100);f.w.tick(600);check(std::abs(f.w.position(1,600).first-72)<.3);check(f.w.armor_earth(f.owner,0,600));f.w.tick(1100);check(std::abs(f.w.position(1,1100).first-144)<.3);++cases;}
 for(unsigned rank:{0u,12u,20u}){
  Fixture f(500,60,600);inventory::Profile profile;profile.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;item.definition.id=100;item.definition.type=26;item.definition.quantity=1;item.definition.modifiers={0x22b80014};item.bag=1;profile.items.emplace(100,item);profile.weapons.sets[0].weapon=100;
  const auto equipment=*items::weapon_passives::equipped_enchanting(profile,1);const auto plan=*effects::flame_djinn_haste(rank);
  const auto duration=effects::enchanting_duration(1381,plan.duration,f.owner,equipment,*f.w.status_snapshot(f.owner,100),100);check(duration.has_value());
  check(f.w.flame_djinn_haste(f.owner,plan,20,100,&*duration).has_value());check(f.w.status_snapshot(f.owner,100)->front().expires==100+plan.duration+(rank==0?2000:3000));
  check(!f.w.flame_djinn_haste(f.owner,plan,20,101,&*duration));++cases;
 }
 for(bool stance:{false,true})for(bool zephyr:{false,true}){
  const std::vector<effects::Entry> active=stance?std::vector<effects::Entry>{{1,1,13,12,effects::Category::stance,100,10000}}:std::vector<effects::Entry>{};
  check(skills::timing::recharge(*skills::find_skill(1381),6,0,false,active,100,20,.5,zephyr)==10);++cases;
 }
 check(cases==281);std::cout<<"PASS "<<cases<<" Flame Djinn mixed damage/conversion, movement lifetime, atomic admission and paid Runtime cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
